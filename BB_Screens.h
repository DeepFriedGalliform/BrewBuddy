// BB_Screens.h
// Screens, button callbacks, screen builders, refresh functions and the config pages
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

// ---------------------------------------------------------------------------
// Screens + widgets
// ---------------------------------------------------------------------------
static lv_obj_t *scrSel, *scrEsp, *scrEspT, *scrPrep, *scrReady, *scrBrew, *scrDone,
                *scrMenu, *scrConfirm, *scrCfg, *scrEdit;
// overview
static lv_obj_t *ovTitle, *ovName, *ovSub, *ovCoffee, *ovWater, *ovGrind, *ovTemp, *ovTime;
// espresso select / run
static lv_obj_t *esMode, *esMsg, *esHint, *esStartBtn;
static lv_obj_t *etTime, *etStatus, *etLaps, *etPause, *etNext, *etReset, *etDone;
// prep / ready / brew / done
static lv_obj_t *prTitle, *prText, *rdNum;
static lv_obj_t *bStepL, *bInstr, *bTimeL, *bWeight, *bFlow, *bTotal, *bValveDot, *bValveLbl;
static lv_obj_t *dnTime;
// menu / confirm / config
static lv_obj_t *mnName, *cfText;
static lv_obj_t *cgCap, *cgVal, *cgSlider, *cgMinus, *cgPlus, *cgRestore;
static int cfgPage = 0;
#define CFG_PAGES 15
// Page order. The numbers are page ids: 0-3 method slots, 4 get-ready, 5 vibration, 6 sleep, 7 grind display,
// 8 grind min, 9 grind max, 10 temperature unit, 11 restore recipes, 12 Wi-Fi, 13 brightness, 14 learn vibration
static const uint8_t CFG_ORDER[CFG_PAGES] = {0, 1, 2, 3, 4, 13, 5, 14, 6, 7, 8, 9, 10, 11, 12};

// Espresso state
static bool espRunAuto = false, espFinished = false, espRunning = false, espPaused = false;
static uint32_t espT0 = 0, espStopT = 0, espResume = 0, espAccum = 0, espLastVib = 0;
static uint32_t espLaps[4];
static int espLapCount = 0;
static bool vibAbove = false;
static uint32_t vibSince = 0;

// Guided brew state
static Recipe run;
static int   bStep = 0;
static uint32_t bStepStart = 0, bStart = 0, bLastFrame = 0, doneStart = 0;
static float bLevel = 0, bStepStartLevel = 0, bCap = 1, bCumBase = 0;
static uint32_t readyStart = 0;
static bool bPaused = false;
static uint32_t bPauseT = 0, bFlashT = 0;
static int bTimeColState = 0, bLastRem = -1;

// forward declarations
static void showSelect();
static void refreshOverview();
static void refreshEspSelect();
static void enterConfig();
static void openMenu(const char* msg);
static void startEditor(bool isNew);
static void beginReady();
static void beginBrew();
static void startDone();
static void goSleep();
static void wakeUp();
static void startConnect();
static void learnStart();
static void showCharging();

// ---- formatting helpers for espresso timer
static void fmtEsp(char* b, size_t n, uint32_t ms) {
  uint32_t t = ms / 100;
  snprintf(b, n, "%02u:%02u.%u", (unsigned)(t / 600), (unsigned)((t / 10) % 60), (unsigned)(t % 10));
}
static void fmtLap(char* b, size_t n, uint32_t ms) {
  uint32_t t = ms / 100;
  if (t < 600) snprintf(b, n, "%u.%u", (unsigned)(t / 10), (unsigned)(t % 10));
  else snprintf(b, n, "%u:%02u.%u", (unsigned)(t / 600), (unsigned)((t / 10) % 60), (unsigned)(t % 10));
}
static uint32_t espElapsed(uint32_t now) {
  if (espRunAuto) return (espFinished ? espStopT : now) - espT0;
  return espAccum + ((espRunning && !espPaused) ? now - espResume : 0);
}

