#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>

const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";

const int BUTTON_NEXT = 14;
const int BUTTON_PREV = 0;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

int  prevHour         = -1;
int  prevMinute       = -1;
bool flipped          = false;
bool lastBtnNextState = HIGH;
bool lastBtnPrevState = HIGH;

#define CARD_BG      0x1082
#define CARD_TEXT    TFT_WHITE
#define CARD_DIVIDER 0x3186
#define SCREEN_BG    TFT_BLACK
#define SEG_ON       TFT_WHITE
#define SEG_OFF      0x2104

// =====================
// --- NTP / TIME ---
// =====================

bool isDST(struct tm* t) {
  int month = t->tm_mon + 1;
  int day   = t->tm_mday;
  int dow   = t->tm_wday;
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;
  int lastSunday = day - dow;
  if (month == 3)  return lastSunday >= 25;
  if (month == 10) return lastSunday < 25;
  return false;
}

void getLondonTime(struct tm* out) {
  time_t now; time(&now);
  struct tm* utc = gmtime(&now);
  int offsetMins = isDST(utc) ? 60 : 0;
  time_t local = now + (offsetMins * 60);
  *out = *gmtime(&local);
}

void setupNTP() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = 0;
  int attempts = 0;
  while (now < 1000000000L && attempts < 20) {
    delay(500);
    time(&now);
    attempts++;
  }
}

// =====================
// --- 7-SEGMENT ---
// =====================
//
//  Segments:
//    0 = top
//    1 = top-right
//    2 = bottom-right
//    3 = bottom
//    4 = bottom-left
//    5 = top-left
//    6 = middle
//
//  Which segments are on for each digit 0-9:
//
bool segMap[10][7] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}, // 9
};

// Draw a single 7-segment digit into the sprite
// x, y = top-left of digit bounding box
// w, h = digit size
// t    = segment thickness
void drawDigit(int x, int y, int w, int h, int t, int digit, uint16_t onCol, uint16_t offCol) {
  if (digit < 0 || digit > 9) return;

  bool* segs = segMap[digit];
  int r = t / 2; // corner radius for segments

  // Horizontal segments (top, middle, bottom)
  // top
  sprite.fillRoundRect(x + t,         y,           w - t*2, t, r, segs[0] ? onCol : offCol);
  // middle
  sprite.fillRoundRect(x + t,         y + h/2 - t/2, w - t*2, t, r, segs[6] ? onCol : offCol);
  // bottom
  sprite.fillRoundRect(x + t,         y + h - t,   w - t*2, t, r, segs[3] ? onCol : offCol);

  // Vertical segments (top-left, top-right, bottom-left, bottom-right)
  // top-left
  sprite.fillRoundRect(x,             y + t,       t, h/2 - t - t/2, r, segs[5] ? onCol : offCol);
  // top-right
  sprite.fillRoundRect(x + w - t,     y + t,       t, h/2 - t - t/2, r, segs[1] ? onCol : offCol);
  // bottom-left
  sprite.fillRoundRect(x,             y + h/2 + t/2, t, h/2 - t - t/2, r, segs[4] ? onCol : offCol);
  // bottom-right
  sprite.fillRoundRect(x + w - t,     y + h/2 + t/2, t, h/2 - t - t/2, r, segs[2] ? onCol : offCol);
}

// =====================
// --- DRAW CARD ---
// =====================

