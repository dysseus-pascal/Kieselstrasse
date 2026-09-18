#include <pebble.h>
#include "nav_window.h"
#include "weg.h"

// Wie oft hoechstens gesummt wird. Ein Netz unter dem Kern-Vergleich: sollte
// die Gegenseite je die Abbiegeart flattern lassen, klopft die Uhr trotzdem
// nicht Sturm. Der Preis ist ehrlich zu nennen - folgt ein echter zweiter
// Schritt binnen zwanzig Sekunden ("rechts, dann sofort links"), bleibt sein
// Summen aus. Auf dem Schirm steht er trotzdem.
#define KS_SUMM_PAUSE_S 20

static time_t s_zuletzt_gesummt;

static void prv_empfangen(DictionaryIterator *iter, void *context) {
  const bool neuer_schritt = weg_uebernimm(iter);
  nav_window_refresh();

  if (!neuer_schritt) return;
  const time_t jetzt = time(NULL);
  if (jetzt - s_zuletzt_gesummt < KS_SUMM_PAUSE_S) return;
  s_zuletzt_gesummt = jetzt;
  // In der Ruhezeit schweigt die Uhr - so haelt sie es auch mit Mitteilungen.
  if (!quiet_time_is_active()) vibes_short_pulse();
}

#ifdef KS_DEMO
// Nur im Pruefbau: ein Stand, damit sich die Anzeige im Emulator ansehen
// laesst. Ohne Telefon kommt sonst nie etwas an.
static void prv_demo(void) {
  uint8_t puffer[128];
  Tuplet werte[] = {
    TupletInteger(MESSAGE_KEY_ABBIEGEART, (int32_t)5),      // TR, rechts
    TupletInteger(MESSAGE_KEY_ENTFERNUNG, (int32_t)250),
    TupletCString(MESSAGE_KEY_STRASSE, "Bahnhofstrasse"),
    TupletInteger(MESSAGE_KEY_REST, (int32_t)12400),
    TupletInteger(MESSAGE_KEY_ANKUNFT, (int32_t)(time(NULL) + 1500)),
  };
  // DIE BENUTZTE GROESSE ZURUECKLESEN, nicht die des Puffers weiterreichen.
  // Wer zum Lesen die ganzen 128 Byte reicht, laesst den Leser hinter dem Ende
  // weiterlaufen; er findet dort Muell, meldet das Woerterbuch als kaputt und
  // dict_find liefert fuer alles null - ohne ein Wort der Warnung.
  uint32_t benutzt = sizeof(puffer);
  dict_serialize_tuplets_to_buffer(werte, ARRAY_LENGTH(werte), puffer, &benutzt);
  DictionaryIterator lesen;
  dict_read_begin_from_buffer(&lesen, puffer, (uint16_t)benutzt);
  weg_uebernimm(&lesen);
}
#endif

static void prv_init(void) {
  weg_init();
#ifdef KS_DEMO
  prv_demo();
#endif
  nav_window_push();

  app_message_register_inbox_received(prv_empfangen);
  // Klein genug fuer fuenf Felder, gross genug fuer einen langen
  // Strassennamen. Ein zu knapper Eingang verwirft die Nachricht still.
  app_message_open(256, 64);
}

int main(void) {
  prv_init();
  app_event_loop();
  return 0;
}
