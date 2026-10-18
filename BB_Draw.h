// BB_Draw.h
// Custom drawing (rings, water, tick), widget helpers and the battery indicator
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

// ---------------------------------------------------------------------------
// UI building blocks
// ---------------------------------------------------------------------------
static Deco dSel, dEsp, dEspT, dPrep, dReady, dBrew, dDone, dMenu, dConf, dCfg, dEdit;
static lv_obj_t *oReady, *oBrew, *oDone, *oEsp;   // deco objects that animate

static void arcSweep(lv_layer_t* layer, int radius, int width, float startDeg, float sweepDeg,
                     lv_color_t col, bool rounded = false, lv_opa_t opa = LV_OPA_COVER) {
  if (sweepDeg < 0.3f) return;
  if (sweepDeg >= 359.5f) {          // draw full circles as two halves (no seam / gap at 0 deg)
    arcSweep(layer, radius, width, startDeg, 180.0f, col, rounded, opa);
    arcSweep(layer, radius, width, startDeg + 180.0f, 180.0f, col, rounded, opa);
    return;
  }
  float s = fmodf(startDeg, 360.0f);
  float e = s + sweepDeg;
  lv_draw_arc_dsc_t d;
  lv_draw_arc_dsc_init(&d);
  d.color = col; d.width = width; d.center.x = CX; d.center.y = CY; d.radius = radius;
  d.opa = opa; d.rounded = rounded;
  if (e <= 360.0f) {
    d.start_angle = (int)s; d.end_angle = (int)e;
    lv_draw_arc(layer, &d);
  } else {
    d.start_angle = (int)s; d.end_angle = 360;
    lv_draw_arc(layer, &d);
    d.start_angle = 0; d.end_angle = (int)(e - 360.0f);
    lv_draw_arc(layer, &d);
  }
}

static float waveY(float yTop, float amp, float dx, float phase) {
  return yTop + amp * sinf(dx * 0.055f + phase) + amp * 0.5f * sinf(dx * 0.11f - phase * 1.4f);
}
// The water is an anti-aliased filled disc (smooth circular edge). The part above the wave surface is then
// covered with black (the ring is drawn afterwards, so the cover can safely overlap its inner edge), and a
// lighter anti-aliased line is drawn along the surface.
static void drawWater(lv_layer_t* layer, const Deco* d) {
  const int Rd = 106, Rc = 109;
  if (d->level <= 0.003f) return;
  float lvl = fclampf(d->level, 0.0f, 1.0f);
  float yTop = CY + Rd - lvl * 2.0f * Rd;
  float amp = 3.6f * fclampf(fminf(lvl, 1.0f - lvl) * 8.0f, 0.0f, 1.0f);
  float phase = millis() * 0.0042f;
  lv_color_t black = lv_color_black();

  lv_draw_rect_dsc_t r;
  lv_draw_rect_dsc_init(&r);
  r.bg_color = lv_color_mix(d->wcol, black, d->wopa);
  r.bg_opa = LV_OPA_COVER; r.radius = LV_RADIUS_CIRCLE; r.border_width = 0;
  lv_area_t a; a.x1 = CX - Rd; a.y1 = CY - Rd; a.x2 = CX + Rd; a.y2 = CY + Rd;
  lv_draw_rect(layer, &r, &a);

  if (lvl < 0.995f) {
    lv_draw_rect_dsc_t k;
    lv_draw_rect_dsc_init(&k);
    k.bg_color = black; k.bg_opa = LV_OPA_COVER; k.radius = 0; k.border_width = 0;
    for (int x = CX - Rc; x <= CX + Rc; x += 2) {
      float dx = x + 1.0f - CX;
      float h2 = (float)(Rc * Rc) - dx * dx;
      if (h2 <= 0.0f) continue;
      float top = CY - sqrtf(h2) - 1.0f;
      int y2 = (int)waveY(yTop, amp, dx, phase) - 1;
      if (y2 < (int)top) continue;
      lv_area_t c; c.x1 = x; c.x2 = x + 1; c.y1 = (int)top; c.y2 = y2;
      lv_draw_rect(layer, &k, &c);
    }
    lv_draw_line_dsc_t l;
    lv_draw_line_dsc_init(&l);
    l.color = lv_color_mix(d->wcol, black, 170); l.width = 2; l.round_start = 0; l.round_end = 0;
    bool havePrev = false; float px = 0, py = 0;
    for (int x = CX - Rd + 3; x <= CX + Rd - 3; x += 6) {
      float dx = x - CX, y = waveY(yTop, amp, dx, phase), dy = y - CY;
      bool inside = (dx * dx + dy * dy) <= (float)((Rd - 3) * (Rd - 3));
      if (inside && havePrev) {
        l.p1.x = (int)px; l.p1.y = (int)py; l.p2.x = x; l.p2.y = (int)y;
        lv_draw_line(layer, &l);
      }
      havePrev = inside; px = x; py = y;
    }
  }
}

