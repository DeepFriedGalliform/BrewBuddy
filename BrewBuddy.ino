/*
 * ============================================================================
 *  COFFEE BREW BUDDY  -  firmware v3
 *  Waveshare ESP32-S3 1.28" round 240x240 touch LCD (GC9A01 + CST816S + QMI8658)
 *  LVGL v9 + TFT_eSPI
 *
 *  FILES (all in this sketch folder):
 *    BrewBuddy.ino  this file: LVGL glue, setup() and loop()
 *    BB_Core.h      tunables, fonts, colours, data types, global state, helpers
 *    BB_Data.h      recipe table (edit the defaults here), settings, history, flash storage
 *    BB_Input.h     touch + gesture recogniser, IMU (orientation / vibration / shake)
 *    BB_Draw.h      rings, water animation, widget helpers, battery indicator
 *    BB_Screens.h   screens, callbacks, config pages
 *    BB_Editor.h    on-device recipe editor
 *    BB_Web.h       Wi-Fi hotspot + web server
 *    WebUI.h        the web page itself
 *    BB_Power.h     charging screen, battery sampling, charger detection
 *    BB_App.h       brew engine, per-frame update, gestures, rotation, standby
 *
 *  FEATURES
 *   - Rotate the device (0/90/180/270) to pick one of 4 configured methods;
 *     the display is always kept the right way up.
 *   - Espresso auto (vibration triggered) / manual (timer + laps)
 *   - Guided brews with timer ring, water fill/drain animation, flow rate,
 *     weight and valve state. Tap = pause/resume, swipe left = skip step,
 *     swipe right = repeat step / go back. Ring pulses on each step change
 *     and on the last 3 seconds of a countdown. Shake cancels.
 *   - 1-cup / 2-cup recipe table, on-device recipe editor, Wi-Fi + web page
 *     (recipes, settings, brew history with CSV export, backup / restore)
 *   - Remembers cup size, espresso mode and last recipe per method
 *   - Settings: method slots, get-ready time, brightness, vibration
 *     sensitivity (with auto-learn), sleep time, grind display / range,
 *     temperature unit, restore recipes, Wi-Fi connect
 *   - Battery indicator + charging screen; light-sleep standby that wakes on
 *     touch or when the device is picked up; screen dims before standby
 *
 *  Gestures are decoded in software from the raw CST816S touch points.
 * ============================================================================
 */
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <Preferences.h>
#include <math.h>
#include <stddef.h>
#include "lv_conf.h"
#include "CST816S.h"
#include "DEV_Config.h"
#include "QMI8658.h"
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <sys/time.h>
#include "WebUI.h"          // the web page (keep in the sketch folder)

// ---- sketch parts (each is included once, in this order) ----
#include "BB_Core.h"
#include "BB_Data.h"
#include "BB_Input.h"
#include "BB_Draw.h"
#include "BB_Screens.h"
#include "BB_Editor.h"
#include "BB_Web.h"
#include "BB_Power.h"
#include "BB_App.h"

