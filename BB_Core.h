// BB_Core.h
// Tunables, fonts, colours, data types, global state and small helpers
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

// ---------------------------------------------------------------------------
// Tunables
// ---------------------------------------------------------------------------
#define SCREEN_W        240
#define SCREEN_H        240
#define CX              120
#define CY              120
#define FRAME_MS        16          // LVGL refresh period (~60 fps target)
#define LONG_PRESS_MS   5000        // hold 5 s to enter configuration
#define DOUBLE_TAP_MS   360
#define SWIPE_MIN_PX    30
#define MOVE_THRESH_PX  14
#define SHAKE_MG        1100.0f     // |dynamic acceleration| peak that counts as a shake
#define VIB_DEBUG       0           // 1 = print vibration RMS to Serial for tuning

#define MAX_STEPS       10
#define MAX_RECIPES     24
#define NAME_LEN        22

// ---------------------------------------------------------------------------
// Fonts (falls back gracefully if a size is not enabled in lv_conf.h).
// Recommended in lv_conf.h: LV_FONT_MONTSERRAT_12, 14, 20, 38, 48 = 1
// ---------------------------------------------------------------------------
#if LV_FONT_MONTSERRAT_12
#define FONT_XS (&lv_font_montserrat_12)
#else
#define FONT_XS (&lv_font_montserrat_14)
#endif
#define FONT_SM (&lv_font_montserrat_14)
#if LV_FONT_MONTSERRAT_20
#define FONT_MD (&lv_font_montserrat_20)
#elif LV_FONT_MONTSERRAT_18
#define FONT_MD (&lv_font_montserrat_18)
#elif LV_FONT_MONTSERRAT_16
#define FONT_MD (&lv_font_montserrat_16)
#else
#define FONT_MD (&lv_font_montserrat_14)
#endif
#if LV_FONT_MONTSERRAT_38
#define FONT_LG (&lv_font_montserrat_38)
#elif LV_FONT_MONTSERRAT_32
#define FONT_LG (&lv_font_montserrat_32)
#elif LV_FONT_MONTSERRAT_28
#define FONT_LG (&lv_font_montserrat_28)
#else
#define FONT_LG FONT_MD
#endif
#if LV_FONT_MONTSERRAT_48
#define FONT_XL (&lv_font_montserrat_48)
#else
#define FONT_XL FONT_LG
#endif

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
#define COL_BAND    lv_color_hex(0x3A2A1E)
#define COL_CREAM   lv_color_hex(0xF3E5D0)
#define COL_GOLD    lv_color_hex(0xE2B857)
#define COL_AMBER   lv_color_hex(0xE0A040)
#define COL_BLUE    lv_color_hex(0x2196F3)
#define COL_ORANGE  lv_color_hex(0xFF9800)
#define COL_GREEN   lv_color_hex(0x43A047)
#define COL_RED     lv_color_hex(0xE53935)
#define COL_GREY    lv_color_hex(0x4A4A4A)
#define COL_DIM     lv_color_hex(0x9E9E9E)
#define COL_TRACK   lv_color_hex(0x202020)
#define COL_WHITE   lv_color_white()

static lv_color_t buf[SCREEN_W * 60];   // 1/4 screen partial render buffer

TFT_eSPI tft = TFT_eSPI(SCREEN_W, SCREEN_H);
CST816S  touch(DEV_SDA_PIN, DEV_SCL_PIN, Touch_RST_PIN, Touch_INT_PIN);
static Preferences prefs;

// ---------------------------------------------------------------------------
// Data model
// ---------------------------------------------------------------------------
enum AppState {
  APP_SELECT, APP_ESP_RUN, APP_PREP, APP_READY, APP_BREW, APP_DONE,
  APP_CONFIG, APP_MENU, APP_CONFIRM, APP_EDIT, APP_SLEEP, APP_CHARGING, APP_CONNECT
};

enum BrewMethodType {
  METHOD_ESPRESSO, METHOD_POUR_OVER, METHOD_SWITCH, METHOD_CLEVER_DRIPPER,
  METHOD_AEROPRESS, METHOD_FRENCH_PRESS, METHOD_COUNT
};

