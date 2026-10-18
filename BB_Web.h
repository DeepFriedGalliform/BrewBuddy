// BB_Web.h
// Wi-Fi hotspot, web server and its JSON / recipe endpoints
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

// ---------------------------------------------------------------------------
// Wi-Fi hotspot + web page (recipe management and configuration)
// ---------------------------------------------------------------------------
static WebServer server(80);
static lv_obj_t *scrConn, *cnUrl, *cnSsid, *cnPass, *cnQr = NULL, *cnQrBox = NULL;
static char apSsid[24], apPass[12], apQr[80];
static uint32_t connLastReq = 0;
static bool routesSet = false;
static RecipeSet webTmp;
static Deco dConn;

static void jsonStr(String& o, const char* s) {
  o += '"';
  for (; *s; s++) {
    char c = *s;
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c >= 32) o += c;
  }
  o += '"';
}
static void httpRoot() { connLastReq = millis(); server.send_P(200, "text/html", WEB_UI); }
static void httpNotFound() { server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); }

static void httpState() {
  connLastReq = millis(); lastActivity = millis();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  String o; o.reserve(1400);
  o += "{\"v\":1,\"cfg\":{\"slots\":[";
  for (int i = 0; i < 4; i++) { o += (int)config.assignedMethods[i]; if (i < 3) o += ','; }
  o += "],\"ready\":"; o += (int)config.getReadyDuration;
  o += ",\"vib\":"; o += (int)config.vibrationSensitivity;
  o += ",\"sleep\":"; o += (int)config.sleepTimeMinutes;
  o += ",\"gmode\":"; o += (int)config.grindMode;
  o += ",\"unit\":"; o += (int)config.tempUnit;
  o += ",\"gmin\":"; o += (int)config.grindMinX10;
  o += ",\"gmax\":"; o += (int)config.grindMaxX10;
  o += ",\"bright\":"; o += (int)config.brightness;
  o += "},\"methods\":[";
  for (int m = 0; m < METHOD_COUNT; m++) { jsonStr(o, METHOD_NAMES[m]); if (m < METHOD_COUNT - 1) o += ','; }
  o += "],\"types\":[";
  for (int t = 0; t < STEP_COUNT; t++) {
    o += '['; jsonStr(o, STEP_INFO[t].name); o += ','; o += STEP_INFO[t].water ? '1' : '0'; o += ']';
    if (t < STEP_COUNT - 1) o += ',';
  }
  o += "],\"mask\":[";
  for (int m = 0; m < METHOD_COUNT; m++) { o += (int)METHOD_STEP_MASK[m]; if (m < METHOD_COUNT - 1) o += ','; }
  o += "],\"valve\":[";
  for (int m = 0; m < METHOD_COUNT; m++) { o += hasValve(m) ? '1' : '0'; if (m < METHOD_COUNT - 1) o += ','; }
  o += "],\"max\":{\"recipes\":"; o += MAX_RECIPES;
  o += ",\"steps\":"; o += MAX_STEPS;
  o += ",\"name\":"; o += (NAME_LEN - 1);
  o += "},\"recipes\":[[]";
  server.sendContent(o);
  for (int m = 1; m < METHOD_COUNT; m++) {
    String c; c.reserve(7000);
    c += ",[";
    for (int i = 0; i < store[m].count; i++) {
      const Recipe& r = store[m].r[i];
      if (i) c += ',';
      c += "{\"n\":"; jsonStr(c, r.name);
      c += ",\"c\":"; c += (int)r.cups;
      c += ",\"cf\":"; c += (int)r.coffeeX10;
      c += ",\"g\":"; c += (int)r.grindX10;
      c += ",\"t\":"; c += (int)r.tempX10;
      c += ",\"s\":[";
      for (int k = 0; k < r.nSteps; k++) {
        if (k) c += ',';
        c += '['; c += (int)r.steps[k].type; c += ','; c += (int)r.steps[k].valve; c += ',';
        c += (int)r.steps[k].secs; c += ','; c += (int)r.steps[k].water; c += ']';
      }
      c += "]}";
    }
    c += ']';
    server.sendContent(c);
  }
  server.sendContent("]}");
  server.sendContent("");
}

