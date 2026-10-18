// BB_Data.h
// Recipe table, saved settings, brew history and flash (NVS) persistence
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

// ---------------------------------------------------------------------------
// Default recipes + persistence (NVS)
// ---------------------------------------------------------------------------
// ===========================================================================
//  RECIPE TABLE  -  built-in recipes. Edit this table to change the defaults.
//  Step = {type, valve (1 = open, 0 = closed), seconds, water added in grams}
//  Each row: {method, name, cups, coffee g x10, grind x10, temp C, steps}
//  Water for a recipe = sum of its step weights. 1-cup and 2-cup are separate rows.
//  (Recipes you save on the device replace the table for that method - see loadAll)
// ===========================================================================
static const Step T_PO1[] = { {STEP_BLOOM,1,10,40}, {STEP_WAIT,1,25,0}, {STEP_POUR,1,20,100}, {STEP_WAIT,1,30,0}, {STEP_POUR,1,20,100}, {STEP_DRAWDOWN,1,60,0} };
static const Step T_PO2[] = { {STEP_BLOOM,1,15,90}, {STEP_WAIT,1,30,0}, {STEP_POUR,1,30,190}, {STEP_WAIT,1,30,0}, {STEP_POUR,1,30,200}, {STEP_DRAWDOWN,1,75,0} };
static const Step T_SW_IMM[] = { {STEP_BLOOM,0,10,40}, {STEP_WAIT,0,25,0}, {STEP_POUR,0,25,200}, {STEP_WAIT,0,60,0}, {STEP_DRAWDOWN,1,60,0} };
static const Step T_HY_CLOSED[] = { {STEP_BLOOM,0,10,40}, {STEP_WAIT,0,25,0}, {STEP_POUR,1,20,100}, {STEP_WAIT,1,30,0}, {STEP_POUR,1,20,100}, {STEP_DRAWDOWN,1,60,0} };
static const Step T_HY_OPEN[] = { {STEP_BLOOM,1,10,40}, {STEP_WAIT,1,25,0}, {STEP_POUR,0,25,200}, {STEP_WAIT,0,60,0}, {STEP_DRAWDOWN,1,60,0} };
static const Step T_CL1[] = { {STEP_ADD_WATER,0,15,120}, {STEP_ADD_COFFEE,0,15,0}, {STEP_ADD_WATER,0,15,120}, {STEP_STIR,0,15,0}, {STEP_WAIT,0,105,0}, {STEP_BREAK_CRUST,0,15,0}, {STEP_DRAWDOWN,1,60,0} };
static const Step T_CL2[] = { {STEP_ADD_WATER,0,15,240}, {STEP_ADD_COFFEE,0,15,0}, {STEP_ADD_WATER,0,15,240}, {STEP_STIR,0,15,0}, {STEP_WAIT,0,125,0}, {STEP_BREAK_CRUST,0,15,0}, {STEP_DRAWDOWN,1,80,0} };
static const Step T_AERO[] = { {STEP_ADD_WATER,0,15,225}, {STEP_ADD_COFFEE,0,15,0}, {STEP_STIR,0,10,0}, {STEP_WAIT,0,90,0}, {STEP_PLUNGE,1,20,0} };
static const Step T_FR1[] = { {STEP_ADD_WATER,0,15,255}, {STEP_ADD_COFFEE,0,15,0}, {STEP_STIR,0,10,0}, {STEP_WAIT,0,120,0}, {STEP_BREAK_CRUST,0,15,0}, {STEP_WAIT,0,120,0}, {STEP_PLUNGE,0,20,0} };
static const Step T_FR2[] = { {STEP_ADD_WATER,0,15,510}, {STEP_ADD_COFFEE,0,15,0}, {STEP_STIR,0,10,0}, {STEP_WAIT,0,150,0}, {STEP_BREAK_CRUST,0,15,0}, {STEP_WAIT,0,150,0}, {STEP_PLUNGE,0,20,0} };