enum StepType : uint8_t {
  STEP_ADD_WATER, STEP_ADD_COFFEE, STEP_BLOOM, STEP_WAIT, STEP_POUR,
  STEP_STIR, STEP_BREAK_CRUST, STEP_DRAWDOWN, STEP_PLUNGE, STEP_COUNT
};

enum Gesture { G_NONE, G_TAP, G_DOUBLE, G_LONG, G_UP, G_DOWN, G_LEFT, G_RIGHT, G_SCRUB };

struct Step   { uint8_t type; uint8_t valve; uint16_t secs; uint16_t water; };   // valve: 1 = open
struct Recipe {
  char     name[NAME_LEN];
  uint16_t grindX10;      // grind setting x10 (0.1 steps)
  uint16_t tempX10;       // water temperature in C x10
  uint16_t coffeeX10;     // grams x10 (1-cup)
  uint16_t waterG;        // grams (1-cup)
  uint8_t  nSteps;
  uint8_t  cups;          // 1 or 2 (each size is its own recipe)
  Step     steps[MAX_STEPS];
};
struct RecipeSet { uint8_t count; uint8_t pad; Recipe r[MAX_RECIPES]; };

struct DeviceConfig {
  uint8_t  magic = 0xB3;
  uint8_t  assignedMethods[4] = {METHOD_POUR_OVER, METHOD_SWITCH, METHOD_AEROPRESS, METHOD_ESPRESSO};
  uint8_t  getReadyDuration = 5;      // 0-30 s (0 = skip)
  uint8_t  vibrationSensitivity = 40; // 0-100 %
  uint16_t sleepTimeMinutes = 5;      // 0 = never
  uint8_t  grindMode = 0;             // 0 = number, 1 = description
  uint8_t  tempUnit = 0;              // 0 = C, 1 = F
  uint16_t grindMinX10 = 10;          // selectable grind range (x10)
  uint16_t grindMaxX10 = 110;
  // ---- added later: kept at the END so settings saved by older firmware still load
  uint8_t  brightness = 100;          // 10-100 %
  uint8_t  cupTwo = 0;                // remembered: 2-cup selected
  uint8_t  espAutoMode = 0;           // remembered: espresso auto (1) / manual (0)
  uint8_t  lastRecipe[METHOD_COUNT] = {0};   // remembered: last recipe used per method
};

struct Deco {
  bool    band = true;
  uint8_t ring = 0;            // 0 none, 1 red->green gradient, 2 solid, 3 spinner
  float   prog = 0;            // 0..1 (spinner: degrees)
  lv_color_t col;
  bool    water = false;
  float   level = 0;           // 0..1
  lv_color_t wcol;
  uint8_t wopa = 80;
  bool    tick = false;
  float   tickP = 0;
  uint8_t dots = 0, dotIdx = 0;
  float   flash = 0;           // 0..1 white pulse over the ring (step-change / countdown cue)
};

struct GestureEvent { Gesture g; int x, y; };
struct HistEntry {                    // one logged brew / espresso shot
  uint32_t epoch;                     // unix time (0 = clock was not set)
  uint16_t ds;                        // duration in tenths of a second
  uint16_t lapDs[4];                  // espresso lap times (tenths of a second)
  uint8_t  method, cups, kind, laps;  // kind: 0 guided brew, 1 espresso manual, 2 espresso auto
  char     name[NAME_LEN];
};
struct BattW { lv_obj_t *body, *fill, *lbl, *tip; };

static const char* const METHOD_NAMES[METHOD_COUNT] = {
  "Espresso", "Pour Over", "Switch", "Clever Dripper", "AeroPress", "French Press"
};
static const char* const METHOD_SHORT[METHOD_COUNT] = {
  "Espresso", "Pour Over", "Switch", "Clever Dripper", "AeroPress", "French Press"
};