// ---------------------------------------------------------------------------
// Event callbacks
// ---------------------------------------------------------------------------
enum { B_PREPARE, B_STARTBREW, B_PREP_BACK, B_ESP_START, B_ESP_PAUSE, B_ESP_NEXT, B_ESP_RESET, B_ESP_DONE,
       M_ADD, M_EDIT, M_DELETE, M_CANCEL, C_YES, C_NO, G_MINUS, G_PLUS, G_OK, G_RESTORE };

static void adjustConfig(int dir, bool fine);
static void refreshConfig();

static bool restoreArmed = false;
static void restoreDefaultRecipes() {            // throw away every saved recipe edit and reload the recipe table
  for (int m = 1; m < METHOD_COUNT; m++) {
    char k[8]; snprintf(k, sizeof(k), "t%d", m); prefs.remove(k);
    buildDefaults(m);
    recipeIdx[m] = 0;
  }
  queueSettingsSave();
}
static void deleteCurrentRecipe() {
  int m = curMethod();
  RecipeSet& s = store[m];
  int i = recipeIdx[m];
  for (int k = i; k < s.count - 1; k++) s.r[k] = s.r[k + 1];
  s.count--;
  if (recipeIdx[m] >= s.count) recipeIdx[m] = s.count - 1;
  fixSelection(m);
  saveRecipes(m);
  queueSettingsSave();
}

static void espManualStart() {
  espRunAuto = false; espFinished = false; espRunning = true; espPaused = false;
  espAccum = 0; espResume = millis(); espLapCount = 0;
  setTxt(etLaps, ""); setTxt(etStatus, "");
  btnText(etPause, LV_SYMBOL_PAUSE);
  showObj(etPause, true); showObj(etNext, true); showObj(etReset, true); showObj(etDone, false);
  appState = APP_ESP_RUN;
  loadScr(scrEspT);
}
static void espAutoStart(uint32_t t0) {
  espRunAuto = true; espFinished = false; espRunning = true; espPaused = false;
  espT0 = t0; espLastVib = millis(); espLapCount = 0;
  setTxt(etLaps, ""); setTxt(etStatus, "Shot running");
  showObj(etPause, false); showObj(etNext, false); showObj(etReset, false); showObj(etDone, false);
  appState = APP_ESP_RUN;
  loadScr(scrEspT);
}