static void deco_draw(lv_event_t* e) {
  const Deco* d = (const Deco*)lv_event_get_user_data(e);
  lv_layer_t* layer = lv_event_get_layer(e);
  if (!d || !layer) return;

  if (d->water) drawWater(layer, d);

  if (d->ring) {
    arcSweep(layer, 119, 11, 0, 359.5f, COL_TRACK);
    float sweep = fclampf(d->prog, 0.0f, 1.0f) * 360.0f;
    if (d->ring == 1) {
      const int N = 60;
      for (int i = 0; i < N; i++) {
        float s0 = i * 6.0f;
        if (s0 >= sweep) break;
        float len = fminf(6.8f, sweep - s0 + 0.6f);
        lv_color_t c = lv_color_mix(COL_GREEN, COL_RED, (uint8_t)(i * 255 / (N - 1)));
        arcSweep(layer, 119, 11, 270 + s0, len, c);
      }
    } else if (d->ring == 2) {
      arcSweep(layer, 119, 11, 270, sweep, d->col);
    } else {
      arcSweep(layer, 119, 11, d->prog, 80, d->col);
    }
    if (d->flash > 0.02f) arcSweep(layer, 119, 11, 0, 360.0f, COL_WHITE, false, (lv_opa_t)(d->flash * 190));   // step / countdown pulse
  }

  if (d->tick) {
    float p = fclampf(d->tickP, 0.0f, 1.0f);
    arcSweep(layer, 62, 8, 270, fclampf(p * 1.7f, 0.0f, 1.0f) * 360.0f, COL_GREEN);
    if (p > 0.45f) {
      float q = (p - 0.45f) / 0.55f;
      int cy = 98;
      float x1 = CX - 28, y1 = cy - 2, x2 = CX - 8, y2 = cy + 20, x3 = CX + 30, y3 = cy - 22;
      lv_draw_line_dsc_t l;
      lv_draw_line_dsc_init(&l);
      l.color = COL_GREEN; l.width = 10; l.round_start = 1; l.round_end = 1;
      float a = fclampf(q * 2.0f, 0.0f, 1.0f);
      l.p1.x = (int)x1; l.p1.y = (int)y1;
      l.p2.x = (int)(x1 + (x2 - x1) * a); l.p2.y = (int)(y1 + (y2 - y1) * a);
      lv_draw_line(layer, &l);
      if (q > 0.5f) {
        float b = fclampf((q - 0.5f) * 2.0f, 0.0f, 1.0f);
        l.p1.x = (int)x2; l.p1.y = (int)y2;
        l.p2.x = (int)(x2 + (x3 - x2) * b); l.p2.y = (int)(y2 + (y3 - y2) * b);
        lv_draw_line(layer, &l);
      }
    }
  }

  if (d->dots > 0) {
    lv_draw_rect_dsc_t r;
    lv_draw_rect_dsc_init(&r);
    r.radius = LV_RADIUS_CIRCLE; r.border_width = 0;
    int total = d->dots * 7;
    for (int i = 0; i < d->dots; i++) {
      r.bg_color = (i == d->dotIdx) ? COL_AMBER : COL_GREY;
      int x = CX - total / 2 + i * 7 + 1;
      lv_area_t a; a.x1 = x; a.x2 = x + 4; a.y1 = 219; a.y2 = 224;
      lv_draw_rect(layer, &r, &a);
    }
  }
}