static void httpConfig() {
  connLastReq = millis(); lastActivity = millis();
  String b = server.arg("plain");
  DeviceConfig c = config;
  int pos = 0, n = b.length();
  while (pos < n) {
    int e = b.indexOf('\n', pos); if (e < 0) e = n;
    String line = b.substring(pos, e); pos = e + 1; line.trim();
    int eq = line.indexOf('='); if (eq < 1) continue;
    String k = line.substring(0, eq), v = line.substring(eq + 1);
    int iv = v.toInt();
    if (k == "ready") c.getReadyDuration = (uint8_t)iclamp(iv, 0, 30);
    else if (k == "vib") c.vibrationSensitivity = (uint8_t)iclamp(iv, 0, 100);
    else if (k == "sleep") c.sleepTimeMinutes = (uint16_t)iclamp(iv, 0, 240);
    else if (k == "gmode") c.grindMode = iv ? 1 : 0;
    else if (k == "unit") c.tempUnit = iv ? 1 : 0;
    else if (k == "gmin") c.grindMinX10 = (uint16_t)iclamp(iv, 1, 490);
    else if (k == "gmax") c.grindMaxX10 = (uint16_t)iclamp(iv, 11, 500);
    else if (k == "bright") c.brightness = (uint8_t)iclamp(iv, 10, 100);
    else if (k == "slots") {
      int a[4];
      if (sscanf(v.c_str(), "%d,%d,%d,%d", &a[0], &a[1], &a[2], &a[3]) == 4)
        for (int i = 0; i < 4; i++) if (a[i] >= 0 && a[i] < METHOD_COUNT) c.assignedMethods[i] = (uint8_t)a[i];
    }
  }
  if (c.grindMaxX10 < c.grindMinX10 + 10) c.grindMaxX10 = c.grindMinX10 + 10;
  config = c;
  saveConfig();
  applyBacklight(config.brightness);
  server.send(200, "text/plain", "ok");
}

// Body format (one recipe = one R line followed by its S lines):
//   R|name|cups|coffeeX10|grindX10|tempX10
//   S|type|valve|secs|water
static bool parseRecipes(const String& body, RecipeSet& out, const char** err) {
  memset(&out, 0, sizeof(out));
  Recipe* cur = NULL;
  int pos = 0, n = body.length();
  while (pos < n) {
    int e = body.indexOf('\n', pos); if (e < 0) e = n;
    String line = body.substring(pos, e); pos = e + 1; line.trim();
    if (line.length() < 3 || line[1] != '|') continue;
    char kind = line[0];
    String f[6]; int nf = 0, p = 2;
    while (nf < 6) {
      int q = line.indexOf('|', p);
      if (q < 0) { f[nf++] = line.substring(p); break; }
      f[nf++] = line.substring(p, q); p = q + 1;
    }
    if (kind == 'R') {
      if (nf < 5 || out.count >= MAX_RECIPES) { *err = "bad recipe"; return false; }
      cur = &out.r[out.count++];
      int len = 0;
      for (unsigned i = 0; i < f[0].length() && len < NAME_LEN - 1; i++) {
        char ch = f[0][i];
        if ((uint8_t)ch >= 32 && ch != 127) cur->name[len++] = ch;
      }
      while (len > 0 && cur->name[len - 1] == ' ') cur->name[--len] = 0;
      if (len == 0) strcpy(cur->name, "Recipe");
      cur->cups = f[1].toInt() == 2 ? 2 : 1;
      cur->coffeeX10 = (uint16_t)iclamp(f[2].toInt(), 10, 600);
      cur->grindX10 = (uint16_t)iclamp(f[3].toInt(), 1, 990);
      cur->tempX10 = (uint16_t)iclamp(f[4].toInt(), 400, 1000);
    } else if (kind == 'S') {
      if (!cur || nf < 4 || cur->nSteps >= MAX_STEPS) { *err = "bad step"; return false; }
      int t = f[0].toInt();
      if (t < 0 || t >= STEP_COUNT) { *err = "bad step type"; return false; }
      Step& st = cur->steps[cur->nSteps++];
      st.type = (uint8_t)t;
      st.valve = f[1].toInt() ? 1 : 0;
      st.secs = (uint16_t)iclamp(f[2].toInt(), 1, 900);
      st.water = STEP_INFO[t].water ? (uint16_t)iclamp(f[3].toInt(), 0, 1000) : 0;
    }
  }
  if (out.count < 1) { *err = "no recipes"; return false; }
  for (int i = 0; i < out.count; i++) {
    if (out.r[i].nSteps < 1) { *err = "recipe without steps"; return false; }
    out.r[i].waterG = recipeSumWater(out.r[i]);
  }
  return true;
}
static void httpRecipes() {
  connLastReq = millis(); lastActivity = millis();
  int m = server.arg("m").toInt();
  if (m < 1 || m >= METHOD_COUNT) { server.send(400, "text/plain", "bad method"); return; }
  const char* err = NULL;
  if (!parseRecipes(server.arg("plain"), webTmp, &err)) { server.send(400, "text/plain", err ? err : "bad data"); return; }
  store[m] = webTmp;
  recipeIdx[m] = 0;
  fixSelection(m);
  saveRecipes(m);
  server.send(200, "text/plain", "ok");
}
static void httpRestore() {
  connLastReq = millis(); lastActivity = millis();
  restoreDefaultRecipes();
  server.send(200, "text/plain", "ok");
}