static void cb_main(lv_event_t* e) {
  if (!clickOK()) return;
  int id = (int)(intptr_t)lv_event_get_user_data(e);
  uint32_t now = millis();
  lastActivity = now;
  switch (id) {
    case B_PREPARE: {
      activeMethod = curMethod();
      run = viewRecipe(activeMethod);
      Recipe& r = run;
      char cof[12], txt[260], gr[16], tmpS[12];
      fmtG10(cof, sizeof(cof), r.coffeeX10);
      fmtGrind(gr, sizeof(gr), r.grindX10);
      fmtTemp(tmpS, sizeof(tmpS), r.tempX10);
      switch (activeMethod) {
        case METHOD_CLEVER_DRIPPER:
          snprintf(txt, sizeof(txt), "Water first. Weigh %s beans, grind at %s. Boil water to %s. Rinse filter, no coffee yet. Place dripper on scale, valve closed, tare and press start.", cof, gr, tmpS); break;
        case METHOD_AEROPRESS:
          snprintf(txt, sizeof(txt), "Weigh %s beans, grind at %s. Boil water to %s. Rinse filter and fit to AeroPress. Place on cup, tare and press start.", cof, gr, tmpS); break;
        case METHOD_FRENCH_PRESS:
          snprintf(txt, sizeof(txt), "Weigh %s beans, grind at %s. Boil water to %s. Place French press on scale, tare and press start.", cof, gr, tmpS); break;
        default:
          snprintf(txt, sizeof(txt), "Weigh %s beans, grind at %s. Boil water to %s. Rinse filter and add ground coffee. Place cup and dripper on scale, tare and press start.", cof, gr, tmpS); break;
      }
      setTxt(prTitle, METHOD_SHORT[activeMethod]);
      lv_label_set_text(prText, txt);
      appState = APP_PREP;
      loadScr(scrPrep);
      break;
    }
    case B_STARTBREW:
      if (config.getReadyDuration > 0) beginReady(); else beginBrew();
      break;
    case B_PREP_BACK: showSelect(); break;
    case B_ESP_START: espManualStart(); break;
    case B_ESP_PAUSE:
      if (espPaused) { espPaused = false; espResume = now; btnText(etPause, LV_SYMBOL_PAUSE); setTxt(etStatus, ""); }
      else if (espRunning) { espAccum += now - espResume; espPaused = true; btnText(etPause, LV_SYMBOL_PLAY); setTxt(etStatus, "Paused"); }
      break;
    case B_ESP_NEXT:
      if (espRunning && espLapCount < 4) {
        espLaps[espLapCount++] = espElapsed(now);
        char out[96];
        out[0] = 0;
        for (int i = 0; i < espLapCount; i++) {
          char lp[16], one[32];
          fmtLap(lp, sizeof(lp), espLaps[i]);
          snprintf(one, sizeof(one), "%d: %s", i + 1, lp);
          strcat(out, one);
          if (i == 0 || i == 2) strcat(out, "    ");
          else if (i == 1 && espLapCount > 2) strcat(out, "\n");
        }
        lv_label_set_text(etLaps, out);
      }
      break;
    case B_ESP_RESET:
      if (espElapsed(now) >= 5000) histAdd(METHOD_ESPRESSO, "Espresso", 1, 1, espElapsed(now) / 100, espLaps, espLapCount);
      espRunning = false; espPaused = false; showSelect(); break;
    case B_ESP_DONE:  showSelect(); break;
    case M_ADD:
      if (store[curMethod()].count >= MAX_RECIPES) { setTxt(mnName, "Max 24 recipes - delete one"); }
      else startEditor(true);
      break;
    case M_EDIT: startEditor(false); break;
    case M_DELETE: {
      if (store[curMethod()].count <= 1) { setTxt(mnName, "Can't delete the last recipe"); break; }
      char t[48];
      snprintf(t, sizeof(t), "Delete\n\"%s\"?", recipeFor(curMethod()).name);
      lv_label_set_text(cfText, t);
      appState = APP_CONFIRM;
      loadScr(scrConfirm);
      break;
    }
    case M_CANCEL: showSelect(); break;
    case C_YES: deleteCurrentRecipe(); showSelect(); break;
    case C_NO:  openMenu(NULL); break;
    case G_MINUS: adjustConfig(-1, false); break;
    case G_PLUS:  adjustConfig(+1, false); break;
    case G_RESTORE: {
      int pg = CFG_ORDER[cfgPage];
      if (pg == 12) { startConnect(); break; }
      if (pg == 14) { learnStart(); break; }
      if (!restoreArmed) { restoreArmed = true; refreshConfig(); }
      else { restoreDefaultRecipes(); restoreArmed = false; refreshConfig(); setTxt(cgVal, "Restored"); }
      break;
    }
    case G_OK: saveConfig(); showSelect(); break;
  }
}
static void cb_slider(lv_event_t* e) {
  lv_obj_t* s = (lv_obj_t*)lv_event_get_target(e);
  config.vibrationSensitivity = (uint8_t)lv_slider_get_value(s);
  refreshConfig();
}