void drawCard(int x, int y, int w, int h,
              int tens, int units,
              bool flipping, float flipProgress) {
  int cy = y + h / 2;

  // Card background
  sprite.fillRoundRect(x, y, w, h, 8, CARD_BG);

  // Segment thickness proportional to card size
  int t    = max(6, w / 14);
  // Each digit takes roughly half the card width minus a gap
  int gap  = t;
  int dw   = (w - gap * 3) / 2;  // digit width
  int dh   = h - t * 2;           // digit height
  int dy   = y + t;               // digit top

  int dx1  = x + gap;             // tens digit x
  int dx2  = x + gap * 2 + dw;   // units digit x

  if (!flipping) {
    drawDigit(dx1, dy, dw, dh, t, tens,  SEG_ON, SEG_OFF);
    drawDigit(dx2, dy, dw, dh, t, units, SEG_ON, SEG_OFF);
  } else {
    // Always draw full digits
    drawDigit(dx1, dy, dw, dh, t, tens,  SEG_ON, SEG_OFF);
    drawDigit(dx2, dy, dw, dh, t, units, SEG_ON, SEG_OFF);

    // Cover bottom half with descending flap
    int flapH = (int)((1.0f - flipProgress) * (h / 2 - 1));
    if (flapH > 0) {
      sprite.fillRect(x + 2, cy + 1, w - 4, flapH, 0x3186);
      sprite.fillRect(x + 2, cy + flapH, w - 4, 2, 0x2104);
    }
  }

  // Divider line across the middle
  sprite.fillRect(x, cy - 2, w, 4, CARD_DIVIDER);
}

void drawColon(int x, int y, int h) {
  int r = 7;
  int q = h / 4;
  sprite.fillCircle(x, y + q,     r, 0x4228);
  sprite.fillCircle(x, y + q * 3, r, 0x4228);
}

void drawClock(int hour, int minute,
               bool flipH, bool flipM,
               float flipProg) {

  sprite.fillSprite(SCREEN_BG);

  int margin = 6;
  int colonW = 20;
  int cardH  = 170 - margin * 2;
  int cardW  = (320 - margin * 3 - colonW) / 2;
  int cardY  = margin;
  int xH     = margin;
  int xC     = xH + cardW + margin / 2;
  int xM     = xC + colonW;

  drawCard(xH, cardY, cardW, cardH,
           hour / 10, hour % 10,
           flipH, flipProg);

  drawColon(xC + colonW / 2, cardY, cardH);

  drawCard(xM, cardY, cardW, cardH,
           minute / 10, minute % 10,
           flipM, flipProg);

  sprite.pushSprite(0, 0);
}

void runFlip(int h, int m, bool changeH, bool changeM) {
  const int FRAMES   = 10;
  const int FRAME_MS = 25;
  for (int f = 0; f <= FRAMES; f++) {
    float prog = (float)f / FRAMES;
    drawClock(h, m, changeH, changeM, prog);
    delay(FRAME_MS);
  }
}

// =====================
// --- SETUP ---
// =====================

void setup() {
  USBSerial.begin(115200);
  delay(2000);

  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_PREV, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  sprite.createSprite(320, 170);

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Connecting...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.println("WiFi failed");
    return;
  }

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("Syncing NTP...");
  setupNTP();

  struct tm t;
  getLondonTime(&t);
  prevHour   = t.tm_hour;
  prevMinute = t.tm_min;
  drawClock(prevHour, prevMinute, false, false, 1.0f);
}

// =====================
// --- LOOP ---
// =====================

void loop() {
  bool btnNext = digitalRead(BUTTON_NEXT);
  bool btnPrev = digitalRead(BUTTON_PREV);

  if ((btnNext == LOW && lastBtnNextState == HIGH) ||
      (btnPrev == LOW && lastBtnPrevState == HIGH)) {
    delay(50);
    if (digitalRead(BUTTON_NEXT) == LOW ||
        digitalRead(BUTTON_PREV) == LOW) {
      flipped = !flipped;
      tft.setRotation(flipped ? 3 : 1);
      struct tm t;
      getLondonTime(&t);
      drawClock(t.tm_hour, t.tm_min, false, false, 1.0f);
    }
  }
  lastBtnNextState = btnNext;
  lastBtnPrevState = btnPrev;

  struct tm t;
  getLondonTime(&t);
  int h = t.tm_hour;
  int m = t.tm_min;

  bool changeH = (h != prevHour);
  bool changeM = (m != prevMinute);

  if (changeH || changeM) {
    runFlip(h, m, changeH, changeM);
    prevHour   = h;
    prevMinute = m;
  }

  delay(200);
}
