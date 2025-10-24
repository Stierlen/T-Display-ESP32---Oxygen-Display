#include "LV_Helper.h"
#include "ui.h"

// ---------- Hardware & Kalibrierung ----------
const int   sensorPin     = A10;       // Analog in
const float druckFaktor   = 0.0795122f;  // bar pro ADC-Count

// ---------- Abtastung ----------
const uint32_t SAMPLE_PERIOD_MS = 250;
uint32_t lastSampleMs = 0;

// ---------- Entstörung ----------
const int MEDIAN_N = 5;
int16_t medBuf[MEDIAN_N];
int     medIdx = 0;
bool    medFilled = false;

float alphaEMA = 0.15f;   // EMA-Glättung
float ema      = NAN;     // geglättete ADC-Counts

// ---------- Trendanalyse (Regression über 30 s) ----------
const int   REG_N = 240;          // 120 x 250ms = 30s
float       regBuf[REG_N];
int         regIdx   = 0;
int         regCount = 0;

const float minSlopeThreshold = -0.005f; // Counts/s  (echter Abfall)
const float slopeNoiseFloor   = -0.0005f;// nahe 0 -> unbrauchbar

// ---------- Flasche & Anzeige ----------
const float bottleLiters        = 3.0f;     // [L]
const float P_rest              = 40.0f;    // [bar]  Alarm/Leergrenze
const float P_warn_gelb         = 80.0f;    // [bar]  Warnschwelle
const float BAR_FULL_SCALE_BAR  = 200.0f;   // [bar]  Balken-Vollskala

// Zeitwarnung & Blinkintervall
const float    T_warn_minutes     = 30.0f;   // [min]  Schwelle 6000m-Mindestzeit
const uint32_t BLINK_INTERVAL_MS  = 500;     // [ms]   Blinkintervall (nur Schraffur)

// ---------- UI-Farben ----------
const uint32_t COLOR_GREEN_HI = 0x91FF00; // Wunsch-Neongrün (#91FF00)
const uint32_t COLOR_YELLOW   = 0xFFFF00; // Gelb
const uint32_t COLOR_RED      = 0xFF0000; // Rot
const uint32_t COLOR_ORANGE   = 0xFFA500; // Orange (Label5 "6k")

// ---------- Schraffur-Parameter (Bar3 Overlay) ----------
const int HATCH_LINE_W = 6;   // Linienbreite
const int HATCH_GAP    = 14;  // Abstand
const int HATCH_STEP   = HATCH_LINE_W + HATCH_GAP; // 24 px

// ---------- Mindestreichweite: Fixe Höhe 6000 m ----------
const int   persons = 1; // Anzahl versorgter Personen
static inline float edsFlowLpm_at6000m() {
  const float ft   = 19685.0f;           // 6000 m
  const float x0   = 18000.0f, y0 = 0.724f;
  const float x1   = 20000.0f, y1 = 1.046f;
  const float frac = (ft - x0) / (x1 - x0);
  return y0 + frac * (y1 - y0); // ~0.89 L/min pro Person
}

// ---------- Startfenster / No-Change-Detektor ----------
const uint32_t NO_CHANGE_WINDOW_MS = 60000;  // 1 min
const float    NO_CHANGE_TOL_BAR   = 0.20f;  // 0,2 bar Toleranz
float          noChangeTolCounts   = NO_CHANGE_TOL_BAR / druckFaktor;

uint32_t bootStartMs        = 0;
bool     bootRefSet         = false;
float    bootRefCounts      = NAN;
bool     significantChange  = false;

