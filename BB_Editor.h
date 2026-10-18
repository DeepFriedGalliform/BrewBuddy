// BB_Editor.h
// On-device recipe editor (add / edit)
// Part of the BrewBuddy sketch: included once, in order, from BrewBuddy.ino (keep it in the same folder).
#pragma once

// ---------------------------------------------------------------------------
// Recipe editor (Add / Edit)
// ---------------------------------------------------------------------------
enum EdPage { EP_NAME, EP_COFFEE, EP_WATER, EP_TEMP, EP_STEPS, EP_STEP, EP_SAVE };
enum EdBtn  { EB_BACK, EB_NEXT, EB_A_DEC, EB_A_INC, EB_B_DEC, EB_B_INC, EB_C_DEC, EB_C_INC,
              EB_STEP_ADD, EB_STEP_EDIT, EB_STEP_DEL, EB_VALVE, EB_SAVE, EB_LIST };

static Recipe ed;
static int  edMethod = METHOD_POUR_OVER, edIndex = -1, edPage = EP_NAME, edStepSel = 0, edNameIdx = -1;
static bool edRebuild = false;
static lv_obj_t *edBody, *edTitle, *edLA, *edLB, *edLC, *edBtnValve, *edRoller, *edSlider;
static char edOpts[MAX_STEPS * 28];

static int edRatioX10() { return ed.coffeeX10 ? (ed.waterG * 100 + ed.coffeeX10 / 2) / ed.coffeeX10 : 0; }

