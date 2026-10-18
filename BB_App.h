// BB_App.h
// Guided brew engine, per-frame update, gesture handling, rotation, standby and wake
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

// ---------------------------------------------------------------------------
// Guided brew engine
// ---------------------------------------------------------------------------
static const char* stepDisplayName(int m, const Recipe& r, int i) {
  const Step& st = r.steps[i];
  switch (st.type) {
    case STEP_BLOOM:
      if (hasValve(m)) return st.valve ? "Open Bloom" : "Closed Bloom";
      return "Bloom Pour";
    case STEP_WAIT:
      if (i > 0 && r.steps[i - 1].type == STEP_BLOOM) {
        if (hasValve(m)) return st.valve ? "Bloom Drawdown" : "Bloom Rest";
        return m == METHOD_POUR_OVER ? "Bloom Drawdown" : "Bloom Rest";
      }
      return "Wait";
    case STEP_POUR:
      if (hasValve(m)) return st.valve ? "Open Pour" : "Closed Pour";
      return m == METHOD_POUR_OVER ? "Open Pour" : "Pour";
    default: return STEP_INFO[st.type].name;
  }
}
static int cumTarget(int idx) {            // total water in the vessel once this step has finished
  int w = 0;
  for (int i = 0; i <= idx && i < run.nSteps; i++) if (STEP_INFO[run.steps[i].type].water) w += run.steps[i].water;
  return w;
}
static void stepInstruction(char* out, size_t n, int idx) {
  const Step& s = run.steps[idx];
  switch (s.type) {
    case STEP_ADD_WATER:   snprintf(out, n, "Add water to %dg", cumTarget(idx)); break;
    case STEP_ADD_COFFEE:  snprintf(out, n, "Add the coffee"); break;
    case STEP_BLOOM:       snprintf(out, n, "Pour to %dg to bloom", cumTarget(idx)); break;
    case STEP_WAIT:
      if (idx > 0 && run.steps[idx - 1].type == STEP_BLOOM) snprintf(out, n, "Wait for bloom");
      else snprintf(out, n, "Wait");
      break;
    case STEP_POUR:        snprintf(out, n, "Pour to %dg in spiral motion", cumTarget(idx)); break;
    case STEP_STIR:        snprintf(out, n, "Stir gently"); break;
    case STEP_BREAK_CRUST: snprintf(out, n, "Break the crust"); break;
    case STEP_DRAWDOWN:
      if (activeMethod == METHOD_CLEVER_DRIPPER) snprintf(out, n, s.valve ? "Place on vessel to drain" : "Allow water to settle");
      else if (hasValve(activeMethod) && s.valve) snprintf(out, n, "Valve open - let it drain");
      else snprintf(out, n, "Allow water to settle");
      break;
    default:               snprintf(out, n, "Press down slowly"); break;
  }
}

