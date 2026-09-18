#include <pebble.h>
#include "nav_window.h"
#include "pfeil.h"
#include "theme.h"
#include "weg.h"

// Der Schirm, von oben nach unten:
//
//   [Pfeil]  250 m          <- Zeichen und Zahl als Gruppe, mittig
//   ================        <- Balken, erst unter 300 m
//   Bahnhofstrasse          <- wohin es geht
//   12 km · an 14:22        <- leise, was noch bleibt
//
// Die Gruppe aus Pfeil und Zahl steht MITTIG und nicht linksbuendig: sonst
// wandert die Zeile, sobald aus "980 m" ein "1,2 km" wird, und am Steuer
// springt einem die Anzeige unter dem Blick weg.

static Window *s_window;
static Layer *s_leinwand;

#define BALKEN_H (KS_BREIT ? 8 : 6)

static GFont prv_font_zahl(void) {
  return fonts_get_system_font(KS_BREIT ? FONT_KEY_LECO_38_BOLD_NUMBERS
                                        : FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
}

/**
 * Die Entfernung als Zahl und Einheit.
 *
 * ZWEIGETEILT, WEIL LECO KEINE BUCHSTABEN HAT. Es hat ein Komma - nachgesehen,
 * nicht angenommen -, aber kein m und kein k. Die Zahl steht also in LECO, die
 * Einheit daneben in GOTHIC.
 */
static void prv_zerlege(int32_t meter, char *zahl, size_t zl, char *einheit, size_t el) {
  if (meter < 1000) {
    snprintf(zahl, zl, "%d", (int)meter);
    snprintf(einheit, el, "%s", "m");
  } else {
    const int32_t zehntel = (meter + 50) / 100;   // auf 100 m gerundet
    snprintf(zahl, zl, "%d,%d", (int)(zehntel / 10), (int)(zehntel % 10));
    snprintf(einheit, el, "%s", "km");
  }
}

static void prv_zeichne(Layer *layer, GContext *ctx) {
  const GRect b = layer_get_bounds(layer);
  const Weg *w = weg_stand();
  const int16_t breite = (int16_t)(b.size.w - 2 * KS_RAND);
  const GFont f_zart = fonts_get_system_font(KS_BREIT ? FONT_KEY_GOTHIC_18
                                                      : FONT_KEY_GOTHIC_14);
  const GFont f_name = fonts_get_system_font(KS_BREIT ? FONT_KEY_GOTHIC_24_BOLD
                                                      : FONT_KEY_GOTHIC_18_BOLD);

  graphics_context_set_text_color(ctx, KS_COLOR_TEXT);

  // --- Kein gueltiger Stand ---
  // Die Uhr laedt beim Oeffnen den letzten Stand. Einer von gestern saehe aus
  // wie eine gueltige Anweisung, und man faehrt danach. Also sagen, was Sache
  // ist - und klein dazu, wann zuletzt etwas kam.
  if (weg_veraltet() || !weg_hat_zahl()) {
    int16_t y = (int16_t)(b.size.h / 3);
    if (!weg_veraltet() && pfeil_kennt(w->abbiegeart)) {
      // Kein Meterwert, aber eine Richtung: das ist der Augenblick des
      // Abbiegens selbst. Dann steht der Pfeil allein da.
      const int16_t ph = pfeil_hoehe();
      y = (int16_t)(b.size.h / 3 - ph * 2 / 3);
      if (y < KS_RAND) y = KS_RAND;
      pfeil_zeichne(ctx, GPoint((int16_t)(b.size.w / 2), (int16_t)(y + ph / 2)),
                    w->abbiegeart);
      y = (int16_t)(y + ph + (KS_BREIT ? 10 : 7));
      if (w->strasse[0]) {
        graphics_draw_text(ctx, w->strasse, f_name,
                           GRect(KS_RAND, y, breite, (KS_BREIT ? 30 : 24) * 2),
                           GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
      }
      return;
    }
    graphics_draw_text(ctx, "Keine Navigation", f_name,
                       GRect(KS_RAND, y, breite, (KS_BREIT ? 30 : 24) * 2),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    char wann[28];
    weg_alter_text(wann, sizeof(wann));
    char zeile[28 + KS_STRASSE_LEN];
    snprintf(zeile, sizeof(zeile), "zuletzt %s", wann);
    graphics_context_set_text_color(ctx, KS_COLOR_ZART);
    graphics_draw_text(ctx, zeile, f_zart,
                       GRect(KS_RAND, (int16_t)(y + (KS_BREIT ? 62 : 50)), breite, 40),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    return;
  }

  int16_t y = PBL_IF_ROUND_ELSE(34, 10);

  // --- Pfeil und Zahl als Gruppe, mittig ---
  char zahl[12], einheit[4];
  prv_zerlege(w->entfernung_m, zahl, sizeof(zahl), einheit, sizeof(einheit));
  const GFont fz = prv_font_zahl();
  const GSize mz = graphics_text_layout_get_content_size(
      zahl, fz, GRect(0, 0, breite, 60), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
  const GSize me = graphics_text_layout_get_content_size(
      einheit, f_zart, GRect(0, 0, breite, 30), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);

  const bool mit_pfeil = pfeil_kennt(w->abbiegeart);
  const int16_t pb = mit_pfeil ? (int16_t)(pfeil_breite() + (KS_BREIT ? 10 : 6)) : 0;
  const int16_t gesamt = (int16_t)(pb + mz.w + 2 + me.w);
  int16_t x = (int16_t)((b.size.w - gesamt) / 2);
  if (x < KS_RAND) x = KS_RAND;

  if (mit_pfeil) {
    pfeil_zeichne(ctx, GPoint((int16_t)(x + pfeil_breite() / 2),
                              (int16_t)(y + mz.h / 2)), w->abbiegeart);
    x = (int16_t)(x + pb);
  }
  graphics_context_set_text_color(ctx, KS_COLOR_TEXT);
  graphics_draw_text(ctx, zahl, fz, GRect(x, y, (int16_t)(mz.w + 4), (int16_t)(mz.h + 4)),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, einheit, f_zart,
                     GRect((int16_t)(x + mz.w + 2),
                           (int16_t)(y + mz.h - (KS_BREIT ? 22 : 16)),
                           (int16_t)(me.w + 4), 24),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  y = (int16_t)(y + mz.h + (KS_BREIT ? 6 : 4));

  // --- Balken ---
  const int proz = weg_balken_prozent();
  if (proz >= 0) {
    const int16_t voll = (int16_t)((int32_t)proz * breite / 100);
    graphics_context_set_fill_color(ctx, KS_COLOR_BALKEN_BG);
    graphics_fill_rect(ctx, GRect(KS_RAND, y, breite, BALKEN_H), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, KS_COLOR_BALKEN);
    graphics_fill_rect(ctx, GRect(KS_RAND, y, voll, BALKEN_H), 0, GCornerNone);
    y = (int16_t)(y + BALKEN_H + (KS_BREIT ? 8 : 5));
  }

  // --- Wohin es geht ---
  if (w->strasse[0]) {
    const GSize mn = graphics_text_layout_get_content_size(
        w->strasse, f_name, GRect(0, 0, breite, 100), GTextOverflowModeWordWrap,
        GTextAlignmentCenter);
    graphics_context_set_text_color(ctx, KS_COLOR_TEXT);
    graphics_draw_text(ctx, w->strasse, f_name, GRect(KS_RAND, y, breite, mn.h + 4),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    y = (int16_t)(y + mn.h + 2);
  }

  // --- Was noch bleibt ---
  // Restweg und Ankunft in einer Zeile, leise. Sie sind kein Fahrbefehl,
  // sondern Auskunft - und sollen den Blick nicht von der Zahl oben wegziehen.
  char unten[40] = "";
  if (w->rest_m > 0) {
    if (w->rest_m < 1000) snprintf(unten, sizeof(unten), "%d m", (int)w->rest_m);
    else snprintf(unten, sizeof(unten), "%d km", (int)((w->rest_m + 500) / 1000));
  }
  if (w->ankunft > 0) {
    char uhr[10];
    clock_copy_time_string(uhr, sizeof(uhr));   // Format der Uhr uebernehmen
    struct tm *an = localtime(&w->ankunft);
    char hhmm[10];
    strftime(hhmm, sizeof(hhmm), clock_is_24h_style() ? "%H:%M" : "%I:%M", an);
    if (unten[0]) {
      const size_t n = strlen(unten);
      snprintf(unten + n, sizeof(unten) - n, " · an %s", hhmm);
    } else {
      snprintf(unten, sizeof(unten), "an %s", hhmm);
    }
  }
  if (unten[0]) {
    graphics_context_set_text_color(ctx, KS_COLOR_ZART);
    graphics_draw_text(ctx, unten, f_zart, GRect(KS_RAND, y, breite, 30),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void prv_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_leinwand = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_leinwand, prv_zeichne);
  layer_add_child(root, s_leinwand);
  pfeil_init();
}

static void prv_unload(Window *window) {
  pfeil_deinit();
  layer_destroy(s_leinwand);
  s_leinwand = NULL;
  window_destroy(s_window);
  s_window = NULL;
}

void nav_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_background_color(s_window, KS_COLOR_BG);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_load, .unload = prv_unload,
  });
  window_stack_push(s_window, true);
}

void nav_window_refresh(void) {
  if (s_leinwand) layer_mark_dirty(s_leinwand);
}