static void edSyncWater() { ed.waterG = recipeSumWater(ed); }
static void edRescaleSteps(int oldW, int newW) {
  if (oldW <= 0) { ed.waterG = newW; return; }
  int sum = 0, last = -1;
  for (int i = 0; i < ed.nSteps; i++) {
    if (!STEP_INFO[ed.steps[i].type].water) continue;
    int w = ((ed.steps[i].water * newW / oldW + 2) / 5) * 5;
    ed.steps[i].water = w; sum += w; last = i;
  }
  if (last >= 0) {
    int fixed = (int)ed.steps[last].water + (newW - sum);
    ed.steps[last].water = (uint16_t)imax(0, fixed);
    edSyncWater();
  } else ed.waterG = newW;
}
static uint8_t edNextType(uint8_t cur, int dir) {
  uint16_t mask = METHOD_STEP_MASK[edMethod];
  int t = cur;
  for (int i = 0; i < STEP_COUNT; i++) {
    t = (t + dir + STEP_COUNT) % STEP_COUNT;
    if (mask & SB(t)) return (uint8_t)t;
  }
  return cur;
}
static void edBuildOpts() {
  edOpts[0] = 0;
  for (int i = 0; i < ed.nSteps; i++) {
    const Step& s = ed.steps[i];
    char line[32];
    if (STEP_INFO[s.type].water) snprintf(line, sizeof(line), "%d %s %ds %dg", i + 1, STEP_INFO[s.type].name, s.secs, s.water);
    else snprintf(line, sizeof(line), "%d %s %ds", i + 1, STEP_INFO[s.type].name, s.secs);
    if (hasValve(edMethod)) strcat(line, s.valve ? "  O" : "  C");
    strcat(edOpts, line);
    if (i + 1 < ed.nSteps) strcat(edOpts, "\n");
  }
}
static void edRefresh() {
  char b[96];
  switch (edPage) {
    case EP_NAME: setTxt(edLA, edNameIdx >= 0 ? NAME_PRESETS[edNameIdx] : ed.name); break;
    case EP_COFFEE:
      fmtG10(b, sizeof(b), ed.coffeeX10); setTxt(edLA, b);
      fmtGrind(b, sizeof(b), ed.grindX10); setTxt(edLB, b);
      lv_obj_set_style_text_font(edLB, config.grindMode ? FONT_SM : FONT_MD, 0);
      break;
    case EP_WATER: {
      int r = edRatioX10();
      snprintf(b, sizeof(b), "1:%d.%d", r / 10, r % 10); setTxt(edLA, b);
      snprintf(b, sizeof(b), "%dg", ed.waterG); setTxt(edLB, b); break;
    }
    case EP_TEMP:
      fmtTemp(b, sizeof(b), ed.tempX10); setTxt(edLA, b);
      if (edSlider) lv_slider_set_value(edSlider, tempDisp(ed.tempX10), LV_ANIM_OFF);
      break;
    case EP_STEP: {
      const Step& s = ed.steps[edStepSel];
      setTxt(edLA, STEP_INFO[s.type].name);
      snprintf(b, sizeof(b), "%d s", s.secs); setTxt(edLB, b);
      if (edLC) { snprintf(b, sizeof(b), "%dg", s.water); setTxt(edLC, b); }
      if (edBtnValve) {
        btnText(edBtnValve, s.valve ? "Valve: OPEN" : "Valve: CLOSED");
        lv_obj_set_style_bg_color(edBtnValve, s.valve ? COL_GREEN : COL_RED, 0);
      }
      break;
    }
    case EP_SAVE: {
      char cof[12], tm[8], gr[16], tmpS[12];
      fmtG10(cof, sizeof(cof), ed.coffeeX10);
      fmtTime(tm, sizeof(tm), recipeTotalSecs(ed));
      fmtGrind(gr, sizeof(gr), ed.grindX10);
      fmtTemp(tmpS, sizeof(tmpS), ed.tempX10);
      int r = edRatioX10();
      const char* nm = edNameIdx >= 0 ? NAME_PRESETS[edNameIdx] : ed.name;
      snprintf(b, sizeof(b), "%s\n%s : %dg  (1:%d.%d)\n%s  grind %s\n%d steps  %s", nm, cof, ed.waterG, r / 10, r % 10, tmpS, gr, ed.nSteps, tm);
      setTxt(edLA, b);
      break;
    }
  }
}
static void cb_ed(lv_event_t* e);
static void edApply(int row, int dir);
// row: 0/1/2 = A/B/C. px = scrub distance per step
static lv_obj_t* edRow(int cy, int decId, int incId, const lv_font_t* f, int row, int px) {
  mkBtn(edBody, LV_SYMBOL_MINUS, 46, cy, 42, 32, COL_GREY, cb_ed, ID(decId));
  mkBtn(edBody, LV_SYMBOL_PLUS, 194, cy, 42, 32, COL_GREY, cb_ed, ID(incId));
  addZone(70, cy - 20, 170, cy + 20, row, px);
  return mkLabel(edBody, "", f, COL_WHITE, CX, cy, 100);
}
static void cb_ed_slider(lv_event_t* e) {
  lv_obj_t* s = (lv_obj_t*)lv_event_get_target(e);
  ed.tempX10 = (uint16_t)tempFromDisp(lv_slider_get_value(s));
  char b[16]; fmtTemp(b, sizeof(b), ed.tempX10); setTxt(edLA, b);
}
static void edBuildPage() {
  lv_obj_clean(edBody);
  edLA = edLB = edLC = edBtnValve = edRoller = edSlider = NULL;
  clearZones();
  char t[24];
  switch (edPage) {
    case EP_NAME:
      setTxt(edTitle, "Recipe name");
      mkLabel(edBody, "Name", FONT_XS, COL_DIM, CX, 64, 120);
      edLA = edRow(106, EB_A_DEC, EB_A_INC, FONT_MD, 0, 40);
      break;
    case EP_COFFEE:
      setTxt(edTitle, "Coffee");
      mkLabel(edBody, "Coffee weight", FONT_XS, COL_DIM, CX, 58, 140);
      edLA = edRow(86, EB_A_DEC, EB_A_INC, FONT_MD, 0, 14);
      mkLabel(edBody, "Grind setting", FONT_XS, COL_DIM, CX, 120, 140);
      edLB = edRow(148, EB_B_DEC, EB_B_INC, FONT_MD, 1, config.grindMode ? 40 : 10);
      break;
    case EP_WATER:
      setTxt(edTitle, "Water");
      mkLabel(edBody, "Coffee : water ratio", FONT_XS, COL_DIM, CX, 58, 160);
      edLA = edRow(86, EB_A_DEC, EB_A_INC, FONT_MD, 0, 10);
      mkLabel(edBody, "Water amount", FONT_XS, COL_DIM, CX, 120, 140);
      edLB = edRow(148, EB_B_DEC, EB_B_INC, FONT_MD, 1, 8);
      break;
    case EP_TEMP:
      setTxt(edTitle, "Temperature");
      mkLabel(edBody, "Water temperature", FONT_XS, COL_DIM, CX, 58, 160);
      edLA = edRow(92, EB_A_DEC, EB_A_INC, FONT_MD, 0, 12);
      edSlider = lv_slider_create(edBody);
      lv_obj_set_size(edSlider, 130, 10);
      lv_obj_align(edSlider, LV_ALIGN_CENTER, 0, 138 - CY);
      lv_slider_set_range(edSlider, tempDisp(600), tempDisp(1000));
      lv_obj_set_style_bg_color(edSlider, COL_AMBER, LV_PART_INDICATOR);
      lv_obj_set_style_bg_color(edSlider, COL_AMBER, LV_PART_KNOB);
      lv_obj_add_event_cb(edSlider, cb_ed_slider, LV_EVENT_VALUE_CHANGED, NULL);
      break;
    case EP_STEPS:
      setTxt(edTitle, "Steps");
      edBuildOpts();
      edRoller = lv_roller_create(edBody);
      lv_roller_set_options(edRoller, edOpts, LV_ROLLER_MODE_NORMAL);
      lv_roller_set_visible_row_count(edRoller, 3);
      lv_obj_set_width(edRoller, 176);
      lv_obj_align(edRoller, LV_ALIGN_CENTER, 0, 90 - CY);
      lv_obj_set_style_bg_color(edRoller, lv_color_hex(0x1C1C1C), 0);
      lv_obj_set_style_border_width(edRoller, 0, 0);
      lv_obj_set_style_text_color(edRoller, COL_DIM, 0);
      lv_obj_set_style_bg_color(edRoller, COL_AMBER, LV_PART_SELECTED);
      lv_obj_set_style_text_color(edRoller, lv_color_black(), LV_PART_SELECTED);
      lv_roller_set_selected(edRoller, edStepSel, LV_ANIM_OFF);
      mkBtn(edBody, LV_SYMBOL_PLUS, CX - 58, 152, 52, 30, COL_GREEN, cb_ed, ID(EB_STEP_ADD));
      mkBtn(edBody, LV_SYMBOL_EDIT, CX, 152, 52, 30, COL_BLUE, cb_ed, ID(EB_STEP_EDIT));
      mkBtn(edBody, LV_SYMBOL_TRASH, CX + 58, 152, 52, 30, COL_RED, cb_ed, ID(EB_STEP_DEL));
      break;
    case EP_STEP: {
      snprintf(t, sizeof(t), "Step %d/%d", edStepSel + 1, ed.nSteps);
      setTxt(edTitle, t);
      int y = 58;
      edLA = edRow(y, EB_A_DEC, EB_A_INC, FONT_SM, 0, 40); y += 36;
      edLB = edRow(y, EB_B_DEC, EB_B_INC, FONT_MD, 1, 14); y += 36;
      if (STEP_INFO[ed.steps[edStepSel].type].water) { edLC = edRow(y, EB_C_DEC, EB_C_INC, FONT_MD, 2, 14); y += 36; }
      if (hasValve(edMethod)) edBtnValve = mkBtn(edBody, "", CX, y, 140, 30, COL_RED, cb_ed, ID(EB_VALVE));
      break;
    }
    case EP_SAVE:
      setTxt(edTitle, "Save recipe");
      edLA = mkLabel(edBody, "", FONT_SM, COL_WHITE, CX, 100, 170);
      lv_obj_set_style_text_line_space(edLA, 4, 0);
      break;
  }
  if (edPage == EP_STEP) {
    // < previous step   [list]   > next step (after the last step -> Save)
    mkBtn(edBody, LV_SYMBOL_LEFT, CX - 52, 198, 46, 28, COL_GREY, cb_ed, ID(EB_BACK));
    mkBtn(edBody, LV_SYMBOL_LIST, CX, 198, 46, 28, COL_BLUE, cb_ed, ID(EB_LIST));
    mkBtn(edBody, edStepSel >= ed.nSteps - 1 ? LV_SYMBOL_OK : LV_SYMBOL_RIGHT, CX + 52, 198, 46, 28,
          edStepSel >= ed.nSteps - 1 ? COL_GREEN : COL_BLUE, cb_ed, ID(EB_NEXT));
  } else {
    mkBtn(edBody, LV_SYMBOL_LEFT, CX - 40, 198, 62, 30, COL_GREY, cb_ed, ID(EB_BACK));
    if (edPage == EP_SAVE) mkBtn(edBody, LV_SYMBOL_OK " Save", CX + 40, 198, 76, 30, COL_GREEN, cb_ed, ID(EB_SAVE));
    else mkBtn(edBody, LV_SYMBOL_RIGHT, CX + 40, 198, 62, 30, COL_BLUE, cb_ed, ID(EB_NEXT));
  }
  edRefresh();
}