static void beginReady() {
  readyStart = millis();
  dReady.prog = 0;
  char b[8]; snprintf(b, sizeof(b), "%d", config.getReadyDuration);
  setTxt(rdNum, b);
  appState = APP_READY;
  loadScr(scrReady);
}
static void updateBrewUI(uint32_t now, float cum, float frac, uint32_t remainMs) {
  const Step& st = run.steps[bStep];
  char b[48];
  int rem = (int)((remainMs + 999) / 1000);
  snprintf(b, sizeof(b), "%d/%d  %s", bStep + 1, run.nSteps, stepDisplayName(activeMethod, run, bStep)); setTxt(bStepL, b);
  if (bPaused) snprintf(b, sizeof(b), "Paused - tap to resume"); else stepInstruction(b, sizeof(b), bStep);
  setTxt(bInstr, b);
  fmtTime(b, sizeof(b), rem); setTxt(bTimeL, b);
  int cs = bPaused ? 1 : ((rem <= 3 && rem > 0) ? 2 : 0);              // colour cue: gold when paused, orange for the last 3 s
  if (cs != bTimeColState) { bTimeColState = cs; lv_obj_set_style_text_color(bTimeL, cs == 1 ? COL_AMBER : (cs == 2 ? COL_ORANGE : COL_WHITE), 0); }
  if (!bPaused && rem <= 3 && rem > 0 && rem != bLastRem) dBrew.flash = fmaxf(dBrew.flash, 0.55f);   // pulse each of the last 3 seconds
  bLastRem = rem;
  snprintf(b, sizeof(b), "%dg", (int)(cum + 0.5f)); setTxt(bWeight, b);
  if (STEP_INFO[st.type].water && st.secs > 0) {
    int f10 = (st.water * 10 + st.secs / 2) / st.secs;
    snprintf(b, sizeof(b), "%d.%dg/s", f10 / 10, f10 % 10);
  } else b[0] = 0;
  setTxt(bFlow, b);
  fmtTime(b, sizeof(b), (int)((now - bStart) / 1000)); setTxt(bTotal, b);

  bool water = STEP_INFO[st.type].water;
  lv_color_t c = water ? COL_BLUE : COL_ORANGE;
  dBrew.col = c; dBrew.wcol = c; dBrew.prog = frac; dBrew.level = bLevel;
  if (hasValve(activeMethod)) {
    bool open = st.valve != 0;
    lv_obj_set_style_bg_color(bValveDot, open ? COL_GREEN : COL_RED, 0);
    lv_obj_set_style_text_color(bValveLbl, open ? COL_GREEN : COL_RED, 0);
    setTxt(bValveLbl, open ? "OPEN" : "CLOSED");
  }
  lv_obj_invalidate(oBrew);
}
static void beginBrew() {
  uint32_t now = millis();
  bStep = 0; bStart = bStepStart = bLastFrame = now;
  bLevel = 0; bStepStartLevel = 0; bCumBase = 0;
  bPaused = false; bPauseT = 0; bFlashT = now; bLastRem = -1; bTimeColState = 0; dBrew.flash = 0;
  lv_obj_set_style_text_color(bTimeL, COL_WHITE, 0);
  bCap = fmaxf(1.0f, recipeSumWater(run) * 1.08f);
  showObj(bValveDot, hasValve(activeMethod));
  showObj(bValveLbl, hasValve(activeMethod));
  appState = APP_BREW;
  loadScr(scrBrew);
  const Step& st = run.steps[0];
  updateBrewUI(now, 0, 0, (uint32_t)st.secs * 1000UL);
}
static void startDone() {
  doneStart = millis();
  char b[24], tm[8];
  fmtTime(tm, sizeof(tm), (int)((doneStart - bStart) / 1000));
  snprintf(b, sizeof(b), "Total %s", tm);
  setTxt(dnTime, b);
  dDone.tickP = 0;
  histAdd(activeMethod, run.name, run.cups, 0, (doneStart - bStart) / 100, NULL, 0);
  appState = APP_DONE;
  loadScr(scrDone);
}
// ---- pause / skip / back (tap, swipe left, swipe right during a brew)
static void brewTogglePause(uint32_t now) {
  if (!bPaused) { bPaused = true; bPauseT = now; }
  else { uint32_t d = now - bPauseT; bStepStart += d; bStart += d; bPaused = false; bLastFrame = now; }
  dBrew.flash = 0.8f;
}
static float levelAtStepStart(int idx) {           // water level when step idx begins (replays the earlier steps)
  float lv = 0;
  for (int i = 0; i < idx && i < run.nSteps; i++) {
    const Step& s = run.steps[i];
    if (STEP_INFO[s.type].water) lv = 1.0f; else if (stepValveOpen(activeMethod, s)) lv = 0.0f;
  }
  return lv;
}
static float cumBefore(int idx) {
  float w = 0;
  for (int i = 0; i < idx && i < run.nSteps; i++) if (STEP_INFO[run.steps[i].type].water) w += run.steps[i].water;
  return w;
}
static void brewGoto(int idx, uint32_t now) {
  if (idx >= run.nSteps) { startDone(); return; }
  bStep = idx;
  bCumBase = cumBefore(idx);
  bStepStartLevel = levelAtStepStart(idx);
  bLevel = bStepStartLevel;
  bStepStart = bPaused ? bPauseT : now;
  bLastFrame = now;
  dBrew.flash = 1.0f;
}
static void brewSkip(uint32_t now) { brewGoto(bStep + 1, now); }
static void brewBack(uint32_t now) {               // restart this step, or go to the previous one if it has only just begun
  uint32_t nowEff = bPaused ? bPauseT : now;
  if (bStep == 0 || nowEff - bStepStart > 3000) brewGoto(bStep, now); else brewGoto(bStep - 1, now);
}