struct StepInfo { const char* name; bool water; };
static const StepInfo STEP_INFO[STEP_COUNT] = {
  {"Add Water", true}, {"Add Coffee", false}, {"Bloom", true}, {"Wait", false}, {"Pour", true},
  {"Gentle Stir", false}, {"Break Crust", false}, {"Drawdown", false}, {"Plunge", false}
};
#define SB(x) (1u << (x))
static const uint16_t METHOD_STEP_MASK[METHOD_COUNT] = {
  0,
  SB(STEP_BLOOM) | SB(STEP_WAIT) | SB(STEP_POUR) | SB(STEP_STIR) | SB(STEP_DRAWDOWN),
  SB(STEP_ADD_WATER) | SB(STEP_ADD_COFFEE) | SB(STEP_BLOOM) | SB(STEP_WAIT) | SB(STEP_POUR) | SB(STEP_STIR) | SB(STEP_BREAK_CRUST) | SB(STEP_DRAWDOWN),
  SB(STEP_ADD_WATER) | SB(STEP_ADD_COFFEE) | SB(STEP_BLOOM) | SB(STEP_WAIT) | SB(STEP_POUR) | SB(STEP_STIR) | SB(STEP_BREAK_CRUST) | SB(STEP_DRAWDOWN),
  SB(STEP_ADD_WATER) | SB(STEP_ADD_COFFEE) | SB(STEP_BLOOM) | SB(STEP_WAIT) | SB(STEP_POUR) | SB(STEP_STIR) | SB(STEP_PLUNGE),
  SB(STEP_ADD_WATER) | SB(STEP_ADD_COFFEE) | SB(STEP_WAIT) | SB(STEP_STIR) | SB(STEP_BREAK_CRUST) | SB(STEP_PLUNGE)
};
static const char* const NAME_PRESETS[] = {
  "Daily", "Light Roast", "Medium Roast", "Dark Roast", "Sweet", "Bright", "Strong", "Weekend", "Competition", "Custom"
};
#define NAME_PRESET_COUNT 10
static const uint16_t SLEEP_OPTIONS[] = {0, 1, 2, 5, 10, 15, 30};
#define SLEEP_OPTION_COUNT 7

static bool hasValve(int m) {
  return m == METHOD_SWITCH || m == METHOD_CLEVER_DRIPPER;
}

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static DeviceConfig config;
static RecipeSet    store[METHOD_COUNT];
static uint8_t      recipeIdx[METHOD_COUNT] = {0};
static bool         twoCup = false;
static bool         espAuto = false;      // default: manual (tap to switch to auto)

static AppState appState = APP_SELECT;
static int  physRot = 0;                // physical orientation index 0..3 (selects the method slot)
static int  activeScreenRotation = 0;   // TFT rotation used to keep the UI upright
static int  methodSlot = 0;             // slot (0..3) that is currently selected
static int  activeMethod = METHOD_POUR_OVER;   // locked once Prepare is pressed

RTC_DATA_ATTR static uint32_t crashMagic = 0;   // marks crashCount as valid (RTC memory holds random data after a fresh flash)
RTC_DATA_ATTR static uint8_t crashCount = 0;   // consecutive crash/watchdog/brown-out resets (kept across resets)
static bool safeMode = false;                // set after repeated crashes: Wi-Fi, QR and battery features are skipped
static uint32_t bootTs = 0, lastActivity = 0, lastRotChange = 0, ignoreTouchUntil = 0;

