#include <pebble.h>
#include "weg.h"
#include "strings.h"

#define PERSIST_WEG 1

// Ab wann ein Stand keine Navigation mehr ist. Zehn Minuten sind lang genug
// fuer einen Stau und kurz genug, dass niemand einem Pfeil von gestern folgt.
#define KS_VERALTET_S 600

// Unter dieser Entfernung laeuft der Balken - die Spanne, in der ein Blick
// aufs Handgelenk noch etwas aendert.
#define KS_BALKEN_AB_M 300

static Weg s_weg;

void weg_init(void) {
  s_weg.abbiegeart = 0;
  s_weg.entfernung_m = -1;
  s_weg.rest_m = -1;
  s_weg.strasse[0] = '\0';
  s_weg.ankunft = 0;
  s_weg.empfangen = 0;
  if (persist_exists(PERSIST_WEG)) {
    persist_read_data(PERSIST_WEG, &s_weg, sizeof(s_weg));
  }
}

/**
 * Eine Zeichenkette uebernehmen.
 *
 * Fehlt der Schluessel, bleibt das alte Feld stehen - so kann das Telefon die
 * Entfernung nachschieben, ohne den Strassennamen mitzuschicken.
 *
 * STEHT DER SCHLUESSEL ABER DA UND IST LEER, WIRD GELOESCHT. Sonst bliebe nach
 * dem Ende einer Navigation die letzte Strasse unter "Keine Navigation" stehen.
 */
static void prv_nimm_text(DictionaryIterator *iter, uint32_t key, char *ziel, size_t len) {
  Tuple *t = dict_find(iter, key);
  if (!t || t->type != TUPLE_CSTRING) return;
  const char *neu = (t->length <= 1) ? "" : t->value->cstring;
  strncpy(ziel, neu, len - 1);
  ziel[len - 1] = '\0';
}

bool weg_uebernimm(DictionaryIterator *iter) {
  const int vorher = s_weg.abbiegeart;

  Tuple *a = dict_find(iter, MESSAGE_KEY_ABBIEGEART);
  if (a && a->type == TUPLE_INT) s_weg.abbiegeart = a->value->int32;

  Tuple *e = dict_find(iter, MESSAGE_KEY_ENTFERNUNG);
  if (e && e->type == TUPLE_INT) s_weg.entfernung_m = e->value->int32;

  Tuple *r = dict_find(iter, MESSAGE_KEY_REST);
  if (r && r->type == TUPLE_INT) s_weg.rest_m = r->value->int32;

  Tuple *k = dict_find(iter, MESSAGE_KEY_ANKUNFT);
  if (k && k->type == TUPLE_INT) s_weg.ankunft = (time_t)k->value->int32;

  prv_nimm_text(iter, MESSAGE_KEY_STRASSE, s_weg.strasse, KS_STRASSE_LEN);

  s_weg.empfangen = time(NULL);
  persist_write_data(PERSIST_WEG, &s_weg, sizeof(s_weg));

  // NEU heisst: eine andere Abbiegeart. Eine schrumpfende Entfernung ist
  // derselbe Schritt und kein Anlass zu summen.
  const bool neu = (s_weg.abbiegeart != vorher) && s_weg.abbiegeart > 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "Art %d -> %d, %d m : %s",
          vorher, s_weg.abbiegeart, (int)s_weg.entfernung_m,
          neu ? "NEUER SCHRITT" : "derselbe");
  return neu;
}

const Weg *weg_stand(void) { return &s_weg; }

bool weg_hat_zahl(void) { return s_weg.entfernung_m >= 0; }

bool weg_veraltet(void) {
  if (s_weg.empfangen == 0) return true;
  return (time(NULL) - s_weg.empfangen) > KS_VERALTET_S;
}

int weg_veraltet_nach_s(void) { return KS_VERALTET_S; }

void weg_alter_text(char *buf, size_t len) {
  if (s_weg.empfangen == 0) {
    snprintf(buf, len, "%s", S(STR_ALTER_NIE));
    return;
  }
  const int32_t s = (int32_t)(time(NULL) - s_weg.empfangen);
  if (s < 60) snprintf(buf, len, "%s", S(STR_ALTER_JETZT));
  else if (s < 3600) snprintf(buf, len, S(STR_ALTER_MIN), (int)(s / 60));
  else if (s < 86400) snprintf(buf, len, S(STR_ALTER_STD), (int)(s / 3600));
  else snprintf(buf, len, "%s", S(STR_ALTER_GESTERN));
}

int weg_balken_prozent(void) {
  if (s_weg.entfernung_m < 0 || s_weg.entfernung_m > KS_BALKEN_AB_M) return -1;
  // Laeuft LEER, je naeher die Abzweigung kommt.
  const int32_t p = s_weg.entfernung_m * 100 / KS_BALKEN_AB_M;
  return (int)(p < 0 ? 0 : (p > 100 ? 100 : p));
}