struct TblRow { uint8_t method; const char* name; uint8_t cups; uint16_t coffeeX10; uint16_t grindX10; uint8_t tempC; const Step* steps; uint8_t nSteps; };
#define NS(a) ((uint8_t)(sizeof(a) / sizeof(a[0])))
static const TblRow RECIPE_TABLE[] = {
  {METHOD_POUR_OVER, "Light Roast", 1, 150, 55, 96, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Med/Light Roast", 1, 150, 58, 93, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Medium Roast", 1, 150, 60, 91, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Dark Roast", 1, 150, 65, 88, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "African Washed", 1, 150, 52, 96, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Central American", 1, 150, 60, 92, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Natural Process", 1, 150, 58, 90, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Sumatran / Pacific", 1, 150, 65, 88, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Speciality Light", 1, 150, 55, 93, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Fruit & Floral", 1, 150, 52, 94, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Chocolate & Nutty", 1, 150, 60, 92, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Dark & Bold", 1, 150, 65, 88, T_PO1, NS(T_PO1)},
  {METHOD_POUR_OVER, "Light Roast", 2, 300, 65, 96, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Med/Light Roast", 2, 300, 68, 93, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Medium Roast", 2, 300, 70, 91, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Dark Roast", 2, 300, 75, 88, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "African Washed", 2, 300, 62, 96, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Central American", 2, 300, 70, 92, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Natural Process", 2, 300, 68, 90, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Sumatran / Pacific", 2, 300, 75, 88, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Speciality Light", 2, 300, 65, 93, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Fruit & Floral", 2, 300, 62, 94, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Chocolate & Nutty", 2, 300, 70, 92, T_PO2, NS(T_PO2)},
  {METHOD_POUR_OVER, "Dark & Bold", 2, 300, 75, 88, T_PO2, NS(T_PO2)},
  {METHOD_SWITCH, "Full Immersion", 1, 150, 70, 94, T_SW_IMM, NS(T_SW_IMM)},
  {METHOD_SWITCH, "Full Pour Over", 1, 150, 60, 94, T_PO1, NS(T_PO1)},
  {METHOD_SWITCH, "Hybrid - Closed Bloom", 1, 150, 60, 94, T_HY_CLOSED, NS(T_HY_CLOSED)},
  {METHOD_SWITCH, "Hybrid - Open Bloom", 1, 150, 60, 94, T_HY_OPEN, NS(T_HY_OPEN)},
  {METHOD_CLEVER_DRIPPER, "Light Roast", 1, 150, 65, 96, T_CL1, NS(T_CL1)},
  {METHOD_CLEVER_DRIPPER, "Med/Light Roast", 1, 150, 68, 93, T_CL1, NS(T_CL1)},
  {METHOD_CLEVER_DRIPPER, "Medium Roast", 1, 150, 70, 91, T_CL1, NS(T_CL1)},
  {METHOD_CLEVER_DRIPPER, "Dark Roast", 1, 150, 75, 88, T_CL1, NS(T_CL1)},
  {METHOD_CLEVER_DRIPPER, "Light Roast", 2, 300, 75, 96, T_CL2, NS(T_CL2)},
  {METHOD_CLEVER_DRIPPER, "Med/Light Roast", 2, 300, 78, 93, T_CL2, NS(T_CL2)},
  {METHOD_CLEVER_DRIPPER, "Medium Roast", 2, 300, 80, 91, T_CL2, NS(T_CL2)},
  {METHOD_CLEVER_DRIPPER, "Dark Roast", 2, 300, 85, 88, T_CL2, NS(T_CL2)},
  {METHOD_AEROPRESS, "Light Roast", 1, 150, 65, 96, T_AERO, NS(T_AERO)},
  {METHOD_AEROPRESS, "Med/Light Roast", 1, 150, 68, 93, T_AERO, NS(T_AERO)},
  {METHOD_AEROPRESS, "Medium Roast", 1, 150, 70, 91, T_AERO, NS(T_AERO)},
  {METHOD_AEROPRESS, "Dark Roast", 1, 150, 75, 88, T_AERO, NS(T_AERO)},
  {METHOD_FRENCH_PRESS, "Light Roast", 1, 170, 90, 96, T_FR1, NS(T_FR1)},
  {METHOD_FRENCH_PRESS, "Med/Light Roast", 1, 170, 93, 93, T_FR1, NS(T_FR1)},
  {METHOD_FRENCH_PRESS, "Medium Roast", 1, 170, 95, 91, T_FR1, NS(T_FR1)},
  {METHOD_FRENCH_PRESS, "Dark Roast", 1, 170, 100, 88, T_FR1, NS(T_FR1)},
  {METHOD_FRENCH_PRESS, "Light Roast", 2, 340, 95, 96, T_FR2, NS(T_FR2)},
  {METHOD_FRENCH_PRESS, "Med/Light Roast", 2, 340, 98, 93, T_FR2, NS(T_FR2)},
  {METHOD_FRENCH_PRESS, "Medium Roast", 2, 340, 100, 91, T_FR2, NS(T_FR2)},
  {METHOD_FRENCH_PRESS, "Dark Roast", 2, 340, 105, 88, T_FR2, NS(T_FR2)},
};
#define RECIPE_TABLE_ROWS ((int)(sizeof(RECIPE_TABLE) / sizeof(RECIPE_TABLE[0])))

static void buildDefaults(int m) {
  RecipeSet& st = store[m];
  memset(&st, 0, sizeof(st));
  for (int i = 0; i < RECIPE_TABLE_ROWS && st.count < MAX_RECIPES; i++) {
    const TblRow& row = RECIPE_TABLE[i];
    if (row.method != m) continue;
    Recipe& r = st.r[st.count++];
    memset(&r, 0, sizeof(r));
    strncpy(r.name, row.name, NAME_LEN - 1);
    r.cups = row.cups; r.coffeeX10 = row.coffeeX10; r.grindX10 = row.grindX10; r.tempX10 = row.tempC * 10;
    r.nSteps = imin(row.nSteps, MAX_STEPS);
    for (int k = 0; k < r.nSteps; k++) r.steps[k] = row.steps[k];
    r.waterG = recipeSumWater(r);
  }
  if (st.count == 0) {                       // safety net: never leave a method empty
    Recipe& r = st.r[st.count++];
    memset(&r, 0, sizeof(r));
    strncpy(r.name, "Basic", NAME_LEN - 1);
    r.cups = 1; r.coffeeX10 = 150; r.grindX10 = 60; r.tempX10 = 940; r.nSteps = 2;
    r.steps[0].type = STEP_ADD_WATER; r.steps[0].secs = 15; r.steps[0].water = 240;
    r.steps[1].type = STEP_WAIT; r.steps[1].secs = 120;
    r.waterG = 240;
  }
}
static bool setValid(const RecipeSet& s) {
  if (s.count < 1 || s.count > MAX_RECIPES) return false;
  for (int i = 0; i < s.count; i++) {
    if (s.r[i].nSteps < 1 || s.r[i].nSteps > MAX_STEPS || s.r[i].cups < 1 || s.r[i].cups > 2) return false;
    for (int k = 0; k < s.r[i].nSteps; k++) if (s.r[i].steps[k].type >= STEP_COUNT) return false;
  }
  return true;
}
static void saveConfig() { prefs.putBytes("cfg", &config, sizeof(config)); }
static void applyBacklight(int pct) { analogWrite(LCD_BL_PIN, iclamp(pct, 0, 100) * 255 / 100); }

// "Remember my last settings": cup size, espresso mode and last recipe per method are saved a moment after they change
static uint32_t settingsSaveAt = 0;
static void queueSettingsSave() { settingsSaveAt = millis() + 2500; if (!settingsSaveAt) settingsSaveAt = 1; }
static void flushSettings() {
  for (int m = 0; m < METHOD_COUNT; m++) config.lastRecipe[m] = recipeIdx[m];
  config.cupTwo = twoCup ? 1 : 0;
  config.espAutoMode = espAuto ? 1 : 0;
  saveConfig();
}

// Brew history: ring buffer of the last 20 brews / shots, kept in flash
#define HIST_MAX 20
static HistEntry hist[HIST_MAX];
static uint8_t histCount = 0, histHead = 0;          // histHead = next slot to write
static void histSave() {
  static uint8_t buf[2 + HIST_MAX * sizeof(HistEntry)];
  buf[0] = histCount; buf[1] = histHead;
  memcpy(buf + 2, hist, sizeof(hist));
  prefs.putBytes("hist", buf, sizeof(buf));
}
static void histLoad() {
  static uint8_t buf[2 + HIST_MAX * sizeof(HistEntry)];
  if (prefs.getBytesLength("hist") != sizeof(buf)) return;
  prefs.getBytes("hist", buf, sizeof(buf));
  if (buf[0] > HIST_MAX || buf[1] >= HIST_MAX) return;
  histCount = buf[0]; histHead = buf[1];
  memcpy(hist, buf + 2, sizeof(hist));
}
static void histClear() { histCount = 0; histHead = 0; memset(hist, 0, sizeof(hist)); histSave(); }
static void histAdd(int method, const char* name, int cups, int kind, uint32_t ds, const uint32_t* lapMs, int nLaps) {
  HistEntry& h = hist[histHead];
  memset(&h, 0, sizeof(h));
  time_t t = time(NULL);
  h.epoch = t > 1700000000 ? (uint32_t)t : 0;       // only trust the clock once the web page has set it
  h.ds = (uint16_t)imin((int)ds, 65535);
  h.method = (uint8_t)method; h.cups = (uint8_t)cups; h.kind = (uint8_t)kind;
  strncpy(h.name, name, NAME_LEN - 1);
  for (int i = 0; i < nLaps && i < 4; i++) { h.lapDs[i] = (uint16_t)imin((int)(lapMs[i] / 100), 65535); h.laps++; }
  histHead = (histHead + 1) % HIST_MAX;
  if (histCount < HIST_MAX) histCount++;
  histSave();
}
static void saveRecipes(int m) {
  char k[8]; snprintf(k, sizeof(k), "t%d", m);
  prefs.putBytes(k, &store[m], offsetof(RecipeSet, r) + store[m].count * sizeof(Recipe));   // variable length
}
static void loadAll() {
  prefs.begin("brewbuddy", false);
  uint8_t dver = prefs.getUChar("dver", 0);     // data version (recipe / method layout)
  bool loadedCfg = false;
  size_t cfgLen = prefs.getBytesLength("cfg");
  if (cfgLen >= offsetof(DeviceConfig, brightness) && cfgLen <= sizeof(DeviceConfig)) {
    DeviceConfig tmp;                              // fields added later keep their defaults when an older, shorter blob is loaded
    prefs.getBytes("cfg", &tmp, cfgLen);
    if (tmp.magic == 0xB3) { config = tmp; loadedCfg = true; }
  }
  if (loadedCfg && dver < 4) {                   // method list changed (Switch Hybrid + Immersion merged): remap saved slots
    static const uint8_t remap[7] = {0, 1, 2, 2, 3, 4, 5};
    for (int i = 0; i < 4; i++) config.assignedMethods[i] = config.assignedMethods[i] < 7 ? remap[config.assignedMethods[i]] : 1;
    saveConfig();
  }
  for (int i = 0; i < 4; i++) if (config.assignedMethods[i] >= METHOD_COUNT) config.assignedMethods[i] = METHOD_POUR_OVER;
  config.getReadyDuration = imin(config.getReadyDuration, 30);
  config.vibrationSensitivity = imin(config.vibrationSensitivity, 100);
  config.grindMode = config.grindMode ? 1 : 0;
  config.tempUnit = config.tempUnit ? 1 : 0;
  config.brightness = (uint8_t)iclamp(config.brightness, 10, 100);
  config.grindMinX10 = iclamp(config.grindMinX10, 1, 490);
  config.grindMaxX10 = iclamp(config.grindMaxX10, config.grindMinX10 + 10, 500);
  if (dver < 2) { config.grindMinX10 = 10; config.grindMaxX10 = 110; saveConfig(); }
  if (dver < 4) {                                // recipe storage is indexed by method: clear anything saved by older layouts
    for (int m = 1; m < 8; m++) {
      char k[8]; snprintf(k, sizeof(k), "r%d", m); prefs.remove(k);
      snprintf(k, sizeof(k), "t%d", m); prefs.remove(k);
    }
  }
  for (int m = 1; m < METHOD_COUNT; m++) {
    char k[8]; snprintf(k, sizeof(k), "t%d", m);
    bool ok = false;
    size_t len = prefs.getBytesLength(k);
    size_t hdr = offsetof(RecipeSet, r);
    if (len >= hdr + sizeof(Recipe) && len <= sizeof(RecipeSet)) {
      memset(&store[m], 0, sizeof(RecipeSet));
      prefs.getBytes(k, &store[m], len);
      ok = setValid(store[m]) && len == hdr + store[m].count * sizeof(Recipe);
    }
    if (!ok) buildDefaults(m);                   // no saved edits for this method: use the recipe table
  }
  prefs.putUChar("dver", 4);
  histLoad();
  twoCup = config.cupTwo != 0;                     // restore the remembered selections
  espAuto = config.espAutoMode != 0;
  for (int m = 1; m < METHOD_COUNT; m++) {
    recipeIdx[m] = config.lastRecipe[m] < store[m].count ? config.lastRecipe[m] : 0;
    fixSelection(m);
  }
}