static int curMethod() { return config.assignedMethods[methodSlot]; }
static Recipe& recipeFor(int m) {
  RecipeSet& s = store[m];
  if (recipeIdx[m] >= s.count) recipeIdx[m] = 0;
  return s.r[recipeIdx[m]];
}

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------
static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }
static int iclamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static float fclampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void fmtTime(char* b, size_t n, int secs) { snprintf(b, n, "%02d:%02d", secs / 60, secs % 60); }
static void fmtG10(char* b, size_t n, int x10) {
  if (x10 % 10 == 0) snprintf(b, n, "%dg", x10 / 10);
  else snprintf(b, n, "%d.%dg", x10 / 10, x10 % 10);
}
static const char* const GRIND_NAMES[6] = {"Fine", "Med-Fine", "Med", "Med-Coarse", "Coarse", "Very Coarse"};
static int grindIdxFor(int v) {
  int lo = config.grindMinX10, hi = config.grindMaxX10;
  if (hi <= lo) return 0;
  return iclamp(((v - lo) * 5 + (hi - lo) / 2) / (hi - lo), 0, 5);
}
static int grindValForIdx(int i) { return config.grindMinX10 + (config.grindMaxX10 - config.grindMinX10) * i / 5; }
static void fmtGrindNum(char* b, size_t n, int x10) {
  if (x10 % 10 == 0) snprintf(b, n, "%d", x10 / 10); else snprintf(b, n, "%d.%d", x10 / 10, x10 % 10);
}
static void fmtGrind(char* b, size_t n, int x10) {
  if (config.grindMode == 1) snprintf(b, n, "%s", GRIND_NAMES[grindIdxFor(x10)]);
  else fmtGrindNum(b, n, x10);
}
static int tempDisp(int x10) { return config.tempUnit ? (int)lroundf(x10 * 0.18f + 32.0f) : (int)lroundf(x10 / 10.0f); }
static int tempFromDisp(int v) { return config.tempUnit ? (int)lroundf((v - 32) * 50.0f / 9.0f) : v * 10; }
static void fmtTemp(char* b, size_t n, int x10) { snprintf(b, n, "%d%c", tempDisp(x10), config.tempUnit ? 'F' : 'C'); }
static int recipeTotalSecs(const Recipe& r) {
  int t = 0;
  for (int i = 0; i < r.nSteps; i++) t += r.steps[i].secs;
  return t;
}
static int recipeSumWater(const Recipe& r) {
  int w = 0;
  for (int i = 0; i < r.nSteps; i++) if (STEP_INFO[r.steps[i].type].water) w += r.steps[i].water;
  return w;
}
// Scale a recipe to a different cup count (only used when no recipe of that size exists)
static Recipe scaledRecipe(const Recipe& r, int want) {
  Recipe o = r;
  int have = r.cups < 1 ? 1 : r.cups;
  if (have != want) {
    o.coffeeX10 = r.coffeeX10 * want / have;
    o.waterG = r.waterG * want / have;
    for (int i = 0; i < o.nSteps; i++) o.steps[i].water = r.steps[i].water * want / have;
  }
  o.cups = want;
  return o;
}
// The recipe as it will be shown / brewed for the current cup size
static Recipe viewRecipe(int m) { return scaledRecipe(recipeFor(m), twoCup ? 2 : 1); }
// Make sure the selected recipe matches the current cup size when possible (same name first)
static void fixSelection(int m) {
  if (m < 1 || m >= METHOD_COUNT) return;
  RecipeSet& st = store[m];
  int want = twoCup ? 2 : 1;
  if (recipeIdx[m] >= st.count) recipeIdx[m] = 0;
  if (st.r[recipeIdx[m]].cups == want) return;
  for (int j = 0; j < st.count; j++) if (st.r[j].cups == want && strcmp(st.r[j].name, st.r[recipeIdx[m]].name) == 0) { recipeIdx[m] = j; return; }
  for (int j = 0; j < st.count; j++) if (st.r[j].cups == want) { recipeIdx[m] = j; return; }
}
static void stepRecipe(int m, int dir) {          // next / previous recipe of the current cup size
  RecipeSet& st = store[m];
  int want = twoCup ? 2 : 1;
  for (int k = 1; k <= st.count; k++) {
    int j = ((int)recipeIdx[m] + dir * k % st.count + st.count) % st.count;
    if (st.r[j].cups == want) { recipeIdx[m] = j; return; }
  }
}
static void toggleCups(int m) {                   // 1-cup <-> 2-cup, keeping the same recipe name if it exists
  RecipeSet& st = store[m];
  int want = twoCup ? 1 : 2;
  for (int j = 0; j < st.count; j++) if (st.r[j].cups == want && strcmp(st.r[j].name, st.r[recipeIdx[m]].name) == 0) { recipeIdx[m] = j; twoCup = !twoCup; return; }
  for (int j = 0; j < st.count; j++) if (st.r[j].cups == want) { recipeIdx[m] = j; twoCup = !twoCup; return; }
  twoCup = !twoCup;                               // no recipe of that size: show a scaled copy
}
// Is the (virtual) valve open during this step? Drives the drain animation.
static bool stepValveOpen(int m, const Step& s) {
  if (hasValve(m)) return s.valve != 0;
  if (m == METHOD_POUR_OVER) return true;
  return s.type == STEP_DRAWDOWN || s.type == STEP_PLUNGE;
}

