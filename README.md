# Kieselstrasse

Navigationsanzeige für die Pebble, für **OsmAnd**. Sie rechnet nichts und weiss
nichts — sie zeigt, was [Kiesel-Helper](https://github.com/dysseus-pascal/Kiesel-Helper)
ihr vom Telefon schickt.

*Kieselstrasse, weil Kiesel-Helper die Strasse schickt.*

## Was sie zeigt

| | |
|---|---|
| **Pfeil** | die Abbiegeart, links neben der Entfernung |
| **Entfernung** | gross in LECO, unter 1 km in Metern, darüber in Kilometern mit einer Stelle |
| **Balken** | erscheint unter 300 m und läuft leer — die Spanne, in der ein Blick aufs Handgelenk noch etwas ändert |
| **Strasse** | wohin es geht |
| **unten, leise** | Restweg und Ankunftszeit |

**Ein Stand älter als zehn Minuten ist keine Navigation mehr.** Dann steht
»Keine Navigation« da, und darunter klein, wann zuletzt etwas kam. Die Uhr lädt
beim Öffnen zwar den gespeicherten Stand — aber eine Anzeige, die Altes wie
Neues aussehen lässt, ist schlimmer als eine leere. Solange die Karten-App
nachschiebt, sagt ein Zeitstempel nichts; bleibt er stehen, **muss** man es
sehen.

## Der Pfeil kommt aus einer Zahl

Das ist der Unterschied zu allem, was hier vorher stand. OsmAnd liefert die
Abbiegeart als **Kennzahl** — dieselbe, mit der es selbst zeichnet:

| | | | |
|---|---|---|---|
| 1 `C` geradeaus | 2 `TL` links | 3 `TSLL` leicht links | 4 `TSHL` scharf links |
| 5 `TR` rechts | 6 `TSLR` leicht rechts | 7 `TSHR` scharf rechts | 8 `KL` links halten |
| 9 `KR` rechts halten | 10 `TU` wenden | 11 `TRU` wenden rechtsherum | 12 `OFFR` abseits der Route |
| 13 `RNDB` Kreisverkehr | 14 `RNLB` Kreisverkehr linksherum | | |

Der Vorgänger bekam nur einen Satz — »80 m • Turn right and go« — und musste
die Richtung aus **Wörtern** erraten: in zwei Sprachen, mit Wortgrenzen, damit
»Rechtsweg« keinen Rechtspfeil auslöst. Kreisverkehre standen in keiner Liste,
weil niemand alle Formulierungen kennt. Mit einer Kennzahl stellt sich die
Frage nicht mehr.

Acht Richtungen entstehen aus **einer** Form, gedreht. »Halten« bekommt
denselben Winkel wie »leicht« — auf zweihundert Punkten ist der Unterschied
zwischen 30 und 45 Grad keiner mehr, und zwei fast gleiche Pfeile nebeneinander
verwirren mehr, als sie sagen.

**Kreisverkehr und »abseits der Route« sind keine gedrehten Pfeile**, sondern
eigene Zeichen. Einen Kreisverkehr als schräge Spitze zu zeichnen hiesse, über
die Kreuzung zu lügen, die gleich kommt.

Ist die Kennzahl 0 oder unbekannt, steht **kein** Pfeil da. Eine geratene
Richtung wäre schlimmer als keine.

## Gesummt wird bei einem neuen Schritt

Und »neu« heisst hier schlicht: **eine andere Kennzahl**. Eine schrumpfende
Entfernung ist derselbe Schritt.

Beim Vorgänger war das die schwerste Stelle. Dort kam nur Text an, in dem die
Entfernung mitstand, und jede Meldung sah neu aus — die Uhr summte alle paar
hundert Meter Autofahrt. Am Steuer aufgefallen, nicht im Emulator. Mit einer
Zahl ist die Frage trivial.

Darunter liegt trotzdem ein Netz: **höchstens einmal alle 20 Sekunden**. Der
Preis ist ehrlich zu nennen — folgt ein echter zweiter Schritt binnen zwanzig
Sekunden (»rechts, dann sofort links«), bleibt sein Summen aus. Auf dem Schirm
steht er trotzdem. **In der Ruhezeit** schweigt die Uhr ganz, so wie sie es
auch mit Mitteilungen hält.

## Was sie empfängt

| Feld | Nummer | Art | Bedeutung |
|---|---|---|---|
| `ABBIEGEART` | 10000 | Zahl | OsmAnds TurnType, 1–14; 0 = unbekannt |
| `ENTFERNUNG` | 10001 | Zahl | Meter bis zur Abzweigung; **−1 heisst: gar keine** |
| `STRASSE` | 10002 | Text | wohin es geht; **leer heisst: löschen** |
| `ANKUNFT` | 10003 | Zahl | Ankunftszeit, Sekunden seit 1970 |
| `REST` | 10004 | Zahl | Meter bis zum Ziel |

Alle einzeln und alle freiwillig. Fehlt ein Schlüssel, bleibt das alte Feld
stehen — so kann das Telefon die Entfernung nachschieben, ohne den
Strassennamen mitzuschicken. Steht der Schlüssel aber da und ist leer, wird
geräumt; sonst bliebe nach dem Ende die letzte Strasse stehen.

Die Nummern ergeben sich aus der Reihenfolge der `messageKeys` in der
`package.json`, beginnend bei 10000. **Wer dort eine Zeile dazwischenschiebt,
verschiebt alle folgenden** — und das Telefon schickt danach still die falschen
Werte ins falsche Feld.

Geschickt wird über die klassische PebbleKit-Schnittstelle
(`com.getpebble.action.app.SEND`). Diese App hat deshalb **absichtlich keinen
`companionApp`-Eintrag** in ihrer `package.json`: ein Paketname dort wählt
PebbleKit2, und das bindet sich an einen Dienst, statt zu senden.

## Ohne Telefon ausprobieren

```bash
KIESELSTRASSE_SRC=<dieser Ordner> tools/sync_kieselstrasse.sh "" demo
```

Der Schalter `demo` setzt einen Beispielstand ein — ohne Telefon kommt sonst
nie etwas an, und der Schirm bliebe auf »Keine Navigation«.

Wer von Hand schicken will:

```bash
pebble send-app-message --emulator emery --app-uuid a83bf269-e798-45bc-9bf0-a0f68362a79a \
  --string 10002="Bahnhofstrasse" --int 10000=5 10001=250
```

Zwei Fallen im Werkzeug: mit `--app-uuid` löst es die **Namen** aus der
`package.json` nicht auf, es will die Nummern. Und **`--string` darf nur einmal
vorkommen** — steht es zweimal, überschreibt die zweite Angabe die erste, und
die Nachricht kommt lautlos unvollständig an.

## Bauen

```bash
KIESELSTRASSE_SRC=<dieser Ordner> tools/sync_kieselstrasse.sh
```

Spiegelt nach `~/kieselstrasse` und baut dort — waf verträgt keine Pfade mit
Leerzeichen. Kein `npm`: diese App braucht kein Clay, ihre Anzeige kommt nicht
von einer Konfigseite.

## Lizenz

[CC0 1.0](LICENSE) — gemeinfrei.

Diese App enthält **keinen** fremden Code. Sie empfängt Zahlen und zeichnet
sie. Kiesel-Helper, das die Zahlen bei OsmAnd abholt, steht dagegen unter
GPLv3, weil es OsmAnds Schnittstelle mitbringt — das ist dort nachzulesen und
berührt diese App nicht.