static lv_obj_t* mkDeco(lv_obj_t* scr, Deco* d) {
  lv_obj_t* o = lv_obj_create(scr);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, SCREEN_W, SCREEN_H);
  lv_obj_set_pos(o, 0, 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(o, deco_draw, LV_EVENT_DRAW_MAIN, d);
  return o;
}
static lv_obj_t* newScreen() {
  lv_obj_t* s = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(s, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(s, 0, 0);
  lv_obj_set_style_border_width(s, 0, 0);
  lv_obj_set_style_radius(s, 0, 0);
  lv_obj_set_style_text_color(s, COL_WHITE, 0);
  lv_obj_set_style_text_font(s, FONT_SM, 0);
  lv_obj_remove_flag(s, LV_OBJ_FLAG_SCROLLABLE);
  return s;
}
// Label centred on (cx, cy) in screen coordinates, fixed width, centred text
static lv_obj_t* mkLabel(lv_obj_t* p, const char* txt, const lv_font_t* f, lv_color_t c, int cx, int cy, int w) {
  lv_obj_t* l = lv_label_create(p);
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, c, 0);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(l, w);
  lv_label_set_text(l, txt);
  lv_obj_align(l, LV_ALIGN_CENTER, cx - CX, cy - CY);
  return l;
}
// Title: gold, "bold" by double-striking the text 1 px apart (the built-in fonts have no bold weight)
static lv_obj_t* mkTitle(lv_obj_t* scr, const char* t) {
  lv_obj_t* twin = mkLabel(scr, t, FONT_SM, COL_GOLD, CX + 1, 15, 118);
  lv_obj_t* main = mkLabel(scr, t, FONT_SM, COL_GOLD, CX, 15, 118);
  lv_obj_set_user_data(main, twin);
  return main;
}
static lv_obj_t* mkBtn(lv_obj_t* p, const char* txt, int cx, int cy, int w, int h, lv_color_t col,
                       lv_event_cb_t cb, void* ud = NULL) {
  lv_obj_t* b = lv_button_create(p);
  lv_obj_set_size(b, w, h);
  lv_obj_align(b, LV_ALIGN_CENTER, cx - CX, cy - CY);
  lv_obj_set_style_bg_color(b, col, 0);
  lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_border_width(b, 0, 0);
  lv_obj_set_style_text_color(b, COL_WHITE, 0);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  return b;
}
static void btnText(lv_obj_t* b, const char* t) { lv_obj_t* l = lv_obj_get_child(b, 0); if (l) lv_label_set_text(l, t); }
static void setTxt(lv_obj_t* l, const char* s) {
  if (!l) return;
  const char* c = lv_label_get_text(l);
  if (c && strcmp(c, s) == 0) return;
  lv_label_set_text(l, s);
  lv_obj_t* twin = (lv_obj_t*)lv_obj_get_user_data(l);   // title twin (faux bold)
  if (twin) lv_label_set_text(twin, s);
}
static void showObj(lv_obj_t* o, bool show) {
  if (!o) return;
  if (show) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}
#define ID(x) ((void*)(intptr_t)(x))
static void loadScr(lv_obj_t* s) { if (s && lv_screen_active() != s) lv_screen_load(s); }

