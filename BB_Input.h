// BB_Input.h
// Touch polling + gesture recogniser (tap / double tap / long press / swipe / scrub) and the IMU (orientation, vibration, shake)
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

// ---------------------------------------------------------------------------
// Touch: raw CST816S polling + software gesture recogniser
// ---------------------------------------------------------------------------
struct TouchTrack {
  bool down = false;
  int  x = 0, y = 0, x0 = 0, y0 = 0;
  uint32_t t0 = 0;
  bool moved = false, longFired = false;
  bool suppress = false;     // true when the current touch was a swipe / long press (buttons ignore it)
  bool noGesture = false;    // touch started on a draggable widget (slider)
  bool lvPressed = false;
  int  lvX = 0, lvY = 0;
  uint8_t fails = 0;
  int zone = -1, scrubRef = 0;
} tp;

static GestureEvent gq[6];
static int gqCount = 0;
static uint32_t lastTapT = 0;
static int lastTapX = 0, lastTapY = 0;
static lv_obj_t* guardObj = NULL;     // widgets that handle their own horizontal drag

static void pushGesture(Gesture g, int x, int y) {
  if (gqCount < 6) { gq[gqCount].g = g; gq[gqCount].x = x; gq[gqCount].y = y; gqCount++; }
}
static bool pointInObj(lv_obj_t* o, int x, int y) {
  if (!o) return false;
  lv_area_t a; lv_obj_get_coords(o, &a);
  return x >= a.x1 && x <= a.x2 && y >= a.y1 && y <= a.y2;
}
static bool clickOK() { return !tp.suppress; }

// Horizontal "scrub" zones: drag left/right over a value to change it
struct ScrubZone { int x1, y1, x2, y2, id, px; };
static ScrubZone zones[4];
static int zoneCount = 0;
static void clearZones() { zoneCount = 0; }
static void addZone(int x1, int y1, int x2, int y2, int id, int px) {
  if (zoneCount < 4) { zones[zoneCount].x1 = x1; zones[zoneCount].y1 = y1; zones[zoneCount].x2 = x2; zones[zoneCount].y2 = y2; zones[zoneCount].id = id; zones[zoneCount].px = px; zoneCount++; }
}
static void pushScrub(int id, int steps) {
  if (gqCount > 0 && gq[gqCount - 1].g == G_SCRUB && gq[gqCount - 1].x == id) { gq[gqCount - 1].y += steps; return; }
  pushGesture(G_SCRUB, id, steps);
}

static bool readTouchRegs(uint8_t* b) {
  Wire.beginTransmission(CST816S_ADDRESS);
  Wire.write(0x01);
  if (Wire.endTransmission(true) != 0) return false;
  if (Wire.requestFrom((uint8_t)CST816S_ADDRESS, (uint8_t)6) != 6) return false;
  for (int i = 0; i < 6; i++) b[i] = Wire.read();
  return true;
}
// Raw panel coordinates -> upright UI coordinates for the current TFT rotation
static void mapTouch(int rx, int ry, int& x, int& y) {
  switch (activeScreenRotation) {
    case 0: x = rx;                 y = ry;                 break;
    case 1: x = ry;                 y = SCREEN_W - 1 - rx;  break;
    case 2: x = SCREEN_W - 1 - rx;  y = SCREEN_H - 1 - ry;  break;
    default: x = SCREEN_H - 1 - ry; y = rx;                 break;
  }
}