// ---------------------------------------------------------------------------
// Screen builders
// ---------------------------------------------------------------------------
static void createOverview() {
  scrSel = newScreen();
  mkDeco(scrSel, &dSel);
  ovTitle = mkTitle(scrSel, "");
  mkBatt(scrSel, battSel, 228);
  ovName  = mkLabel(scrSel, "", FONT_MD, COL_CREAM, CX, 46, 170);
  ovSub   = mkLabel(scrSel, "", FONT_XS, COL_DIM, CX, 66, 150);
  ovCoffee = mkLabel(scrSel, "", FONT_MD, COL_WHITE, 70, 88, 90);
  ovWater  = mkLabel(scrSel, "", FONT_MD, COL_WHITE, 170, 88, 90);
  ovGrind  = mkLabel(scrSel, "", FONT_MD, COL_WHITE, 70, 128, 90);
  ovTemp   = mkLabel(scrSel, "", FONT_MD, COL_WHITE, 170, 128, 90);
  mkLabel(scrSel, "coffee", FONT_XS, COL_DIM, 70, 106, 80);
  mkLabel(scrSel, "water",  FONT_XS, COL_DIM, 170, 106, 80);
  mkLabel(scrSel, "grind",  FONT_XS, COL_DIM, 70, 146, 80);
  mkLabel(scrSel, "temp",   FONT_XS, COL_DIM, 170, 146, 80);
  ovTime = mkLabel(scrSel, "", FONT_MD, COL_AMBER, CX, 167, 120);
  mkBtn(scrSel, "Prepare", CX, 199, 108, 34, COL_BLUE, cb_main, ID(B_PREPARE));
}
static void createEspSelect() {
  scrEsp = newScreen();
  oEsp = mkDeco(scrEsp, &dEsp);
  mkTitle(scrEsp, "Espresso");
  mkBatt(scrEsp, battEsp, 228);
  esMode = mkLabel(scrEsp, "AUTO", FONT_LG, COL_AMBER, CX, 72, 190);
  esMsg  = mkLabel(scrEsp, "Place on machine - timer will start when vibration is detected", FONT_SM, COL_WHITE, CX, 136, 160);
  esStartBtn = mkBtn(scrEsp, "Start Timer", CX, 138, 150, 46, COL_GREEN, cb_main, ID(B_ESP_START));
  esHint = mkLabel(scrEsp, "tap to switch mode", FONT_XS, COL_DIM, CX, 204, 130);
}
static void createEspRun() {
  scrEspT = newScreen();
  mkDeco(scrEspT, &dEspT);
  mkTitle(scrEspT, "Espresso");
  etTime   = mkLabel(scrEspT, "00:00.0", FONT_LG, COL_WHITE, CX, 80, 200);
  etStatus = mkLabel(scrEspT, "", FONT_SM, COL_AMBER, CX, 112, 180);
  etLaps   = mkLabel(scrEspT, "", FONT_SM, COL_CREAM, CX, 140, 190);
  etPause = mkBtn(scrEspT, LV_SYMBOL_PAUSE, 64, 186, 50, 34, COL_ORANGE, cb_main, ID(B_ESP_PAUSE));
  etNext  = mkBtn(scrEspT, LV_SYMBOL_NEXT, 120, 186, 50, 34, COL_BLUE, cb_main, ID(B_ESP_NEXT));
  etReset = mkBtn(scrEspT, LV_SYMBOL_REFRESH, 176, 186, 50, 34, COL_RED, cb_main, ID(B_ESP_RESET));
  etDone  = mkBtn(scrEspT, "Done", CX, 186, 100, 34, COL_GREEN, cb_main, ID(B_ESP_DONE));
}
static void createPrep() {
  scrPrep = newScreen();
  mkDeco(scrPrep, &dPrep);
  prTitle = mkTitle(scrPrep, "");
  prText = mkLabel(scrPrep, "", FONT_SM, COL_WHITE, CX, 106, 170);
  lv_obj_set_style_text_line_space(prText, -1, 0);
  mkBtn(scrPrep, LV_SYMBOL_LEFT, 62, 196, 46, 30, COL_GREY, cb_main, ID(B_PREP_BACK));
  mkBtn(scrPrep, "Start Brew", 146, 196, 98, 30, COL_GREEN, cb_main, ID(B_STARTBREW));
}
static void createReady() {
  scrReady = newScreen();
  dReady.band = false; dReady.ring = 1;
  oReady = mkDeco(scrReady, &dReady);
  rdNum = mkLabel(scrReady, "5", FONT_XL, COL_WHITE, CX, CY, 150);
  mkLabel(scrReady, "GET READY", FONT_XS, COL_DIM, CX, 168, 120);
}
static void createBrew() {
  scrBrew = newScreen();
  dBrew.band = false; dBrew.ring = 2; dBrew.water = true; dBrew.col = COL_BLUE; dBrew.wcol = COL_BLUE;
  oBrew = mkDeco(scrBrew, &dBrew);
  bStepL  = mkLabel(scrBrew, "", FONT_SM, COL_CREAM, CX, 32, 150);
  bInstr  = mkLabel(scrBrew, "", FONT_SM, COL_WHITE, CX, 62, 150);
  bTimeL  = mkLabel(scrBrew, "00:00", FONT_XL, COL_WHITE, CX, 116, 170);
  bWeight = mkLabel(scrBrew, "", FONT_MD, COL_WHITE, 52, 158, 72);
  bFlow   = mkLabel(scrBrew, "", FONT_SM, COL_WHITE, CX, 158, 72);
  bTotal  = mkLabel(scrBrew, "", FONT_MD, COL_WHITE, 188, 158, 72);
  bValveDot = lv_obj_create(scrBrew);
  lv_obj_remove_style_all(bValveDot);
  lv_obj_set_size(bValveDot, 12, 12);
  lv_obj_set_style_radius(bValveDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(bValveDot, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(bValveDot, COL_RED, 0);
  lv_obj_remove_flag(bValveDot, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_align(bValveDot, LV_ALIGN_CENTER, -34, 190 - CY);
  bValveLbl = mkLabel(scrBrew, "CLOSED", FONT_SM, COL_RED, CX + 14, 190, 80);
}
static void createDone() {
  scrDone = newScreen();
  dDone.band = false; dDone.tick = true;
  oDone = mkDeco(scrDone, &dDone);
  mkLabel(scrDone, "Brew complete", FONT_MD, COL_WHITE, CX, 170, 180);
  dnTime = mkLabel(scrDone, "", FONT_SM, COL_DIM, CX, 196, 120);
}
static void createMenu() {
  scrMenu = newScreen();
  mkDeco(scrMenu, &dMenu);
  mkTitle(scrMenu, "Recipes");
  mnName = mkLabel(scrMenu, "", FONT_SM, COL_AMBER, CX, 52, 170);
  mkBtn(scrMenu, LV_SYMBOL_PLUS " Add",    CX, 88,  130, 34, COL_GREEN, cb_main, ID(M_ADD));
  mkBtn(scrMenu, LV_SYMBOL_EDIT " Edit",   CX, 128, 130, 34, COL_BLUE,  cb_main, ID(M_EDIT));
  mkBtn(scrMenu, LV_SYMBOL_TRASH " Delete", CX, 168, 130, 34, COL_RED,  cb_main, ID(M_DELETE));
  mkBtn(scrMenu, LV_SYMBOL_CLOSE, CX, 207, 70, 28, COL_GREY, cb_main, ID(M_CANCEL));
}
static void createConfirm() {
  scrConfirm = newScreen();
  static Deco dConfirm;
  mkDeco(scrConfirm, &dConfirm);
  mkTitle(scrConfirm, "Delete recipe");
  cfText = mkLabel(scrConfirm, "", FONT_MD, COL_WHITE, CX, 100, 170);
  mkBtn(scrConfirm, "Yes", 76, 168, 76, 36, COL_RED,  cb_main, ID(C_YES));
  mkBtn(scrConfirm, "No", 164, 168, 76, 36, COL_GREY, cb_main, ID(C_NO));
}
static void createConfig() {
  scrCfg = newScreen();
  dConf.dots = 15;
  mkDeco(scrCfg, &dConf);
  mkTitle(scrCfg, "Setup");
  cgCap   = mkLabel(scrCfg, "", FONT_SM, COL_DIM, CX, 52, 170);
  cgVal   = mkLabel(scrCfg, "", FONT_MD, COL_WHITE, CX, 104, 112);
  cgMinus = mkBtn(scrCfg, LV_SYMBOL_MINUS, 44, 104, 44, 38, COL_GREY, cb_main, ID(G_MINUS));
  cgPlus  = mkBtn(scrCfg, LV_SYMBOL_PLUS, 196, 104, 44, 38, COL_GREY, cb_main, ID(G_PLUS));
  cgRestore = mkBtn(scrCfg, "Restore", CX, 150, 128, 34, COL_RED, cb_main, ID(G_RESTORE));
  showObj(cgRestore, false);
  cgSlider = lv_slider_create(scrCfg);
  lv_obj_set_size(cgSlider, 130, 10);
  lv_obj_align(cgSlider, LV_ALIGN_CENTER, 0, 150 - CY);
  lv_slider_set_range(cgSlider, 0, 100);
  lv_obj_set_style_bg_color(cgSlider, COL_AMBER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(cgSlider, COL_AMBER, LV_PART_KNOB);
  lv_obj_add_event_cb(cgSlider, cb_slider, LV_EVENT_VALUE_CHANGED, NULL);
  mkBtn(scrCfg, LV_SYMBOL_OK " Done", CX, 190, 96, 30, COL_GREEN, cb_main, ID(G_OK));
}

// ---------------------------------------------------------------------------
// Refresh functions
// ---------------------------------------------------------------------------
static void refreshOverview() {
  int m = curMethod();
  Recipe r = viewRecipe(m);
  Recipe& stored = recipeFor(m);
  int pos = 0, total = 0;
  for (int i = 0; i < store[m].count; i++) {
    if (store[m].r[i].cups == stored.cups) { if (i == recipeIdx[m]) pos = total; total++; }
  }
  char b[48];
  setTxt(ovTitle, METHOD_SHORT[m]);
  lv_obj_set_style_text_font(ovName, strlen(r.name) > 13 ? FONT_SM : FONT_MD, 0);
  setTxt(ovName, r.name);
  snprintf(b, sizeof(b), "%d CUP    %d/%d", twoCup ? 2 : 1, pos + 1, total);
  setTxt(ovSub, b);
  fmtG10(b, sizeof(b), r.coffeeX10); setTxt(ovCoffee, b);
  snprintf(b, sizeof(b), "%dg", r.waterG); setTxt(ovWater, b);
  fmtGrind(b, sizeof(b), r.grindX10);
  lv_obj_set_style_text_font(ovGrind, config.grindMode ? FONT_SM : FONT_MD, 0);
  lv_obj_set_width(ovGrind, 100);
  setTxt(ovGrind, b);
  fmtTemp(b, sizeof(b), r.tempX10); setTxt(ovTemp, b);
  fmtTime(b, sizeof(b), recipeTotalSecs(r)); setTxt(ovTime, b);
}
static void refreshEspSelect() {
  setTxt(esMode, espAuto ? "AUTO" : "MANUAL");
  setTxt(esHint, espAuto ? "tap for manual mode" : "tap for auto mode");
  showObj(esMsg, espAuto);
  showObj(esStartBtn, !espAuto);
  dEsp.ring = espAuto ? 3 : 0;
  dEsp.col = COL_AMBER;
  lv_obj_invalidate(oEsp);
  vibAbove = false;
}
static void showSelect() {
  bool keepSlot = (appState == APP_MENU || appState == APP_CONFIRM || appState == APP_EDIT);
  if (!keepSlot) methodSlot = physRot;       // otherwise the slot follows the physical orientation
  appState = APP_SELECT;
  lastActivity = millis();
  guardObj = NULL;
  clearZones();
  updateBatteryWidgets();
  if (curMethod() == METHOD_ESPRESSO) { refreshEspSelect(); loadScr(scrEsp); }
  else { fixSelection(curMethod()); refreshOverview(); loadScr(scrSel); }
}
static void openMenu(const char* msg) {
  char b[48];
  if (msg) snprintf(b, sizeof(b), "%s", msg);
  else snprintf(b, sizeof(b), "%s", recipeFor(curMethod()).name);
  setTxt(mnName, b);
  appState = APP_MENU;
  loadScr(scrMenu);
}

// "Learn vibration": measures the background level with the machine off, then the level with it running
static uint8_t  learnState = 0;         // 0 idle, 1 background (machine off), 2 measuring (machine on), 3 done, 4 failed
static uint32_t learnT0 = 0, learnLastSample = 0;
static float    learnIdleMax = 0, learnBuf[300];
static int      learnN = 0, learnResult = 0;

static void refreshConfig() {
  const int pg = CFG_ORDER[cfgPage];
  char cap[40], val[40];
  if (pg < 4) {
    snprintf(cap, sizeof(cap), "Brew %d  (%d deg)", pg + 1, pg * 90);
    snprintf(val, sizeof(val), "%s", METHOD_NAMES[config.assignedMethods[pg]]);
  } else if (pg == 4) {
    snprintf(cap, sizeof(cap), "Get ready time");
    if (config.getReadyDuration == 0) snprintf(val, sizeof(val), "Off");
    else snprintf(val, sizeof(val), "%d s", config.getReadyDuration);
  } else if (pg == 5) {
    snprintf(cap, sizeof(cap), "Vibration sensitivity");
    snprintf(val, sizeof(val), "%d%%", config.vibrationSensitivity);
  } else if (pg == 6) {
    snprintf(cap, sizeof(cap), "Sleep after");
    if (config.sleepTimeMinutes == 0) snprintf(val, sizeof(val), "Never");
    else snprintf(val, sizeof(val), "%d min", config.sleepTimeMinutes);
  } else if (pg == 7) {
    snprintf(cap, sizeof(cap), "Grind display");
    snprintf(val, sizeof(val), "%s", config.grindMode ? "Description" : "Number");
  } else if (pg == 8) {
    snprintf(cap, sizeof(cap), "Min grind setting");
    fmtGrindNum(val, sizeof(val), config.grindMinX10);
  } else if (pg == 9) {
    snprintf(cap, sizeof(cap), "Max grind setting");
    fmtGrindNum(val, sizeof(val), config.grindMaxX10);
  } else if (pg == 10) {
    snprintf(cap, sizeof(cap), "Temperature unit");
    snprintf(val, sizeof(val), "%s", config.tempUnit ? "Fahrenheit" : "Celsius");
  } else if (pg == 11) {
    snprintf(cap, sizeof(cap), "Restore recipes");
    snprintf(val, sizeof(val), "%s", restoreArmed ? "Are you sure?" : "Defaults");
  } else if (pg == 13) {
    snprintf(cap, sizeof(cap), "Screen brightness");
    snprintf(val, sizeof(val), "%d%%", config.brightness);
  } else if (pg == 14) {
    switch (learnState) {
      case 1:  snprintf(cap, sizeof(cap), "Keep machine OFF"); snprintf(val, sizeof(val), "Listening"); break;
      case 2:  snprintf(cap, sizeof(cap), "Run the machine NOW"); snprintf(val, sizeof(val), "Measuring"); break;
      case 3:  snprintf(cap, sizeof(cap), "Sensitivity set to"); snprintf(val, sizeof(val), "%d%%", learnResult); break;
      case 4:  snprintf(cap, sizeof(cap), "No vibration found"); snprintf(val, sizeof(val), "Try again"); break;
      default: snprintf(cap, sizeof(cap), "Learn vibration"); snprintf(val, sizeof(val), "On machine"); break;
    }
  } else {
    snprintf(cap, sizeof(cap), "Wi-Fi connect");
    snprintf(val, sizeof(val), "Phone / PC");
  }
  if (pg != 11) restoreArmed = false;
  if (pg != 14 && learnState) learnState = 0;
  setTxt(cgCap, cap);
  setTxt(cgVal, val);
  bool vib = (pg == 5), act = (pg == 11 || pg == 12 || pg == 14);       // act: pages with an action button instead of - / +
  showObj(cgMinus, !act); showObj(cgPlus, !act); showObj(cgRestore, act);
  if (act) {
    if (pg == 12) { btnText(cgRestore, "Start"); lv_obj_set_style_bg_color(cgRestore, COL_GREEN, 0); }
    else if (pg == 14) {
      btnText(cgRestore, learnState == 0 ? "Start" : (learnState == 3 ? "Again" : (learnState == 4 ? "Retry" : "...")));
      lv_obj_set_style_bg_color(cgRestore, COL_BLUE, 0);
    } else { btnText(cgRestore, restoreArmed ? "Confirm" : "Restore"); lv_obj_set_style_bg_color(cgRestore, COL_RED, 0); }
  }
  showObj(cgSlider, vib);
  guardObj = vib ? cgSlider : NULL;
  if (vib) lv_slider_set_value(cgSlider, config.vibrationSensitivity, LV_ANIM_OFF);
  lv_obj_align(cgVal, LV_ALIGN_CENTER, 0, (vib ? 96 : 104) - CY);
  clearZones();
  int px = (pg < 4 || pg == 6 || pg == 7 || pg == 10) ? 40 : 14;
  if (!act) addZone(68, vib ? 72 : 80, 172, vib ? 120 : 130, 100, px);
  dConf.dotIdx = cfgPage;
  lv_obj_invalidate(scrCfg);
}
static void adjustConfig(int dir, bool fine) {
  const int pg = CFG_ORDER[cfgPage];
  if (pg < 4) {
    config.assignedMethods[pg] = (config.assignedMethods[pg] + dir + METHOD_COUNT) % METHOD_COUNT;
  } else if (pg == 4) {
    config.getReadyDuration = (uint8_t)iclamp((int)config.getReadyDuration + dir, 0, 30);
  } else if (pg == 5) {
    config.vibrationSensitivity = (uint8_t)iclamp((int)config.vibrationSensitivity + dir * (fine ? 1 : 5), 0, 100);
  } else if (pg == 6) {
    int idx = 0;
    for (int i = 0; i < SLEEP_OPTION_COUNT; i++) if (SLEEP_OPTIONS[i] == config.sleepTimeMinutes) idx = i;
    idx = iclamp(idx + dir, 0, SLEEP_OPTION_COUNT - 1);
    config.sleepTimeMinutes = SLEEP_OPTIONS[idx];
  } else if (pg == 7) {
    config.grindMode ^= 1;
  } else if (pg == 8) {
    config.grindMinX10 = (uint16_t)iclamp((int)config.grindMinX10 + dir * (fine ? 1 : 10), 1, (int)config.grindMaxX10 - 10);
  } else if (pg == 9) {
    config.grindMaxX10 = (uint16_t)iclamp((int)config.grindMaxX10 + dir * (fine ? 1 : 10), (int)config.grindMinX10 + 10, 500);
  } else if (pg == 10) {
    config.tempUnit ^= 1;
  } else if (pg == 13) {
    config.brightness = (uint8_t)iclamp((int)config.brightness + dir * (fine ? 1 : 5), 10, 100);
    applyBacklight(config.brightness);                                   // live preview
  }
  refreshConfig();
}

static int cmpFloat(const void* a, const void* b) { float x = *(const float*)a, y = *(const float*)b; return x < y ? -1 : (x > y ? 1 : 0); }
static void learnStart() {
  learnState = 1; learnT0 = millis(); learnIdleMax = 0; learnN = 0; learnLastSample = 0;
  refreshConfig();
}
static void learnFinish() {
  qsort(learnBuf, learnN, sizeof(float), cmpFloat);
  float run35 = learnN > 0 ? learnBuf[(learnN * 35) / 100] : 0.0f;      // level the machine reaches most of the time
  if (learnN < 50 || run35 < fmaxf(6.0f, learnIdleMax * 3.0f)) { learnState = 4; refreshConfig(); return; }
  float thr = fmaxf(run35 * 0.4f, learnIdleMax * 2.0f + 1.0f);          // trigger well below the running level, well above the background
  thr = fminf(thr, run35 * 0.7f);
  learnResult = iclamp((int)lroundf(-31.25f * logf(thr / 100.0f)), 0, 100);   // inverse of vibThreshold()
  config.vibrationSensitivity = (uint8_t)learnResult;
  saveConfig();
  Serial.printf("[LEARN] background %.1f mg, running %.1f mg, threshold %.1f mg -> sensitivity %d%%\n", learnIdleMax, run35, thr, learnResult);
  learnState = 3;
  refreshConfig();
}
static void learnTick(uint32_t now) {
  if (learnState != 1 && learnState != 2) return;
  uint32_t el = now - learnT0;
  if (learnState == 1) {                                                 // 2 s with the machine off
    if (vibRms > learnIdleMax) learnIdleMax = vibRms;
    if (el >= 2000) { learnState = 2; learnT0 = now; learnN = 0; refreshConfig(); }
  } else {                                                               // 6 s with the machine running
    if (now - learnLastSample >= 20 && learnN < 300) { learnBuf[learnN++] = vibRms; learnLastSample = now; }
    if (el >= 6000) learnFinish();
  }
}
static void enterConfig() {
  cfgPage = 0;
  appState = APP_CONFIG;
  refreshConfig();
  loadScr(scrCfg);
}