// ---------- Statusanzeige (Label5) ----------
static int lastStatus = -1; // -1=unbekannt, 0=6k, 1=Trend
static inline void setStatusLabel(bool trendActive){
  if(trendActive){
    if(lastStatus != 1){
      lv_label_set_text(ui_Label5, "T");
      lv_obj_set_style_text_color(ui_Label5, lv_color_hex(COLOR_GREEN_HI), LV_PART_MAIN | LV_STATE_DEFAULT);
      lastStatus = 1;
    }
  } else {
    if(lastStatus != 0){
      lv_label_set_text(ui_Label5, "6k");
      lv_obj_set_style_text_color(ui_Label5, lv_color_hex(COLOR_ORANGE), LV_PART_MAIN | LV_STATE_DEFAULT);
      lastStatus = 0;
    }
  }
}

// ---------- Label1-Farblogik (mit Zeit- oder Druckwarnung) ----------
static inline void updatePressureLabelColor(float P_bar, bool critical){
  if(critical){
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(COLOR_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
    return;
  }
  if(P_bar < P_rest){
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(COLOR_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
  } else if(P_bar < P_warn_gelb){
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(COLOR_YELLOW), LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(COLOR_GREEN_HI), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

// ---------- Bar3 Overlay für 45°-Schraffur ----------
static lv_obj_t* ui_Bar3Overlay = nullptr;

// Schraffur-Zeichnung: volle Objektfläche (get_coords), 45°, rot, Breite HATCH_LINE_W, Abstand HATCH_GAP
static void bar3_hatch_draw_event_cb(lv_event_t * e){
  lv_obj_t* obj = lv_event_get_target(e);
  lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);

  lv_area_t coords;
  lv_obj_get_coords(obj, &coords);  // volle Fläche

  const int16_t w = lv_area_get_width(&coords);
  const int16_t h = lv_area_get_height(&coords);

  lv_draw_line_dsc_t dsc;
  lv_draw_line_dsc_init(&dsc);
  dsc.color = lv_color_hex(COLOR_RED);
  dsc.width = HATCH_LINE_W;   // 6 px
  dsc.opa   = LV_OPA_COVER;

  const int step = HATCH_STEP; // 24 px
  const int x_start = coords.x1 - h;
  const int x_end   = coords.x1 + w;

  for (int x = x_start; x < x_end; x += step) {
    lv_point_t p1 = { (lv_coord_t)x,       coords.y2 };                       // unten
    lv_point_t p2 = { (lv_coord_t)(x + h), (lv_coord_t)(coords.y2 - h) };     // oben
    lv_draw_line(draw_ctx, &dsc, &p1, &p2);  // LVGL-API (4 Argumente)
  }
}

// ---------- Bar3-Farblogik & (nur Schraffur-)Blinken ----------
static inline void updateBarColorAndBlink(float P_bar, bool critical, uint32_t now){
  if(critical){
    // Indicator bleibt statisch ROT (kein Blinken)
    lv_obj_set_style_bg_color(ui_Bar3, lv_color_hex(COLOR_RED), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa  (ui_Bar3, LV_OPA_COVER,          LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Nur die Schraffur (Overlay) blinkt
    const bool phaseOn = ((now / BLINK_INTERVAL_MS) % 2) == 0;
    if(ui_Bar3Overlay){
      lv_obj_set_style_opa(ui_Bar3Overlay, phaseOn ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    return;
  }

  // Kein kritischer Zustand: Overlay aus, Indicator statisch nach Druck
  if(ui_Bar3Overlay){
    lv_obj_set_style_opa(ui_Bar3Overlay, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  lv_color_t c;
  if(P_bar < P_rest){
    c = lv_color_hex(COLOR_RED);
  } else if(P_bar < P_warn_gelb){
    c = lv_color_hex(COLOR_YELLOW);
  } else {
    c = lv_color_hex(COLOR_GREEN_HI);
  }
  lv_obj_set_style_bg_color(ui_Bar3, c, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa  (ui_Bar3, LV_OPA_COVER,         LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

// ---------- Hilfsfunktionen ----------
static inline int clamp12b(int v){ return v<0?0:(v>4095?4095:v); }

int median5(const int16_t *x){
  int16_t a[MEDIAN_N];
  for(int i=0;i<MEDIAN_N;++i) a[i]=x[i];
  for(int i=1;i<MEDIAN_N;++i){
    int16_t key=a[i]; int j=i-1;
    while(j>=0 && a[j]>key){ a[j+1]=a[j]; --j; }
    a[j+1]=key;
  }
  return a[MEDIAN_N/2];
}

float doEMA(float prev, float x, float alpha){
  if(isnan(prev)) return x;
  return alpha*x + (1.0f-alpha)*prev;
}

bool regressionSlopeCountsPerSec(const float *buf, int n, float &slopeCps){
  if(n<2) return false;
  const float mean_x = 0.5f*(n-1);
  float mean_y=0.0f;
  for(int i=0;i<n;++i) mean_y += buf[i];
  mean_y/=n;
  float num=0.0f;
  for(int i=0;i<n;++i){
    float dx = (float)i - mean_x;
    num += dx * (buf[i]-mean_y);
  }
  const float den = (float)n * ((float)n*(float)n - 1.0f) / 12.0f;
  if(den<=0.0f) return false;
  float slope_per_sample = num/den;
  slopeCps = slope_per_sample * (1000.0f/(float)SAMPLE_PERIOD_MS);
  return true;
}

// ---------- Setup ----------
void setup(){
  Serial.begin(115200);
  lv_helper();
  ui_init();
  analogWrite(38, 220); // Display-Helligkeit
  pinMode(sensorPin, INPUT);     
  analogReadResolution(12);           // 12 Bit
  analogSetAttenuation(ADC_11db);     // Messbereich bis 3.3 V

  for(int i=0;i<MEDIAN_N;++i) medBuf[i]=0;
  for(int i=0;i<REG_N; ++i) regBuf[i]=NAN;

  lastSampleMs = millis();
  bootStartMs  = lastSampleMs;

  // (Optional) Padding des Bars entfernen, damit Overlay exakt deckt
  lv_obj_set_style_pad_all(ui_Bar3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Overlay für Bar3: volle Größe, ohne Padding/Borders, zeichnet im DRAW_MAIN die Schraffur
  ui_Bar3Overlay = lv_obj_create(ui_Bar3);
  lv_obj_set_size(ui_Bar3Overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_pad_all(ui_Bar3Overlay, 0, LV_PART_MAIN | LV_STATE_DEFAULT);     // kein Padding
  lv_obj_set_style_bg_opa(ui_Bar3Overlay, LV_OPA_TRANSP,   LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(ui_Bar3Overlay, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_clear_flag(ui_Bar3Overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ui_Bar3Overlay, bar3_hatch_draw_event_cb, LV_EVENT_DRAW_MAIN, NULL);
  lv_obj_set_style_opa(ui_Bar3Overlay, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT); // Anfangs aus

  // Initialer Status (6k) bis Trend erkannt wird
  setStatusLabel(false);

  Serial.println("Start: Nur Schraffur blinkt im kritischen Zustand; Indicator bleibt statisch (rot bei kritisch)");
}

// ---------- Loop ----------
void loop(){
  lv_task_handler();
  delay(5);

  const uint32_t now = millis();
  if((uint32_t)(now - lastSampleMs) < SAMPLE_PERIOD_MS) return;
  lastSampleMs += SAMPLE_PERIOD_MS;

  // 1) Rohmessung
  int raw = clamp12b(analogRead(sensorPin));
  Serial.println(analogRead(sensorPin));
  // 2) Median-of-5
  medBuf[medIdx] = (int16_t)raw;
  medIdx = (medIdx + 1) % MEDIAN_N;
  if(medIdx==0) medFilled=true;
  int med = medFilled ? median5(medBuf) : raw;

  // 3) EMA
  ema = doEMA(ema, (float)med, alphaEMA);

  // 4) Regression-Puffer
  regBuf[regIdx] = ema;
  regIdx = (regIdx + 1) % REG_N;
  if(regCount < REG_N) regCount++;

  // ---- Startfenster-/Toleranzlogik ----
  if(!bootRefSet && !isnan(ema)) {
    bootRefCounts = ema;
    bootRefSet    = true;
  }
  if(bootRefSet && !significantChange && (now - bootStartMs) <= NO_CHANGE_WINDOW_MS) {
    float dCounts = fabsf(ema - bootRefCounts);
    if(dCounts > noChangeTolCounts) significantChange = true;
  }

  // 5) Trend berechnen
  float slopeCps = 0.0f;
  bool  trendOk  = regressionSlopeCountsPerSec(regBuf, regCount, slopeCps);

  // 6) Druck & Restgas
  float P_bar   = ema * druckFaktor;
  float V_restL = (P_bar - P_rest) * bottleLiters;
  if(V_restL < 0) V_restL = 0;

  // 7) Trend-basierte Zeit (falls signifikanter Abfall)
  float t_trend_minutes = NAN;
  if(trendOk && slopeCps < minSlopeThreshold){
    float slope_bar_per_s = slopeCps * druckFaktor;
    float flow_from_pressure_Lpm = (-slope_bar_per_s) * bottleLiters * 60.0f;
    if(flow_from_pressure_Lpm > 0.05f){
      t_trend_minutes = V_restL / flow_from_pressure_Lpm;
      if(!(t_trend_minutes > 0.0f && t_trend_minutes < 600.0f)) t_trend_minutes = NAN;
    }
  }

  // 8) Mindestzeit (6000 m) – konservativ (Fallback, nie "--:--")
  const float flow_pp_6000 = edsFlowLpm_at6000m(); // ~0.89 L/min
  const float flow_tot_min = flow_pp_6000 * persons;
  float t_min_minutes = (flow_tot_min > 0.05f) ? (V_restL / flow_tot_min) : 0.0f;

  // 9) Zustände / Anzeige
  const bool trendActive  = !isnan(t_trend_minutes);
  float t_show            = trendActive ? t_trend_minutes : t_min_minutes;

  // Kritisch wenn (a) Mindestzeit < T_warn_minutes ODER (b) Druck < P_rest
  bool timeCritical     = (t_min_minutes > 0.0f) && (t_min_minutes < T_warn_minutes);
  bool pressureCritical = (P_bar < P_rest);
  bool critical         = timeCritical || pressureCritical;

  // Status-Label aktualisieren
  setStatusLabel(trendActive);

  // 10) Format & Anzeige (Zeit)
  if(t_show < 0) t_show = 0;
  int h = (int)(t_show / 60.0f);
  int m = (int)roundf(t_show) % 60;
  String timeString = String(h) + ":" + (m < 10 ? "0" : "") + String(m);
  lv_label_set_text(ui_Label3, timeString.c_str());   // Restlaufzeit

  // **Aktueller Druck** in Label1 (Text + Farbe; kritischer Zustand überschreibt Druckfarben)
  lv_label_set_text(ui_Label1, String(P_bar, 0).c_str());
  updatePressureLabelColor(P_bar, critical);

  // **Balken** Bar3 (Wert + Farbe, nur Schraffur blinkt bei kritisch)
  int barVal = (int)roundf((P_bar / BAR_FULL_SCALE_BAR) * 100.0f);
  if(barVal < 0)   barVal = 0;
  if(barVal > 100) barVal = 100;
  lv_anim_enable_t anim = (lv_anim_enable_t)false;
  lv_bar_set_value(ui_Bar3, barVal, anim);
  updateBarColorAndBlink(P_bar, critical, now);

  // Optional Debug:
  // Serial.printf("P=%.1f bar | Vrest=%.0f L | t_trend=%.1f | t_min=%.1f | t_show=%.1f | critical=%d\n",
  //               P_bar, V_restL, t_trend_minutes, t_min_minutes, t_show, critical);
}