static void updateBrew(uint32_t realNow) {
  uint32_t now = bPaused ? bPauseT : realNow;      // time stands still while paused
  float fdt = (realNow - bFlashT) / 1000.0f;
  bFlashT = realNow;
  if (fdt > 0.1f) fdt = 0.1f;
  dBrew.flash = fmaxf(0.0f, dBrew.flash - fdt * 2.0f);   // pulse fades out over half a second
  float dt = (now - bLastFrame) / 1000.0f;
  bLastFrame = now;
  if (dt > 0.1f) dt = 0.1f;
  const Step* st = &run.steps[bStep];
  uint32_t dur = (uint32_t)imax(1, st->secs) * 1000UL;
  uint32_t el = now - bStepStart;
  if (el >= dur) {
    bLevel = STEP_INFO[st->type].water ? 1.0f : (stepValveOpen(activeMethod, *st) ? 0.0f : bStepStartLevel);
    if (STEP_INFO[st->type].water) bCumBase += st->water;
    bStep++;
    if (bStep >= run.nSteps) { startDone(); return; }
    bStepStart += dur;
    bStepStartLevel = bLevel;
    dBrew.flash = 1.0f;                            // pulse the ring on every step change
    st = &run.steps[bStep];
    dur = (uint32_t)imax(1, st->secs) * 1000UL;
    el = now - bStepStart;
    if (el > dur) el = dur;
  }
  bool water = STEP_INFO[st->type].water;
  bool valve = stepValveOpen(activeMethod, *st);
  float frac = (float)el / (float)dur;
  // Pour-type steps fill the whole circle; every other step drains it completely unless the valve is closed
  if (water)       bLevel = bStepStartLevel + (1.0f - bStepStartLevel) * frac;
  else if (valve)  bLevel = bStepStartLevel * (1.0f - frac);
  else             bLevel = bStepStartLevel;
  bLevel = fclampf(bLevel, 0.0f, 1.0f);
  float cum = bCumBase + (water ? st->water * frac : 0.0f);
  updateBrewUI(now, cum, frac, dur - el);
}

// ---------------------------------------------------------------------------
// Per-frame application update
// ---------------------------------------------------------------------------
static void updateApp(uint32_t now) {
  switch (appState) {
    case APP_SELECT:
      if (curMethod() == METHOD_ESPRESSO && espAuto) {
        dEsp.prog = fmodf(now * 0.25f, 360.0f);
        lv_obj_invalidate(oEsp);
        if (now - lastRotChange > 1500 && now - bootTs > 2000) {
          if (vibRms > vibThreshold()) {
            if (!vibAbove) { vibAbove = true; vibSince = now; }
            else if (now - vibSince > 350) { espAutoStart(vibSince); vibAbove = false; }
          } else vibAbove = false;
        }
      }
      break;
    case APP_ESP_RUN: {
      lastActivity = now;
      if (espRunAuto && !espFinished) {
        if (vibRms > vibThreshold() * 0.8f) espLastVib = now;
        else if (now - espLastVib > 1500) {
          espFinished = true; espStopT = espLastVib;
          setTxt(etStatus, "Shot completed");
          showObj(etDone, true);
          histAdd(METHOD_ESPRESSO, "Espresso", 1, 2, (espStopT - espT0) / 100, NULL, 0);
        }
      }
      char b[16]; fmtEsp(b, sizeof(b), espElapsed(now)); setTxt(etTime, b);
      break;
    }
    case APP_READY: {
      lastActivity = now;
      float dur = config.getReadyDuration * 1000.0f;
      float el = (float)(now - readyStart);
      dReady.prog = fclampf(el / dur, 0.0f, 1.0f);
      char b[8]; snprintf(b, sizeof(b), "%d", (int)ceilf((dur - el) / 1000.0f)); setTxt(rdNum, b);
      lv_obj_invalidate(oReady);
      if (el >= dur) beginBrew();
      break;
    }
    case APP_BREW:
      lastActivity = now;
      updateBrew(now);
      break;
    case APP_CONFIG:
      learnTick(now);
      break;
    case APP_CHARGING: {
      float tgt = batPctF / 100.0f;
      if (fabsf(tgt - dChg.prog) > 0.0005f) { dChg.prog += (tgt - dChg.prog) * 0.08f; lv_obj_invalidate(oChg); }
      break;
    }
    case APP_DONE:
      if (dDone.tickP < 1.0f) {
        dDone.tickP = fclampf((now - doneStart) / 1100.0f, 0.0f, 1.0f);
        lv_obj_invalidate(oDone);
      }
      break;
    default: break;
  }
}