static void touchTask(uint32_t now) {
  static uint32_t lastPoll = 0;
  uint32_t interval = (appState == APP_SLEEP) ? 40 : (tp.down ? 8 : 15);
  if (now - lastPoll < interval) return;
  lastPoll = now;

  uint8_t raw[6];
  bool pressed = false;
  int x = tp.x, y = tp.y;
  if (readTouchRegs(raw)) {
    tp.fails = 0;
    if (raw[1] > 0) {
      pressed = true;
      int rx = ((raw[2] & 0x0F) << 8) | raw[3];
      int ry = ((raw[4] & 0x0F) << 8) | raw[5];
      mapTouch(rx, ry, x, y);
    }
  } else {
    if (tp.down && ++tp.fails < 4) return;   // tolerate a glitchy read mid-touch
  }
  if (now < ignoreTouchUntil) pressed = false;

  if (pressed && !tp.down) {                 // ---- finger down
    tp.down = true; tp.x0 = tp.x = x; tp.y0 = tp.y = y; tp.t0 = now;
    tp.moved = false; tp.longFired = false; tp.suppress = false;
    tp.noGesture = pointInObj(guardObj, x, y);
    tp.zone = -1;
    for (int i = 0; i < zoneCount; i++) {
      if (x >= zones[i].x1 && x <= zones[i].x2 && y >= zones[i].y1 && y <= zones[i].y2) {
        tp.zone = i; tp.noGesture = true; tp.scrubRef = x; break;
      }
    }
    lastActivity = now;
  } else if (pressed && tp.down) {           // ---- finger held / moving
    tp.x = x; tp.y = y;
    int dist = imax(abs(x - tp.x0), abs(y - tp.y0));
    if (dist > MOVE_THRESH_PX) { tp.moved = true; tp.suppress = true; }
    if (tp.zone >= 0 && tp.zone < zoneCount) {
      int steps = (x - tp.scrubRef) / zones[tp.zone].px;
      if (steps != 0) { tp.scrubRef += steps * zones[tp.zone].px; tp.suppress = true; pushScrub(zones[tp.zone].id, steps); }
    }
    if (!tp.noGesture && !tp.moved && !tp.longFired && now - tp.t0 >= LONG_PRESS_MS) {
      tp.longFired = true; tp.suppress = true;
      pushGesture(G_LONG, tp.x, tp.y);
    }
    lastActivity = now;
  } else if (!pressed && tp.down) {          // ---- finger up
    tp.down = false;
    if (!tp.noGesture) {
      int dx = tp.x - tp.x0, dy = tp.y - tp.y0;
      if (tp.moved) {
        if (imax(abs(dx), abs(dy)) >= SWIPE_MIN_PX) {
          if (abs(dx) > abs(dy)) pushGesture(dx < 0 ? G_LEFT : G_RIGHT, tp.x, tp.y);
          else                   pushGesture(dy < 0 ? G_UP : G_DOWN, tp.x, tp.y);
        }
      } else if (!tp.longFired) {
        if (lastTapT && now - lastTapT < DOUBLE_TAP_MS &&
            imax(abs(tp.x - lastTapX), abs(tp.y - lastTapY)) < 40) {
          pushGesture(G_DOUBLE, tp.x, tp.y);
          lastTapT = 0;
        } else {
          pushGesture(G_TAP, tp.x, tp.y);
          lastTapT = now; lastTapX = tp.x; lastTapY = tp.y;
        }
      }
    }
    lastActivity = now;
  }
  tp.lvPressed = tp.down;
  tp.lvX = tp.x; tp.lvY = tp.y;
}

// ---------------------------------------------------------------------------
// IMU: orientation, vibration (espresso auto) and shake
// ---------------------------------------------------------------------------
static float lpMag = 1000.0f, vibE = 0.0f, vibRms = 0.0f, axF = 0.0f, ayF = 0.0f;
static bool  imuPrimed = false;
static uint32_t shakeT[3] = {0, 0, 0};
static int shakeIdx = 0;
static uint32_t lastPeakT = 0, shakeBlockUntil = 0;
static bool shakeFlag = false;
static int rotCand = 0;
static uint32_t rotCandSince = 0;

static void applyRotation(int newRot, bool select);   // fwd

static int orientationCandidate() {
  int cand = physRot;
  if (fabsf(axF) >= fabsf(ayF)) {
    if (axF < -500.0f) cand = 0; else if (axF > 500.0f) cand = 2;
  } else {
    if (ayF > 500.0f) cand = 1; else if (ayF < -500.0f) cand = 3;
  }
  return cand;
}
static float vibThreshold() {   // mg RMS: 0% -> 100 mg (hard to trigger), 100% -> ~4 mg (very sensitive)
  return 100.0f * expf(-3.2f * (float)config.vibrationSensitivity / 100.0f);
}

static void imuTask(uint32_t now) {
  static uint32_t lastRead = 0, lastOrient = 0;
  if (now - lastRead < 2) return;
  lastRead = now;
  float acc[3] = {0, 0, 0}, gyro[3] = {0, 0, 0};
  QMI8658_read_xyz(acc, gyro, NULL);       // acc in mg, gyro in dps
  float mag = sqrtf(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
  if (!imuPrimed) { lpMag = mag; axF = acc[0]; ayF = acc[1]; imuPrimed = true; }
  lpMag += 0.04f * (mag - lpMag);
  float hp = mag - lpMag;                  // dynamic (non-gravity) component
  vibE += 0.03f * (hp * hp - vibE);
  vibRms = sqrtf(vibE);
  axF += 0.08f * (acc[0] - axF);
  ayF += 0.08f * (acc[1] - ayF);

  // --- shake: three strong peaks inside one second
  if (now > shakeBlockUntil && fabsf(hp) > SHAKE_MG && now - lastPeakT > 70) {
    lastPeakT = now;
    shakeT[shakeIdx] = now;
    shakeIdx = (shakeIdx + 1) % 3;
    if (shakeT[shakeIdx] != 0 && now - shakeT[shakeIdx] < 1000) {
      shakeFlag = true;
      shakeBlockUntil = now + 1500;
      shakeT[0] = shakeT[1] = shakeT[2] = 0;
    }
  }

  // --- orientation (debounced, ignored while the machine is vibrating)
  if (now - lastOrient >= 40) {
    lastOrient = now;
#if VIB_DEBUG
    static uint32_t lastDbg = 0;
    if (now - lastDbg > 250) { lastDbg = now; Serial.printf("vibRms=%.1f thr=%.1f\n", vibRms, vibThreshold()); }
#endif
    if (appState != APP_SLEEP && vibRms < 30.0f) {
      int cand = orientationCandidate();
      if (cand != physRot) {
        if (cand != rotCand) { rotCand = cand; rotCandSince = now; }
        else if (now - rotCandSince > 350) applyRotation(cand, appState == APP_SELECT);
      } else rotCand = cand;
    }
  }
}