static void httpHistory() {
  connLastReq = millis(); lastActivity = millis();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  String o; o.reserve(4200);
  o += "{\"h\":[";
  for (int i = 0; i < histCount; i++) {                          // newest first
    const HistEntry& h = hist[(histHead + HIST_MAX - 1 - i) % HIST_MAX];
    if (i) o += ',';
    o += "{\"t\":"; o += (unsigned)h.epoch;
    o += ",\"m\":"; jsonStr(o, METHOD_NAMES[h.method < METHOD_COUNT ? h.method : 0]);
    o += ",\"n\":"; jsonStr(o, h.name);
    o += ",\"c\":"; o += (int)h.cups;
    o += ",\"k\":"; o += (int)h.kind;
    o += ",\"d\":"; o += (int)(h.ds / 10); o += '.'; o += (int)(h.ds % 10);
    o += ",\"l\":[";
    for (int k = 0; k < h.laps && k < 4; k++) { if (k) o += ','; o += (int)(h.lapDs[k] / 10); o += '.'; o += (int)(h.lapDs[k] % 10); }
    o += "]}";
  }
  o += "]}";
  server.sendContent(o);
  server.sendContent("");
}
static void httpHistClear() { connLastReq = millis(); lastActivity = millis(); histClear(); server.send(200, "text/plain", "ok"); }
static void httpTime() {                                           // the web page sets the clock so history entries get real dates
  connLastReq = millis(); lastActivity = millis();
  long long t = atoll(server.arg("plain").c_str());
  if (t > 1700000000LL) { struct timeval tv; tv.tv_sec = (time_t)t; tv.tv_usec = 0; settimeofday(&tv, NULL); }
  server.send(200, "text/plain", "ok");
}

static void stopConnect() {
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  batSlow = batV;                                  // radio load changes the battery reading: re-baseline
  detectHoldUntil = millis() + 6000;
  if (cnQr) { lv_obj_delete(cnQr); cnQr = NULL; }  // give the QR memory back
  Serial.println("[WIFI] stopped");
  showSelect();
}
static void cb_conn_stop(lv_event_t* e) { if (clickOK()) stopConnect(); }