static void startEditor(bool isNew) {
  edMethod = curMethod();
  bool asNew = isNew || recipeFor(edMethod).cups != (twoCup ? 2 : 1);   // editing a scaled view saves a new recipe
  ed = asNew ? viewRecipe(edMethod) : recipeFor(edMethod);
  ed.grindX10 = (uint16_t)iclamp((int)ed.grindX10, config.grindMinX10, config.grindMaxX10);
  edIndex = asNew ? -1 : recipeIdx[edMethod];
  edNameIdx = isNew ? 9 : -1;
  edPage = EP_NAME; edStepSel = 0;
  appState = APP_EDIT;
  guardObj = NULL;
  loadScr(scrEdit);
  edBuildPage();
}
static void edSave() {
  RecipeSet& s = store[edMethod];
  if (edNameIdx >= 0) { memset(ed.name, 0, NAME_LEN); strncpy(ed.name, NAME_PRESETS[edNameIdx], NAME_LEN - 1); }
  // keep names unique inside the method
  char base[NAME_LEN]; strncpy(base, ed.name, NAME_LEN - 1); base[NAME_LEN - 1] = 0;
  for (int n = 2; n < 20; n++) {
    bool clash = false;
    for (int i = 0; i < s.count; i++) if (i != edIndex && s.r[i].cups == ed.cups && strcmp(s.r[i].name, ed.name) == 0) clash = true;
    if (!clash) break;
    char suf[6]; snprintf(suf, sizeof(suf), " %d", n);
    int keep = imin((int)strlen(base), NAME_LEN - 1 - (int)strlen(suf));
    memset(ed.name, 0, NAME_LEN);
    memcpy(ed.name, base, keep);
    strcat(ed.name, suf);
  }
  edSyncWater();
  if (edIndex >= 0 && edIndex < s.count) { s.r[edIndex] = ed; recipeIdx[edMethod] = edIndex; }
  else if (s.count < MAX_RECIPES) { s.r[s.count] = ed; recipeIdx[edMethod] = s.count; s.count++; }
  saveRecipes(edMethod);
  queueSettingsSave();
  showSelect();
}