// ---------------------------------------------------------------------------
// LVGL glue
// ---------------------------------------------------------------------------
void my_disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* pixel_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t*)pixel_map, w * h, true);
  tft.endWrite();
  lv_display_flush_ready(disp);
}
void my_touchpad_read(lv_indev_t* indev, lv_indev_data_t* data) {
  LV_UNUSED(indev);
  data->state = tp.lvPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  data->point.x = tp.lvX;
  data->point.y = tp.lvY;
}
void example_increase_lvgl_tick(void* arg) {
  LV_UNUSED(arg);
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

// ---------------------------------------------------------------------------
// Setup & loop
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  esp_reset_reason_t rr = esp_reset_reason();
  if (crashMagic != 0xB4B4C0DEUL) { crashMagic = 0xB4B4C0DEUL; crashCount = 0; }
  if (rr == ESP_RST_PANIC || rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT || rr == ESP_RST_BROWNOUT) {
    if (crashCount < 250) crashCount++;
  } else crashCount = 0;                           // power-on, reset button, USB reset, software restart, new upload: not a crash
  safeMode = (crashCount >= 2);                    // two crashes in a row: start with the risky features switched off
  Serial.printf("Reset reason %d, crash count %d%s\n", (int)rr, (int)crashCount, safeMode ? "  -> SAFE MODE (Wi-Fi/QR/battery off)" : "");
  pinMode(LCD_BL_PIN, OUTPUT);
  digitalWrite(LCD_BL_PIN, HIGH);

  lv_init();
  tft.begin();
  tft.setRotation(0);

  Wire.begin(DEV_SDA_PIN, DEV_SCL_PIN);
  touch.begin(RISING);
  Wire.setClock(400000);
  Wire.beginTransmission(CST816S_ADDRESS);      // keep the touch controller awake so polling always works
  Wire.write(0xFE); Wire.write(0x01);
  Wire.endTransmission();

  QMI8658_init();
  loadAll();
  applyBacklight(config.brightness);
  bootTs = millis();

  lv_display_t* disp = lv_display_create(SCREEN_W, SCREEN_H);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_timer_set_period(lv_display_get_refr_timer(disp), FRAME_MS);

  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_indev_set_long_press_time(indev, 60000);   // long press is handled by our own gesture engine

  createOverview();
  createEspSelect();
  createEspRun();
  createPrep();
  createReady();
  createBrew();
  createDone();
  createMenu();
  createConfirm();
  createConfig();
  createEditor();
  createConnect();
  createCharging();

  const esp_timer_create_args_t tickArgs = {
    .callback = &example_increase_lvgl_tick,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "lvgl_tick",
    .skip_unhandled_events = false
  };
  esp_timer_handle_t tickTimer = NULL;
  esp_timer_create(&tickArgs, &tickTimer);
  esp_timer_start_periodic(tickTimer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

  // Let the IMU settle, then pick the start-up orientation (and so the brew method) immediately
  appState = APP_SLEEP;                         // (suppresses live rotation handling during this block)
  uint32_t t0 = millis();
  while (millis() - t0 < 450) { imuTask(millis()); delay(2); }
  shakeFlag = false;
  applyRotation(orientationCandidate(), false);
  appState = APP_SELECT;
  batteryInit();
  showSelect();
  lastActivity = millis();
  Serial.println("BrewBuddy booted.");
}

void loop() {
  uint32_t now = millis();
  bool deepStandby = (LIGHT_SLEEP_ENABLE && appState == APP_SLEEP && !charging && !safeMode);
  if (!deepStandby) { touchTask(now); imuTask(now); }   // in light-sleep standby the wake sources replace polling
  batteryTask(now);

  if (batPlugEdge) {                               // charger plugged in: wake and show the charging screen
    batPlugEdge = false; chargeDismissed = false;
    if (appState == APP_SLEEP) wakeUp();
    else if (appState == APP_SELECT || appState == APP_DONE || appState == APP_PREP) showCharging();
  }
  if (batUnplugEdge) {
    batUnplugEdge = false; chargeDismissed = false;
    if (appState == APP_CHARGING) showSelect();
  }

  if (appState == APP_CONNECT) {                   // Wi-Fi mode: serve the web page, nothing else runs
    server.handleClient();
    lastActivity = now;
    gqCount = 0; shakeFlag = false;
    if ((int32_t)(millis() - connLastReq) > 600000) stopConnect();   // 10 minutes without a request
    lv_timer_handler();
    delay(2);
    return;
  }

  if (appState == APP_SLEEP) {
    if (deepStandby) { standbyStep(); return; }
    if (tp.down || gqCount > 0 || shakeFlag) wakeUp();
    else delay(20);
    return;
  }

  if (shakeFlag) {                               // shake cancels an active brew guide
    shakeFlag = false;
    if (appState == APP_READY || appState == APP_BREW) showSelect();
  }
  for (int i = 0; i < gqCount; i++) handleGesture(gq[i]);
  gqCount = 0;

  if (crashCount && millis() > 60000UL) crashCount = 0;   // ran for a minute without crashing
  now = millis();                                // fresh timestamp (state changes above may have stamped lastActivity later than 'now')
  updateApp(now);
  if (now < forceRedrawUntil) lv_obj_invalidate(lv_screen_active());

  if (edRebuild) { edRebuild = false; if (appState == APP_EDIT) edBuildPage(); }

  if (config.sleepTimeMinutes > 0) {
    int32_t idle = (int32_t)(now - lastActivity);                        // signed: never wraps
    int32_t limit = (int32_t)((uint32_t)config.sleepTimeMinutes * 60000UL);
    bool wantDim = idle > limit - 10000;                                 // dim the screen for the last 10 s before standby
    if (wantDim != dimmed) { dimmed = wantDim; applyBacklight(dimmed ? imax(4, config.brightness / 6) : config.brightness); }
    if (idle > limit) goSleep();
  }
  if (settingsSaveAt && (int32_t)(now - settingsSaveAt) >= 0) { settingsSaveAt = 0; flushSettings(); }

  lv_timer_handler();
  delay(1);
}