static void createConnect() {
  scrConn = newScreen();
  dConn.band = false;
  mkDeco(scrConn, &dConn);
  cnUrl = mkTitle(scrConn, "192.168.4.1");
  lv_obj_t* box = lv_obj_create(scrConn);
  lv_obj_remove_style_all(box);
  lv_obj_set_size(box, 132, 132);
  lv_obj_set_pos(box, CX - 66, 28);
  lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(box, COL_WHITE, 0);
  lv_obj_set_style_radius(box, 8, 0);
  lv_obj_remove_flag(box, LV_OBJ_FLAG_CLICKABLE);
  cnQrBox = box;                                    // the QR widget itself is created when Wi-Fi starts
#if !LV_USE_QRCODE
  lv_obj_t* nq = mkLabel(box, "QR disabled.\nSet LV_USE_QRCODE 1\nin lv_conf.h", FONT_XS, lv_color_black(), CX, CY, 120);
  (void)nq;
#endif
  cnSsid = mkLabel(scrConn, "", FONT_XS, COL_CREAM, CX, 174, 170);
  cnPass = mkLabel(scrConn, "", FONT_XS, COL_AMBER, CX, 191, 170);
  mkBtn(scrConn, LV_SYMBOL_CLOSE " Stop", CX, 214, 84, 24, COL_RED, cb_conn_stop);
}

static void startConnect() {
  if (safeMode) { setTxt(cgVal, "Safe mode"); return; }
  static const char alpha[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";     // no look-alike characters
  for (int i = 0; i < 8; i++) apPass[i] = alpha[esp_random() % 32];
  apPass[8] = 0;
  uint64_t mac = ESP.getEfuseMac();
  snprintf(apSsid, sizeof(apSsid), "BrewBuddy-%02X%02X", (unsigned)((mac >> 40) & 0xFF), (unsigned)((mac >> 32) & 0xFF));
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(apSsid, apPass, 6, 0, 2)) {
    WiFi.mode(WIFI_OFF);
    setTxt(cgVal, "Wi-Fi failed");
    return;
  }
  delay(100);
  IPAddress ip = WiFi.softAPIP();
  if (!routesSet) {
    server.on("/", HTTP_GET, httpRoot);
    server.on("/api/state", HTTP_GET, httpState);
    server.on("/api/config", HTTP_POST, httpConfig);
    server.on("/api/recipes", HTTP_POST, httpRecipes);
    server.on("/api/restore", HTTP_POST, httpRestore);
    server.on("/api/history", HTTP_GET, httpHistory);
    server.on("/api/history/clear", HTTP_POST, httpHistClear);
    server.on("/api/time", HTTP_POST, httpTime);
    server.onNotFound(httpNotFound);
    routesSet = true;
  }
  server.begin();
  char b[48];
  snprintf(apQr, sizeof(apQr), "WIFI:T:WPA;S:%s;P:%s;;", apSsid, apPass);
  snprintf(b, sizeof(b), "%u.%u.%u.%u", (unsigned)ip[0], (unsigned)ip[1], (unsigned)ip[2], (unsigned)ip[3]);
  setTxt(cnUrl, b);
  snprintf(b, sizeof(b), "Wi-Fi: %s", apSsid);  setTxt(cnSsid, b);
  snprintf(b, sizeof(b), "Password: %s", apPass); setTxt(cnPass, b);
#if LV_USE_QRCODE
  if (!cnQr) {
    cnQr = lv_qrcode_create(cnQrBox);
    if (cnQr) {
      lv_qrcode_set_size(cnQr, 116);
      lv_qrcode_set_dark_color(cnQr, lv_color_black());
      lv_qrcode_set_light_color(cnQr, COL_WHITE);
      lv_obj_center(cnQr);
    }
  }
  if (cnQr) lv_qrcode_update(cnQr, apQr, strlen(apQr));
#endif
  connLastReq = millis();
  batSlow = batV;
  appState = APP_CONNECT;
  loadScr(scrConn);
  Serial.printf("[WIFI] hotspot %s / %s at %s\n", apSsid, apPass, b);
}

