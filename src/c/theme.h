#pragma once
#include <pebble.h>

// Timeline-Look wie die Schwesterapps: weisser Grund, schwarze Schrift.
// Auf einem Schirm, der spiegelt statt zu leuchten, ist Schwarz auf Weiss bei
// Sonne das Einzige, was traegt - und am Steuer schaut man bei Sonne.
#define KS_COLOR_BG      GColorWhite
#define KS_COLOR_TEXT    GColorBlack
#define KS_COLOR_ZART    PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)
#define KS_COLOR_BALKEN  PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack)
#define KS_COLOR_BALKEN_BG PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite)

// Breit heisst: emery (200) und gabbro (260). Flint ist mit 144 schmal, und
// dort muss jede Schrift eine Stufe kleiner sein.
#define KS_BREIT (PBL_DISPLAY_WIDTH >= 180)

// Der Rand. Auf der runden Uhr weiter, sonst schneidet der Kreis unten links
// den Text an.
#define KS_RAND PBL_IF_ROUND_ELSE(26, 8)