// ---------------------------------------------------------------------------
// Battery (indicator widget + state; sampling code is further down)
// ---------------------------------------------------------------------------
#define BAT_DIVIDER     3.0f     // ADC divider ratio on the Waveshare board - calibrate against a multimeter
#define BAT_CAL         1.0f     // extra fine calibration factor
#define CHARGE_PIN      -1       // set to a GPIO if your board exposes a charger / VBUS sense line (-1 = detect from voltage)
#define CHARGE_ACTIVE   LOW      // pin level that means "charging"
#define CHG_RISE_V      0.045f   // voltage step (V) that means "charger plugged in"
#define CHG_DROP_V      0.045f   // voltage drop from the charging peak (V) that means "charger removed"
#define BAT_DEBUG       0        // 1 = print battery voltage / % / charging to Serial
static float batV = 0, batSlow = 0, batPeak = 0, batPctF = 100;
static int   batPct = 100;
static bool  charging = false, chargeDismissed = false, batPlugEdge = false, batUnplugEdge = false;
static uint32_t detectHoldUntil = 0;
static BattW battSel, battEsp;

static void mkBatt(lv_obj_t* scr, BattW& b, int cy) {      // icon + percentage, centred horizontally on row cy
  b.body = lv_obj_create(scr);
  lv_obj_remove_style_all(b.body);
  lv_obj_set_size(b.body, 22, 11);
  lv_obj_set_pos(b.body, 92, cy - 5);
  lv_obj_set_style_border_width(b.body, 1, 0);
  lv_obj_set_style_border_color(b.body, COL_DIM, 0);
  lv_obj_set_style_radius(b.body, 2, 0);
  lv_obj_remove_flag(b.body, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(b.body, LV_OBJ_FLAG_SCROLLABLE);
  b.fill = lv_obj_create(b.body);
  lv_obj_remove_style_all(b.fill);
  lv_obj_set_pos(b.fill, 1, 1);
  lv_obj_set_size(b.fill, 18, 7);
  lv_obj_set_style_bg_opa(b.fill, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(b.fill, COL_GREEN, 0);
  lv_obj_set_style_radius(b.fill, 1, 0);
  lv_obj_remove_flag(b.fill, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* tip = lv_obj_create(scr);                      // battery terminal nub
  lv_obj_remove_style_all(tip);
  lv_obj_set_size(tip, 3, 5);
  lv_obj_set_pos(tip, 114, cy - 2);
  lv_obj_set_style_bg_opa(tip, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(tip, COL_DIM, 0);
  lv_obj_set_style_radius(tip, 1, 0);
  lv_obj_remove_flag(tip, LV_OBJ_FLAG_CLICKABLE);
  b.tip = tip;
  b.lbl = mkLabel(scr, "", FONT_XS, COL_DIM, 138, cy, 44);
}
static void updateBattW(BattW& b) {
  if (!b.lbl) return;
  if (safeMode) {                                  // battery / charging / Wi-Fi are switched off after repeated crashes
    showObj(b.body, false); showObj(b.tip, false);
    lv_obj_set_width(b.lbl, 120);
    lv_obj_align(b.lbl, LV_ALIGN_CENTER, 0, 228 - CY);
    setTxt(b.lbl, "SAFE MODE");
    lv_obj_set_style_text_color(b.lbl, COL_ORANGE, 0);
    return;
  }
  char t[24];
  snprintf(t, sizeof(t), "%s%d%%", charging ? LV_SYMBOL_CHARGE " " : "", batPct);
  setTxt(b.lbl, t);
  lv_color_t c = charging ? COL_GREEN : (batPct > 35 ? COL_GREEN : (batPct > 15 ? COL_ORANGE : COL_RED));
  lv_obj_set_size(b.fill, imax(1, 18 * batPct / 100), 7);
  lv_obj_set_style_bg_color(b.fill, c, 0);
  lv_obj_set_style_text_color(b.lbl, charging ? COL_GREEN : COL_DIM, 0);
}
static void updateBatteryWidgets() { updateBattW(battSel); updateBattW(battEsp); }

