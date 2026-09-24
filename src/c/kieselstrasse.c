#include <pebble.h>
#include "nav_window.h"
#include "strings.h"
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
  // Die Sprache zuerst: der erste Aufbau des Schirms braucht sie schon.
  strings_refresh();
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

// --- App Glance ---
//
// DIE ZEILE IM STARTER: der naechste Schritt, ohne die App zu oeffnen -
// "250 m Bahnhofstrasse, an 14:32". Sie verfaellt von selbst, sobald der
// Stand zu alt waere (KS_VERALTET_S nach dem Empfang): eine Zeile, die
// gestern stehen blieb, saehe aus wie eine Anweisung fuer heute. Danach
// steht "Keine Navigation" da, und das ist die Wahrheit.
static void prv_glance(AppGlanceReloadSession *session, size_t limit, void *context) {
  if (limit < 1) return;
  const Weg *w = weg_stand();
  char text[64];
  time_t ablauf = APP_GLANCE_SLICE_NO_EXPIRATION;

  if (!weg_veraltet() && (weg_hat_zahl() || w->strasse[0])) {
    char zahl[16] = "";
    if (weg_hat_zahl()) {
      if (w->entfernung_m < 1000) {
        snprintf(zahl, sizeof(zahl), "%d m ", (int)w->entfernung_m);
      } else {
        const int32_t zehntel = (w->entfernung_m + 50) / 100;
        snprintf(zahl, sizeof(zahl), "%d,%d km ", (int)(zehntel / 10), (int)(zehntel % 10));
      }
    }
    char an[16] = "";
    if (w->ankunft > 0) {
      struct tm *t = localtime(&w->ankunft);
      char hhmm[8];
      snprintf(hhmm, sizeof(hhmm), "%02d:%02d", t->tm_hour, t->tm_min);
      snprintf(an, sizeof(an), S(STR_GLANCE_ANKUNFT), hhmm);
    }
    snprintf(text, sizeof(text), "%s%s%s", zahl, w->strasse, an);
    ablauf = w->empfangen + weg_veraltet_nach_s();
  } else {
    snprintf(text, sizeof(text), "%s", S(STR_KEINE_NAVIGATION));
  }

  const AppGlanceSlice slice = {
    .layout = { .icon = APP_GLANCE_SLICE_DEFAULT_ICON, .subtitle_template_string = text },
    .expiration_time = ablauf,
  };
  app_glance_add_slice(session, slice);
}

static void prv_deinit(void) {
  app_glance_reload(prv_glance, NULL);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
  return 0;
}