// Change the value on row A/B/C of the current page by one step (buttons and drag-scrub both come here)
static void edApply(int row, int dir) {
  switch (edPage) {
    case EP_NAME:
      if (edNameIdx < 0) edNameIdx = dir > 0 ? 0 : NAME_PRESET_COUNT - 1;
      else edNameIdx = (edNameIdx + dir + NAME_PRESET_COUNT) % NAME_PRESET_COUNT;
      break;
    case EP_COFFEE:
      if (row == 0) ed.coffeeX10 = (uint16_t)iclamp((int)ed.coffeeX10 + dir * 5, 30, 400);
      else if (config.grindMode) ed.grindX10 = (uint16_t)grindValForIdx(iclamp(grindIdxFor(ed.grindX10) + dir, 0, 5));
      else ed.grindX10 = (uint16_t)iclamp((int)ed.grindX10 + dir, config.grindMinX10, config.grindMaxX10);
      break;
    case EP_WATER:
      if (row == 0) {
        int r = edRatioX10() + dir;                          // 0.1 ratio steps
        int nw = iclamp((ed.coffeeX10 * r + 50) / 100, 50, 1000);
        edRescaleSteps(ed.waterG, nw);
      } else {
        edRescaleSteps(ed.waterG, iclamp((int)ed.waterG + dir, 50, 1000));
      }
      break;
    case EP_TEMP: {
      int v = iclamp(tempDisp(ed.tempX10) + dir, tempDisp(600), tempDisp(1000));
      ed.tempX10 = (uint16_t)tempFromDisp(v);
      break;
    }
    case EP_STEP: {
      Step& s = ed.steps[edStepSel];
      if (row == 0) {
        s.type = edNextType(s.type, dir);
        if (STEP_INFO[s.type].water && s.water == 0) s.water = 50;
        if (!STEP_INFO[s.type].water) s.water = 0;
        edSyncWater();
        edRebuild = true;       // water row appears / disappears
        return;
      } else if (row == 1) {
        s.secs = (uint16_t)iclamp((int)s.secs + dir * 5, 5, 600);
      } else {
        s.water = (uint16_t)iclamp((int)s.water + dir * 5, 0, 800);
        edSyncWater();
      }
      break;
    }
    default: break;
  }
  edRefresh();
}