// ---------------------------------------------------------------------------
// Gestures -> actions
// ---------------------------------------------------------------------------
static void handleGesture(const GestureEvent& ge) {
  switch (appState) {
    case APP_SELECT: {
      int m = curMethod();
      if (m == METHOD_ESPRESSO) {
        if (ge.g == G_TAP && !(!espAuto && pointInObj(esStartBtn, ge.x, ge.y))) { espAuto = !espAuto; refreshEspSelect(); queueSettingsSave(); }
        else if (ge.g == G_LONG) enterConfig();
      } else {
        if (ge.g == G_UP || ge.g == G_DOWN) { toggleCups(m); refreshOverview(); queueSettingsSave(); }
        else if (ge.g == G_LEFT)  { stepRecipe(m, +1); refreshOverview(); queueSettingsSave(); }
        else if (ge.g == G_RIGHT) { stepRecipe(m, -1); refreshOverview(); queueSettingsSave(); }
        else if (ge.g == G_DOUBLE) openMenu(NULL);
        else if (ge.g == G_LONG) enterConfig();
      }
      break;
    }
    case APP_BREW:
      if (ge.g == G_TAP) brewTogglePause(millis());
      else if (ge.g == G_LEFT) brewSkip(millis());
      else if (ge.g == G_RIGHT) brewBack(millis());
      break;
    case APP_PREP:
      if (ge.g == G_RIGHT) showSelect();
      break;
    case APP_CHARGING:
      if (ge.g == G_TAP) { chargeDismissed = true; showSelect(); }
      break;
    case APP_DONE:
      if (ge.g == G_TAP) showSelect();
      break;
    case APP_CONFIG:
      if (ge.g == G_SCRUB) { int n = abs(ge.y), d = ge.y > 0 ? 1 : -1; for (int i = 0; i < n; i++) adjustConfig(d, true); }
      else if (ge.g == G_LEFT)  { cfgPage = imin(cfgPage + 1, CFG_PAGES - 1); refreshConfig(); }
      else if (ge.g == G_RIGHT) { cfgPage = imax(cfgPage - 1, 0); refreshConfig(); }
      else if (ge.g == G_UP)    adjustConfig(+1, false);
      else if (ge.g == G_DOWN)  adjustConfig(-1, false);
      break;
    case APP_EDIT:
      if (ge.g == G_SCRUB) { int n = abs(ge.y), d = ge.y > 0 ? 1 : -1; for (int i = 0; i < n; i++) edApply(ge.x, d); }
      break;
    default: break;
  }
}

