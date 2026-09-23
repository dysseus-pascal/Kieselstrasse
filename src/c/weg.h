#pragma once
#include <pebble.h>

#define KS_STRASSE_LEN 48

/**
 * Was die Uhr ueber die laufende Navigation weiss.
 *
 * SIE WEISS NICHTS SELBST. Jedes Feld kommt von Kiesel-Helper, das es bei
 * OsmAnd abholt. Diese App rechnet nichts aus, sie zeigt - und genau deshalb
 * braucht sie keine Standortberechtigung, keine Karten und keinen Router.
 */
typedef struct {
  int abbiegeart;                    // OsmAnds TurnType, 0 = unbekannt
  int32_t entfernung_m;              // bis zur Abzweigung, -1 = keine Angabe
  char strasse[KS_STRASSE_LEN];
  time_t ankunft;                    // 0 = unbekannt
  int32_t rest_m;                    // bis zum Ziel, -1 = keine Angabe
  time_t empfangen;                  // wann zuletzt etwas kam
} Weg;

/** Beim Start den letzten Stand laden. */
void weg_init(void);

/**
 * Eine Nachricht uebernehmen.
 *
 * Rueckgabe true heisst: ein NEUER SCHRITT. Das ist der Fall, wenn sich die
 * Abbiegeart geaendert hat - nicht, wenn bloss die Entfernung kleiner wurde.
 * Danach richtet sich das Summen.
 *
 * Bei der Vorgaengerin war das die schwerste Stelle: dort kam nur Text an, in
 * dem die schrumpfende Entfernung mitstand, und jede Meldung sah neu aus. Die
 * Uhr summte alle paar hundert Meter Autofahrt. Mit einer Kennzahl ist die
 * Frage trivial - sie ist eine Zahl, die sich aendert oder nicht.
 */
bool weg_uebernimm(DictionaryIterator *iter);

const Weg *weg_stand(void);

/** Steht eine Zahl zum Anzeigen bereit? */
bool weg_hat_zahl(void);

/**
 * Ist der Stand zu alt, um noch eine Navigation zu sein?
 *
 * Die Uhr laedt beim Oeffnen den letzten Stand. Einer von gestern saehe aus
 * wie eine gueltige Anweisung - und man faehrt danach.
 */
bool weg_veraltet(void);
/** Nach so vielen Sekunden ohne Nachricht gilt ein Stand als veraltet. */
int weg_veraltet_nach_s(void);

/** "vor 12 min", "vor 3 Std", "gestern" - in `buf`. */
void weg_alter_text(char *buf, size_t len);

/** Wie voll der Balken steht, 0..100, oder -1 fuer keinen. */
int weg_balken_prozent(void);