static void cb_ed(lv_event_t* e) {
  if (!clickOK()) return;
  int id = (int)(intptr_t)lv_event_get_user_data(e);
  lastActivity = millis();
  if (edPage == EP_STEPS && edRoller) edStepSel = lv_roller_get_selected(edRoller);
  switch (id) {
    case EB_BACK:
      if (edPage == EP_NAME) { showSelect(); return; }
      if (edPage == EP_STEP) { if (edStepSel > 0) edStepSel--; else edPage = EP_STEPS; }
      else if (edPage == EP_SAVE) edPage = EP_STEPS;
      else edPage--;
      edRebuild = true; return;
    case EB_NEXT:
      if (edPage == EP_STEP) { if (edStepSel < ed.nSteps - 1) edStepSel++; else edPage = EP_SAVE; }
      else if (edPage == EP_STEPS) edPage = EP_SAVE;
      else edPage++;
      edRebuild = true; return;
    case EB_LIST: edPage = EP_STEPS; edRebuild = true; return;
    case EB_SAVE: edSave(); return;
    case EB_STEP_EDIT: edPage = EP_STEP; edRebuild = true; return;
    case EB_STEP_ADD:
      if (ed.nSteps < MAX_STEPS) {
        int at = edStepSel + 1;
        for (int i = ed.nSteps; i > at; i--) ed.steps[i] = ed.steps[i - 1];
        Step ns; ns.type = STEP_WAIT; ns.secs = 30; ns.water = 0; ns.valve = ed.steps[edStepSel].valve;
        ed.steps[at] = ns; ed.nSteps++; edStepSel = at;
        edPage = EP_STEP;
      }
      edRebuild = true; return;
    case EB_STEP_DEL:
      if (ed.nSteps > 1) {
        for (int i = edStepSel; i < ed.nSteps - 1; i++) ed.steps[i] = ed.steps[i + 1];
        ed.nSteps--;
        if (edStepSel >= ed.nSteps) edStepSel = ed.nSteps - 1;
        edSyncWater();
      }
      edRebuild = true; return;
    case EB_VALVE: ed.steps[edStepSel].valve ^= 1; edRefresh(); return;
  }
  int dir = (id == EB_A_INC || id == EB_B_INC || id == EB_C_INC) ? 1 : -1;
  int row = (id == EB_A_DEC || id == EB_A_INC) ? 0 : ((id == EB_B_DEC || id == EB_B_INC) ? 1 : 2);
  edApply(row, dir);
}
static void createEditor() {
  scrEdit = newScreen();
  mkDeco(scrEdit, &dEdit);
  edTitle = mkTitle(scrEdit, "Recipe");
  edBody = lv_obj_create(scrEdit);
  lv_obj_remove_style_all(edBody);
  lv_obj_set_size(edBody, SCREEN_W, SCREEN_H);
  lv_obj_set_pos(edBody, 0, 0);
  lv_obj_remove_flag(edBody, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_text_font(edBody, FONT_SM, 0);
  lv_obj_set_style_text_color(edBody, COL_WHITE, 0);
}

