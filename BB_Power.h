// BB_Power.h
// Charging screen and battery sampling / charger detection
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

// ---------------------------------------------------------------------------
// Charging screen + battery sampling
// ---------------------------------------------------------------------------
static lv_obj_t *scrChg, *oChg, *chPct, *chTxt;
static Deco dChg;
static void updateChargeUI() {
  char b[12];
  snprintf(b, sizeof(b), "%d%%", batPct);
  setTxt(chPct, b);
  setTxt(chTxt, batPct >= 99 ? "Fully charged" : "Charging");
}
static void createCharging() {
  scrChg = newScreen();
  dChg.band = false; dChg.ring = 1;            // red (empty) -> green (full) gradient ring
  oChg = mkDeco(scrChg, &dChg);
  mkLabel(scrChg, LV_SYMBOL_CHARGE, FONT_MD, COL_GREEN, CX, 72, 60);
  chPct = mkLabel(scrChg, "0%", FONT_XL, COL_WHITE, CX, 118, 170);
  chTxt = mkLabel(scrChg, "Charging", FONT_SM, COL_DIM, CX, 160, 150);
  mkLabel(scrChg, "tap to close", FONT_XS, COL_GREY, CX, 192, 120);
}
static void showCharging() {
  appState = APP_CHARGING;
  dChg.prog = batPctF / 100.0f;
  updateChargeUI();
  loadScr(scrChg);
  lv_obj_invalidate(oChg);
}

static float readBatteryV() {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(BAT_ADC_PIN);
  return (sum / 16.0f) * BAT_DIVIDER * BAT_CAL / 1000.0f;
}
static int pctFromV(float v) {                 // typical single-cell LiPo discharge curve
  static const float TV[] = {3.27f, 3.61f, 3.69f, 3.71f, 3.73f, 3.75f, 3.77f, 3.79f, 3.80f, 3.82f, 3.84f, 3.85f, 3.87f, 3.91f, 3.95f, 3.98f, 4.02f, 4.08f, 4.11f, 4.15f, 4.20f};
  static const int   TP[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
  const int N = 21;
  if (v <= TV[0]) return 0;
  if (v >= TV[N - 1]) return 100;
  for (int i = 1; i < N; i++) {
    if (v <= TV[i]) return (int)(TP[i - 1] + (TP[i] - TP[i - 1]) * (v - TV[i - 1]) / (TV[i] - TV[i - 1]));
  }
  return 100;
}
static void batteryInit() {
  if (safeMode) return;
  float v = readBatteryV();
  batV = batSlow = batPeak = v;
  batPctF = (float)pctFromV(v);
  batPct = iclamp((int)(batPctF + 0.5f), 0, 100);
  updateBatteryWidgets();
}
// Charger detection: if CHARGE_PIN is set it is read directly. Otherwise it is inferred from the battery
// voltage: plugging a charger in makes the terminal voltage step up; removing it makes it step back down.
static void batteryTask(uint32_t now) {
  if (safeMode) return;
  static uint32_t last = 0, plugT = 0, pctHoldUntil = 0;
  static bool lastChg = false;
  static float chargeBaseV = 0, chargeOffset = 0;
  if (now - last < (appState == APP_SLEEP ? 4000UL : 1000UL)) return;
  last = now;
  float v = readBatteryV();
  if (v < 2.0f) return;                        // no battery / bad reading
  batV += 0.5f * (v - batV);
  bool wasCharging = charging;
#if CHARGE_PIN >= 0
  bool c = (digitalRead(CHARGE_PIN) == CHARGE_ACTIVE);
  if (c && !charging) { charging = true; batPlugEdge = true; }
  else if (!c && charging) { charging = false; batUnplugEdge = true; }
  else if (!c) batSlow += 0.03f * (batV - batSlow);
#else
  if (appState == APP_CONNECT || now < detectHoldUntil || now < bootTs + 3000) {
    batSlow = batV;                            // radio / start-up load swings: do not detect now
  } else if (!charging) {
    if (batV - batSlow >= CHG_RISE_V) { charging = true; batPeak = batV; batPlugEdge = true; }
    else batSlow += 0.03f * (batV - batSlow);
  } else {
    if (batV > batPeak) batPeak = batV;
    if (batV < batPeak - CHG_DROP_V) { charging = false; batUnplugEdge = true; batSlow = batV; }
  }
#endif
  // While a charger is connected the battery terminal reads HIGHER than the battery really is (charge current
  // x internal resistance). The jump at the moment of plugging in is exactly that offset, so it is measured
  // then and subtracted, which gives an estimate of the real battery voltage that keeps rising as it charges.
  if (charging && !wasCharging) { chargeBaseV = batSlow; plugT = now; chargeOffset = 0; pctHoldUntil = now + 6000; }
  if (!charging && wasCharging) { chargeOffset = 0; pctHoldUntil = now + 4000; }   // terminal voltage falls back to the true value
  if (charging) {
    if (now - plugT < 6000) chargeOffset = fmaxf(0.0f, batV - chargeBaseV);        // still settling: keep measuring the jump
    else if (batV >= 4.18f) chargeOffset = fmaxf(0.0f, chargeOffset - 0.00005f);   // constant-voltage plateau: charge current tapers off, so the offset fades
  }
  float ve = charging ? fminf(batV, batV - chargeOffset) : batV;                   // estimated real battery voltage
  if (now >= pctHoldUntil) batPctF += 0.15f * ((float)pctFromV(ve) - batPctF);     // hold the reading while the step settles
  int p = iclamp((int)(batPctF + 0.5f), 0, 100);
#if BAT_DEBUG
  Serial.printf("[BAT] terminal %.3f V  offset %.3f  battery est %.3f V  %d%%  charging=%d\n", batV, chargeOffset, ve, p, charging);
#endif
  if (p != batPct || charging != lastChg) {
    batPct = p; lastChg = charging;
    updateBatteryWidgets();
    if (appState == APP_CHARGING) updateChargeUI();
  }
}