// ---------------------------------------------------------------------------
// Rotation, standby, wake
// ---------------------------------------------------------------------------
static uint32_t forceRedrawUntil = 0;
static void applyRotation(int newRot, bool select) {
  physRot = newRot; rotCand = newRot;
  activeScreenRotation = (4 - physRot) % 4;
  tft.setRotation(activeScreenRotation);
  lastRotChange = millis(); lastActivity = lastRotChange;
  if (select) showSelect();
  lv_obj_invalidate(lv_screen_active());
  lv_refr_now(NULL);                       // repaint immediately in the new orientation
  forceRedrawUntil = millis() + 300;       // and keep repainting for a few frames
  Serial.printf("[ROT] phys=%d screenRot=%d method=%d\n", physRot, activeScreenRotation, curMethod());
}
#define LIGHT_SLEEP_ENABLE 1        // 0 = plain polling standby (use this if light sleep causes USB / touch problems)
#define STANDBY_POLL_MS    400      // how often standby wakes briefly to check for motion
#define STANDBY_WAKE_MG    350.0f   // change in orientation (mg) that wakes the device
static float sbBase[3] = {0, 0, 1000}, sbMag = 1000;
static bool dimmed = false;

static void standbyReadAvg(float* avg, float* peak) {            // ~20 ms burst of accelerometer samples
  float sum[3] = {0, 0, 0}, pk = 0;
  const int N = 10;
  for (int i = 0; i < N; i++) {
    float acc[3] = {0, 0, 0}, gyro[3] = {0, 0, 0};
    QMI8658_read_xyz(acc, gyro, NULL);
    for (int k = 0; k < 3; k++) sum[k] += acc[k];
    float mag = sqrtf(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
    if (fabsf(mag - sbMag) > pk) pk = fabsf(mag - sbMag);
    delay(2);
  }
  for (int k = 0; k < 3; k++) avg[k] = sum[k] / N;
  *peak = pk;
}
static void standbyCaptureBaseline() {
  float pk;
  standbyReadAvg(sbBase, &pk);
  sbMag = sqrtf(sbBase[0] * sbBase[0] + sbBase[1] * sbBase[1] + sbBase[2] * sbBase[2]);
}
static bool standbyMotion() {                                    // picked up, turned or knocked?
  float avg[3], peak;
  standbyReadAvg(avg, &peak);
  float dx = avg[0] - sbBase[0], dy = avg[1] - sbBase[1], dz = avg[2] - sbBase[2];
  return peak > 600.0f || sqrtf(dx * dx + dy * dy + dz * dz) > STANDBY_WAKE_MG;
}
static void wakeUp();
static void standbyStep() {                                      // one cycle of light sleep (wakes on touch, or every STANDBY_POLL_MS)
  if (standbyMotion()) { wakeUp(); return; }
  esp_sleep_enable_timer_wakeup((uint64_t)STANDBY_POLL_MS * 1000ULL);
  gpio_wakeup_enable((gpio_num_t)Touch_INT_PIN, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();
  gpio_wakeup_disable((gpio_num_t)Touch_INT_PIN);
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) wakeUp();
}

static void goSleep() {
  appState = APP_SLEEP;
  dimmed = false;
  applyBacklight(0);
  tft.startWrite(); tft.writecommand(0x10); tft.endWrite();
  // let the touch controller use its own low-power auto-sleep: its IRQ pin then wakes us from light sleep
  Wire.beginTransmission(CST816S_ADDRESS); Wire.write(0xFE); Wire.write(0x00); Wire.endTransmission();
  standbyCaptureBaseline();
  setCpuFrequencyMhz(80);
  Serial.println("[STANDBY] sleeping");
}
static void wakeUp() {
  setCpuFrequencyMhz(240);
  tft.startWrite(); tft.writecommand(0x11); tft.endWrite();
  delay(120);
  applyBacklight(config.brightness);
  dimmed = false;
  for (int i = 0; i < 3; i++) {                    // touch controller: back to always-on so polling works
    Wire.beginTransmission(CST816S_ADDRESS); Wire.write(0xFE); Wire.write(0x01);
    if (Wire.endTransmission() == 0) break;
    delay(10);
  }
  gqCount = 0; shakeFlag = false;
  tp.down = false; tp.suppress = true; tp.lvPressed = false;
  ignoreTouchUntil = millis() + 600;
  lastActivity = millis();
  showSelect();
  if (charging && !chargeDismissed) showCharging();
  Serial.println("[STANDBY] awake");
}

