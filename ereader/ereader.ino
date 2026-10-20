/*
  Proxlight Ink  -  XIAO ePaper Display Board EE05 + 7.5" mono 800x480  -  EPUB e-reader  (v2.0)

  HOW IT WORKS
   1. The reader creates its OWN Wi-Fi network (no router needed):
          Wi-Fi name : Proxlight-Ink
          Password   : proxlight123
   2. Join that network with your phone/laptop and open  http://192.168.4.1  (or http://proxlight.local)
   3. On the web page you can:
        - remote control (next / prev page, chapters, text size, go-to-page)
        - choose an EPUB (or .txt). It is converted IN YOUR BROWSER (text formatting + images turned
          into dithered 1-bit pictures), previewed, and then sent. The reader deletes the previous
          book's files first, then formats the new book before showing it.
   4. The 3 buttons on the board also work:
        KEY1 = previous page (hold = previous chapter)
        KEY2 = text size     (hold = show Wi-Fi / connection info)
        KEY3 = next page     (hold = next chapter)

  ARDUINO IDE SETTINGS (Tools menu):  Board: XIAO_ESP32S3 | PSRAM: OPI PSRAM |
                                       Partition Scheme: "8M with spiffs (3MB APP/1.5MB SPIFFS)"
*/
#include <Arduino.h> 
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <LittleFS.h>
#include <vector>
#include <algorithm>
#if __has_include("soc/usb_serial_jtag_reg.h")
#include "soc/usb_serial_jtag_reg.h"   // lets us see whether a USB host is attached (charging detection)
#endif
#include "TFT_eSPI.h"      // from the Seeed_GFX library
#include "page.h"          // the remote-control web page

// ================= SETTINGS YOU MAY CHANGE =================
#define AP_SSID  "Proxlight-Ink"   // the Wi-Fi network the reader creates
#define AP_PASS  "proxlight123"    // at least 8 characters
#define AP_CHANNEL 6
#define HOST_NAME "proxlight"      // http://proxlight.local
#define BRAND_WEB  "Proxlight Ink"
#define BRAND_FOOT "Proxlight"
#define SCREEN_ROTATION 1      // 0,1,2,3 -> pick the one where text is upright
#define PIN_PREV 2             // KEY1
#define PIN_MENU 3             // KEY2
#define PIN_NEXT 5             // KEY3
#define BTN_PRESSED LOW        // change to HIGH if buttons act inverted
#define ENABLE_BUTTONS 0       // 0 = control only from the phone (web page). Set 1 to also use KEY1/KEY2/KEY3
#define LONG_PRESS_MS 1000
// --- LiPo battery (voltage divider on the EE0x board; pins per Seeed's EE0x docs - change if your readings look wrong) ---
#define SHOW_BATTERY 1
#define BAT_ADC_PIN 1          // A0 / GPIO1  : battery voltage (through a 1/2 divider)
#define BAT_EN_PIN  6          // D5 / GPIO6  : HIGH = connect the divider (switched on only while measuring)
#define BAT_DIV 2.0            // divider ratio
#define BAT_CAL 1.00           // fine tune: multiply readings (e.g. 1.03 if a multimeter says the pack is 3% higher)
#define BAT_CHG_OFFSET_MV 90   // start value: while charging the pin reads this much too high. It is LEARNED from every plug/unplug.
#define BAT_LOAD_COMP_MV 30    // the always-on Wi-Fi load makes the voltage sag; this is added back so % matches the resting voltage
#define BAT_CRITICAL_MV 3300   // below this the reader shows "charge me" and deep-sleeps to protect the LiPo
// --- power saving (450 mAh LiPo) ---
#define WIFI_MAX_CLIENTS 1         // one phone at a time; another can join after the first disconnects
#define BAT_MAH 450                // informational
#define CPU_MHZ 80             // 80 is plenty for an e-reader (240 = default, uses ~2x more power)
#define WIFI_TX_DBM_ENUM WIFI_POWER_11dBm    // 11 dBm is reliable for any phone in the room. (5 dBm saved almost nothing and made some phones fail to connect.)
#define MARGIN 20
#define FOOTER_H 34
#define SAFE_PX 8              // extra space kept free at the right edge of every text line
// ===========================================================
struct Head { bool chapter; char style; int img; uint32_t body; };   // (kept above all functions for the Arduino IDE)
#define NSIZES 3               // text sizes: small / medium / large

// Fonts: [0]=body, [1]=bold (headings), [2]=italic (quotes); one column per text size
enum { F_REG = 0, F_BOLD = 1, F_ITAL = 2 };
static const GFXfont *FONTS[3][NSIZES] = {
  {&FreeSerif9pt7b,      &FreeSerif12pt7b,      &FreeSerif18pt7b},
  {&FreeSerifBold12pt7b, &FreeSerifBold18pt7b,  &FreeSerifBold24pt7b},
  {&FreeSerifItalic9pt7b,&FreeSerifItalic12pt7b,&FreeSerifItalic18pt7b}
};

/*  BOOK FORMAT (book.txt, produced by the web page). One paragraph per line:
      \001 at the start of a paragraph = a new chapter starts here
      \002<n>   = picture number n  (file /i<n>.bin)
      \003 heading   \005 centered   \006 quote   \007 list item   \010 continuation (no indent)
      no marker      = normal paragraph (first line indented)                                      */
static const char DEMO[] PROGMEM =
  "\001\003Welcome to Proxlight Ink\n"
  "If you can read this, your e-reader is working. This is the built-in demo book.\n"
  "\005- - -\n"
  "\006This reader is controlled from your phone: use the Next / Prev buttons on the web page.\n"
  "\001\003Load your own book\n"
  "On your phone, join the Wi-Fi network \"" AP_SSID "\" (password \"" AP_PASS "\"), then open http://192.168.4.1\n"
  "\007Choose an EPUB file\n"
  "\007Check the preview\n"
  "\007Press Send to e-reader\n"
  "Images are converted to black and white automatically. Your place in the book is saved.\n";

EPaper epaper;
Preferences prefs;
WebServer server(80);
DNSServer dnsServer;               // answers every name with the reader's address, so any phone address bar works
std::vector<uint32_t> pageStart, chOff;
std::vector<uint16_t> imgW, imgH;
const char *BOOK = nullptr;        // set only when the whole book fits in RAM (PSRAM); otherwise it is streamed from flash
uint32_t BOOK_LEN = 0;
char *bookBuf = nullptr;
File bookFile;                     // streamed book
bool fromFile = false;
char win[2048]; uint32_t winStart = 0, winLen = 0;
int bookSrc = 0;                   // 0 = demo book, 1 = your book, 2 = your book could not be opened
String bookErr = "";
int batPct = -1, batMv = 0; bool batCharging = false;        // -1 = no battery reading. Only refreshed when a page is drawn (no extra screen refreshes)
volatile bool redrawFlag = false;
uint32_t lastActivity = 0;         // last command from the web page / upload (page polling does NOT count)
void touch() { lastActivity = millis(); }
int sizeIdx = 0, page = 0;
int16_t CW[3][95];                 // glyph advance widths for ASCII 32..126, per font kind
int LH[3];                         // line height per font kind
String ipStr = "";
bool infoShown = false, uploadOk = true;
volatile bool reloadFlag = false;
File upFile;

void goToSleepEmpty();
// ---------- book files ----------
bool isBookFile(String n) {
  if (n.startsWith("/")) n.remove(0, 1);
  return n == "book.txt" || (n.length() > 5 && n[0] == 'i' && n.endsWith(".bin"));
}
// Deletes the previous book (text + all pictures) so nothing old stays cached. Returns bytes freed.
size_t wipeBook() {
  std::vector<String> names; size_t bytes = 0;
  File root = LittleFS.open("/");
  if (root) {
    File f = root.openNextFile();
    while (f) {
      String n = f.name();
      if (isBookFile(n)) { names.push_back(n.startsWith("/") ? n : "/" + n); bytes += f.size(); }
      f = root.openNextFile();
    }
    root.close();
  }
  for (auto &n : names) LittleFS.remove(n);
  return bytes;
}
// Full clean: formats the storage so free space is ALWAYS exactly the same (also removes any cache/leftover file).
bool cleanStorage() {
  LittleFS.end();
  bool ok = LittleFS.format();
  if (!LittleFS.begin(true)) ok = false;
  if (!ok) wipeBook();             // fallback: at least delete the book files
  Serial.printf("Storage cleaned (%s). Free: %u bytes\n", ok ? "format" : "file delete", (unsigned)(LittleFS.totalBytes() - LittleFS.usedBytes()));
  return ok;
}
size_t bookBytes() {
  size_t bytes = 0;
  File root = LittleFS.open("/");
  if (!root) return 0;
  File f = root.openNextFile();
  while (f) { if (isBookFile(String(f.name()))) bytes += f.size(); f = root.openNextFile(); }
  root.close();
  return bytes;
}

// One character of the book (from RAM, from the demo, or streamed from the flash file through a small window)
char B(uint32_t i) {
  if (BOOK) return BOOK[i];
  if (!fromFile) return (char)pgm_read_byte(&DEMO[i]);
  if (i < winStart || i >= winStart + winLen) {
    winStart = (i > 512) ? i - 512 : 0;
    bookFile.seek(winStart);
    winLen = bookFile.read((uint8_t *)win, sizeof win);
    if (i < winStart || i >= winStart + winLen) return '\n';
  }
  return win[i - winStart];
}
void loadBook() {
  if (bookBuf) { free(bookBuf); bookBuf = nullptr; }
  if (bookFile) bookFile.close();
  BOOK = nullptr; fromFile = false; winLen = 0; winStart = 0;
  BOOK_LEN = strlen_P(DEMO); bookSrc = 0; bookErr = "";
  if (LittleFS.exists("/book.txt")) {
    bookFile = LittleFS.open("/book.txt", "r");
    size_t n = bookFile ? bookFile.size() : 0;
    if (n > 0) {
      if (psramFound() && (bookBuf = (char *)ps_malloc(n + 1)) != nullptr) {      // fast path: whole book in PSRAM
        bookFile.seek(0);
        size_t got = bookFile.read((uint8_t *)bookBuf, n);
        bookBuf[got] = 0; BOOK = bookBuf; BOOK_LEN = got; bookSrc = 1;
        bookFile.close();
      } else {                                                                    // no PSRAM: read from flash as needed
        fromFile = true; BOOK_LEN = n; bookSrc = 1;
      }
    } else { bookSrc = 2; bookErr = "book.txt is empty"; if (bookFile) bookFile.close(); }
  }
  chOff.clear();
  for (uint32_t i = 0; i < BOOK_LEN; i++) if (B(i) == 1) chOff.push_back(i);
  if (chOff.empty() || chOff[0] != 0) chOff.insert(chOff.begin(), 0);
  // picture sizes (read once)
  imgW.clear(); imgH.clear();
  for (int n = 0; n < 4000; n++) {
    char p[24]; snprintf(p, sizeof p, "/i%d.bin", n);
    if (!LittleFS.exists(p)) break;
    File f = LittleFS.open(p, "r"); uint8_t h[4] = {0, 0, 0, 0};
    if (f) { f.read(h, 4); f.close(); }
    imgW.push_back(h[0] | (h[1] << 8)); imgH.push_back(h[2] | (h[3] << 8));
  }
  Serial.printf("Book: source=%d, %u chars, %u chapters, %u pictures%s\n", bookSrc, (unsigned)BOOK_LEN, (unsigned)chOff.size(), (unsigned)imgW.size(), (BOOK ? " [in RAM]" : " [streamed]"));
}
// Called before the old book's files are deleted: closes the file and falls back to the demo state.
void resetToDemoState() {
  if (bookBuf) { free(bookBuf); bookBuf = nullptr; }
  if (bookFile) bookFile.close();
  BOOK = nullptr; fromFile = false; winLen = 0; winStart = 0; bookSrc = 0;
  BOOK_LEN = strlen_P(DEMO);
  pageStart.assign(1, 0); chOff.assign(1, 0); imgW.clear(); imgH.clear(); page = 0;
}
int chCount() { return (int)chOff.size(); }
int chapterOf(uint32_t off) {
  int c = 0;
  for (int i = 0; i < chCount(); i++) if (chOff[i] <= off) c = i; else break;
  return c;
}
int pageOfChapter(int c) {
  for (size_t i = 0; i < pageStart.size(); i++) if (pageStart[i] >= chOff[c]) return i;
  return pageStart.size() - 1;
}

// ---------- battery ----------
static const uint16_t LIPO_MV[] = {4200,4150,4110,4080,4020,3980,3950,3910,3870,3850,3840,3820,3800,3790,3770,3750,3730,3710,3690,3610,3270};
static const uint8_t  LIPO_PC[] = { 100,  95,  90,  85,  80,  75,  70,  65,  60,  55,  50,  45,  40,  35,  30,  25,  20,  15,  10,   5,   0};
int lipoPercent(int mv) {
  if (mv >= LIPO_MV[0]) return 100;
  for (int i = 1; i < (int)sizeof(LIPO_MV) / 2; i++)
    if (mv >= LIPO_MV[i]) return LIPO_PC[i] + (LIPO_PC[i - 1] - LIPO_PC[i]) * (mv - LIPO_MV[i]) / (LIPO_MV[i - 1] - LIPO_MV[i]);
  return 0;
}
int readBatteryMv() {              // 0 = not available
#if SHOW_BATTERY
  pinMode(BAT_EN_PIN, OUTPUT); digitalWrite(BAT_EN_PIN, HIGH);   // connect the divider only while measuring
  delay(10);
  // Wi-Fi bursts and screen refreshes make the voltage SAG for a few ms. Sag only goes downwards, so take
  // 24 quick samples and use the 75th percentile (ignores the dips) instead of the average.
  uint16_t smp[24];
  for (int i = 0; i < 24; i++) { smp[i] = analogReadMilliVolts(BAT_ADC_PIN); delay(3); }
  std::sort(smp, smp + 24);
  digitalWrite(BAT_EN_PIN, LOW); pinMode(BAT_EN_PIN, INPUT);
  int mv = (int)(smp[18] * BAT_DIV * BAT_CAL);
  return (mv > 2500 && mv < 4700) ? mv : 0;
#else
  return 0;
#endif
}
// There is no charge-status pin on this board, so charging is detected from the voltage:
//  - at/above 4.20 V (only possible with a charger attached), or
//  - a sudden jump up (+50 mV, USB just plugged in) / jump down (-50 mV, unplugged), or
//  - a steady rise over the last ~6 minutes.
static int hist[4] = {0, 0, 0, 0}, lastMv = 0;
static bool usbSeen = false, haveUsbState = false;
static int chgOff = BAT_CHG_OFFSET_MV, usbMiss = 0;      // chgOff: charger voltage boost in mV (learned)
static uint32_t lastReadMs = 0, lastPctMs = 0;
static int ring[4] = {0, 0, 0, 0}, ringN = 0, dropCnt = 0;   // last readings (to tell a sag-and-recover from a real plug-in)
// USB power detection: while a host (PC, laptop, phone) is attached, the ESP32-S3 USB block counts 1 ms
// frames. If that counter moves, USB is plugged in -> we are charging. (A plain wall charger sends no
// frames, so for those the voltage rules below still apply.)
bool usbHostPresent() {
#ifdef USB_SERIAL_JTAG_FRAM_NUM_REG
  uint32_t a = REG_READ(USB_SERIAL_JTAG_FRAM_NUM_REG) & 0x7FF;
  delay(5);
  uint32_t b = REG_READ(USB_SERIAL_JTAG_FRAM_NUM_REG) & 0x7FF;
  return a != b;
#else
  return false;
#endif
}
// Charging state + battery percentage.
//  * USB host attached (frame counter moving) -> charging for sure.
//  * Wall charger (no USB frames): voltage jump +/-40 mV, >= 4.20 V, or a steady rise over ~6 min.
//  * The charger lifts the pin voltage by "chgOff" mV. At every plug/unplug we MEASURE that step (the load is
//    the same just before/after) and learn it, so the percentage is the same plugged or unplugged.
//  * The shown percentage never jumps: it moves at most 1-3 % per minute, and not at all within 20 s of its
//    last change (page turns / Wi-Fi sag can't make it bounce).
void applyReading(int mv) {
  bool usb = usbHostPresent();
  uint32_t now = millis();
  if (usb) usbMiss = 0; else if (usbMiss < 5) usbMiss++;
  bool usbNow = usb || (usbSeen && usbMiss < 2 && !(lastMv - mv >= 20));   // one missed frame check alone isn't "unplugged" (unless the voltage also fell)
  if (haveUsbState && usbNow != usbSeen && lastMv > 0 && now - lastReadMs < 600000UL) {   // plug / unplug: learn the boost
    int step = usbNow ? (mv - lastMv) : (lastMv - mv);
    if (step >= 15 && step <= 300) chgOff = (chgOff + step) / 2;
  }
  if (usbNow) batCharging = true;
  else if (usbSeen) { batCharging = false; for (int i = 0; i < 4; i++) hist[i] = 0; }   // just unplugged
  else {                                                         // no USB host: wall charger or battery
    if (ringN >= 2) {
      int mx = max(max(ring[0], ring[1]), max(ring[2], ring[3]));
      if (mv >= mx + 40) { batCharging = true; dropCnt = 0; }                     // clearly above everything recent: charger plugged in
      else if (batCharging && mv <= mx - 40) { if (++dropCnt >= 2) batCharging = false; }   // 2 readings 40 mV lower: unplugged
      else dropCnt = 0;                                                            // (a sag that recovers is ignored)
    }
    if (mv >= 4200) batCharging = true;
  }
  for (int i = 0; i < 3; i++) ring[i] = ring[i + 1];
  ring[3] = mv; if (ringN < 4) { ringN++; for (int i = 0; i < 4 - ringN; i++) ring[i] = mv; }
  haveUsbState = true; usbSeen = usbNow; lastMv = mv; lastReadMs = now; batMv = mv;

  int adj = mv + BAT_LOAD_COMP_MV;
  if (batCharging && mv < 4200) { int off = 4200 - mv; if (off > chgOff) off = chgOff; adj = mv - off + BAT_LOAD_COMP_MV; }
  int pc = lipoPercent(adj);
  if (batPct < 0 || pc <= 5) { batPct = pc; lastPctMs = now; return; }
  uint32_t dt = now - lastPctMs;
  if (dt < 20000UL || pc == batPct) return;                      // too soon after the last change: keep the number steady
  int step = 1 + (int)(dt / 60000UL) * 2; if (step > 8) step = 8;
  if (pc > batPct) { if (!batCharging && step > 1) step = 1; batPct = min(pc, batPct + step); }   // rises quickly only while charging
  else batPct = max(pc, batPct - step);
  lastPctMs = now;
}
void pushTrend(int mv) {           // called every ~2 min
  for (int i = 0; i < 3; i++) hist[i] = hist[i + 1];
  hist[3] = mv;
  if (usbSeen) return;                                          // USB host present: already known
  if (hist[0] > 0 && hist[3] - hist[0] >= 12) batCharging = true;       // rising for ~6 min: charger attached
  else if (hist[0] > 0 && hist[0] - hist[3] >= 12) batCharging = false; // falling: running on battery
}
bool updateBattery() {             // returns true if the battery is critically low (never while charging)
  int mv = readBatteryMv();
  if (mv == 0) { batPct = -1; batMv = 0; batCharging = false; return false; }
  applyReading(mv);
  Serial.printf("Battery: %d mV -> %d%% %s (charger boost %d mV)\n", mv, batPct, batCharging ? "(charging)" : "", chgOff);
  return mv < BAT_CRITICAL_MV && !batCharging;
}

// ---------- fonts ----------
void initFonts() {                 // measure the glyphs of the current text size once
  for (int k = 0; k < 3; k++) {
    epaper.setFreeFont(FONTS[k][sizeIdx]);
    LH[k] = epaper.fontHeight();
    if (LH[k] < 10) LH[k] = 10;
    // textWidth() of a LAST character returns its ink width, not its advance. So measure "c." and
    // subtract the width of "." : that leaves the true advance of c (this was causing the cropped right edge).
    int dot = epaper.textWidth(".");
    char s[3] = {0, '.', 0};
    for (int c = 32; c < 127; c++) {
      s[0] = (char)c;
      int a = epaper.textWidth(s) - dot;
      CW[k][c - 32] = (a > 0 && a < 100) ? a : 0;
    }
    if (CW[k][0] == 0) CW[k][0] = LH[k] / 4;               // space
  }
}
static inline int cwOf(int k, char c) { return (c >= 32 && c < 127) ? CW[k][c - 32] : CW[k][0]; }

// ---------- layout / drawing ----------
Head parseHead(uint32_t p0) {
  Head h = {false, 0, -1, p0};
  uint32_t i = p0;
  if (i < BOOK_LEN && B(i) == 1) { h.chapter = true; i++; }
  if (i < BOOK_LEN) {
    char c = B(i);
    if (c == 2) { i++; int n = 0; while (i < BOOK_LEN && B(i) >= '0' && B(i) <= '9') { n = n * 10 + (B(i) - '0'); i++; } h.img = n; }
    else if (c == 3 || c == 5 || c == 6 || c == 7 || c == 8) { h.style = c; i++; }
  }
  h.body = i;
  return h;
}

void drawImage(int n, int x, int y) {
  char p[24]; snprintf(p, sizeof p, "/i%d.bin", n);
  File f = LittleFS.open(p, "r");
  if (!f) return;
  uint8_t hd[4];
  if (f.read(hd, 4) != 4) { f.close(); return; }
  int w = hd[0] | (hd[1] << 8), h = hd[2] | (hd[3] << 8);
  size_t sz = (size_t)((w + 7) / 8) * h;
  uint8_t *b = (uint8_t *)(psramFound() ? ps_malloc(sz) : malloc(sz));
  if (b) {
    size_t got = f.read(b, sz);
    if (got == sz) epaper.drawBitmap(x, y, b, w, h, TFT_BLACK);
    free(b);
  }
  f.close();
}

// Lays out one page starting at 'pos'. Draws it if draw==true; copies its text into *out if given.
// Returns where the next page starts. Pages are built in pixels: text lines, headings and pictures.
static bool fullPicPage = false;      // true when the page drawn last is one big picture (no footer)
uint32_t layoutPage(uint32_t pos, bool draw, String *out = nullptr) {
  const int W = epaper.width();
  const int bottom = epaper.height() - FOOTER_H - 6;
  int y = MARGIN;
  bool first = true, full = false;
  char buf[256];
  while (pos < BOOK_LEN && !full) {
    bool atStart = (pos == 0 || B(pos - 1) == '\n');
    uint32_t p0 = pos;
    if (!atStart) while (p0 > 0 && B(p0 - 1) != '\n') p0--;
    Head h = parseHead(p0);
    if (atStart && h.chapter && !first) break;                 // chapters start on a new page
    uint32_t tp = atStart ? h.body : pos;

    if (h.img >= 0) {                                          // ----- picture -----
      int iw = (h.img < (int)imgW.size()) ? imgW[h.img] : 0;
      int ih = (h.img < (int)imgH.size()) ? imgH[h.img] : 0;
      if (iw > 0 && ih > 0 && y + ih > bottom && !first) break;
      uint32_t q = tp; while (q < BOOK_LEN && B(q) != '\n') q++;
      pos = (q < BOOK_LEN) ? q + 1 : q;
      if (iw <= 0 || ih <= 0) continue;                        // picture file missing: skip it
      bool bigPic = first && ih > bottom - MARGIN;              // taller than the text area: full-screen picture page
      if (draw) {
        if (bigPic) { fullPicPage = true; drawImage(h.img, (W - iw) / 2, (epaper.height() - ih) / 2); }
        else drawImage(h.img, (W - iw) / 2, y);
      }
      if (out) *out += "[picture]\n";
      y += ih + 8; first = false;
      continue;
    }

    int k = (h.style == 3) ? F_BOLD : (h.style == 6) ? F_ITAL : F_REG;
    int lh = LH[k];
    int left = MARGIN, right = W - MARGIN, firstIndent = 0;
    if (h.style == 0 && atStart) firstIndent = lh;
    else if (h.style == 6) { left += 30; right -= 30; }
    else if (h.style == 7) left += 28;
    bool center = (h.style == 3 || h.style == 5);
    int before = (h.style == 3 && !first) ? lh / 2 : 0;
    int need = before + lh + (h.style == 3 ? 2 * LH[F_REG] : 0);  // keep headings with the text after them
    if (y + need > bottom && !first) break;
    y += before;
    bool firstLine = true;

    while (tp < BOOK_LEN && B(tp) != '\n') {
      if (y + lh > bottom && !first) { full = true; break; }
      int avail = (right - left) - (firstLine ? firstIndent : 0) - SAFE_PX;   // safety margin against clipping
      uint32_t i = tp, lastSp = 0; bool haveSp = false; int w = 0, lineEnd;
      while (i < BOOK_LEN && B(i) != '\n') {
        int cw = cwOf(k, B(i));
        if (w + cw > avail && i > tp) break;
        if (B(i) == ' ') { lastSp = i; haveSp = true; }
        w += cw; i++;
      }
      if (i < BOOK_LEN && B(i) != '\n') {                  // line is full: wrap at a space
        if (B(i) == ' ') lineEnd = i;
        else if (haveSp && lastSp > tp) lineEnd = lastSp;
        else lineEnd = i;
      } else lineEnd = i;
      int len = lineEnd - tp; if (len > 255) len = 255;
      if (draw || out) { for (int j = 0; j < len; j++) buf[j] = B(tp + j); buf[len] = 0; }
      if (draw) {
        int x = left + (firstLine ? firstIndent : 0);
        if (center) { int lw = 0; for (int j = 0; j < len; j++) lw += cwOf(k, buf[j]); x = left + ((right - left) - lw) / 2; }
        epaper.setFreeFont(FONTS[k][sizeIdx]);
        if (h.style == 7 && firstLine) epaper.fillCircle(MARGIN + 9, y + lh / 2, 3, TFT_BLACK);
        epaper.drawString(buf, x, y);
      }
      if (out) { *out += buf; *out += "\n"; }
      y += lh; first = false; firstLine = false;
      tp = lineEnd; while (tp < BOOK_LEN && B(tp) == ' ') tp++;
    }
    if (full) { pos = tp; break; }                            // continue this paragraph on the next page
    pos = (tp < BOOK_LEN) ? tp + 1 : tp;                      // skip the '\n'
    y += (h.style == 3) ? lh / 3 : 5;
  }
  return pos;
}
void paginate() {
  epaper.setTextSize(1);
  initFonts();
  pageStart.clear();
  uint32_t pos = 0;
  while (pos < BOOK_LEN) {
    pageStart.push_back(pos);
    uint32_t nx = layoutPage(pos, false);
    if (nx <= pos) nx = pos + 1;                              // safety: always move forward
    pos = nx;
  }
  if (pageStart.empty()) pageStart.push_back(0);
}
void goToOffset(uint32_t off) {
  page = 0;
  for (size_t i = 0; i < pageStart.size(); i++) if (pageStart[i] <= off) page = i;
}

// Clear the WHOLE screen to white (painted in strips because fillScreen() alone can leave rows black when rotated).
void clearScreen() {
  epaper.fillScreen(TFT_WHITE);
  int w = epaper.width(), h = epaper.height();
  for (int y = 0; y < h; y += 16) epaper.fillRect(0, y, w, 16, TFT_WHITE);
}

void drawFooter() {
  int W = epaper.width(), fy = epaper.height() - FOOTER_H;
  epaper.drawFastHLine(MARGIN, fy, W - 2 * MARGIN, TFT_BLACK);
  int x = MARGIN;
  if (batPct >= 0) {                                          // battery (bolt + box + %) at the far left, small
    char t[8]; snprintf(t, sizeof t, "%d%%", batPct);
    int by = fy + 11;
    if (batCharging && batPct < 99) {                         // lightning bolt = charging (hidden once full)
      int zx = x, zy = by - 1;
      epaper.fillTriangle(zx + 8, zy, zx, zy + 9, zx + 5, zy + 9, TFT_BLACK);
      epaper.fillTriangle(zx + 4, zy + 6, zx + 10, zy + 6, zx + 2, zy + 16, TFT_BLACK);
      x += 14;
    }
    epaper.drawRect(x, by, 28, 14, TFT_BLACK);
    epaper.fillRect(x + 28, by + 4, 3, 6, TFT_BLACK);
    int fillW = (24 * batPct) / 100; if (fillW < 1 && batPct > 0) fillW = 1;
    if (fillW > 0) epaper.fillRect(x + 2, by + 2, fillW, 10, TFT_BLACK);
    epaper.setFreeFont(&FreeSans9pt7b);
    epaper.drawString(t, x + 37, fy + 8);
    x += 37 + epaper.textWidth(t) + 4 + 16;
  }
  char f[48];
  snprintf(f, sizeof f, "Ch %d/%d   Pg %d/%d", chapterOf(pageStart[page]) + 1, chCount(), page + 1, (int)pageStart.size());
  epaper.setFreeFont(&FreeSans9pt7b);
  epaper.drawString(f, x, fy + 8);
  int lw = x + epaper.textWidth(f);                           // right edge of the left block
  epaper.setFreeFont(&FreeSansBold9pt7b);                     // brand, bottom-right
  int bw = epaper.textWidth(BRAND_FOOT);
  epaper.drawString(BRAND_FOOT, W - MARGIN - bw - 4, fy + 8);
  int rx = W - MARGIN - bw - 4;
  epaper.setFreeFont(&FreeSans9pt7b);                         // Wi-Fi hint in the middle if there is room
  char m[48]; snprintf(m, sizeof m, "Wi-Fi: %s  %s", AP_SSID, ipStr.c_str());
  int mw = epaper.textWidth(m), mx = (W - mw) / 2;
  if (mx > lw + 20 && mx + mw < rx - 20) epaper.drawString(m, mx, fy + 8);
}
void drawPage() {
  updateBattery();                                           // battery is measured only when a page is drawn
  clearScreen();
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextDatum(TL_DATUM);
  epaper.setTextSize(1);
  fullPicPage = false;
  layoutPage(pageStart[page], true);
  if (!fullPicPage) drawFooter();                            // full-screen pictures use the whole panel
  epaper.update();                                           // e-paper refresh (takes a few seconds)
  prefs.putUInt("off", pageStart[page]);
}
void showPage(int p) {
  p = constrain(p, 0, (int)pageStart.size() - 1);
  if (p == page) return;
  page = p; drawPage();
}
void setSize(int idx) {
  idx = constrain(idx, 0, NSIZES - 1);
  if (idx == sizeIdx) return;
  uint32_t off = pageStart[page];
  sizeIdx = idx;
  prefs.putUChar("size", sizeIdx);
  paginate(); goToOffset(off); drawPage();
}
// Branded message screen (title + up to 5 lines)
void showMessage(const char *title, const char *l1 = "", const char *l2 = "", const char *l3 = "", const char *l4 = "", const char *l5 = "") {
  clearScreen();
  int W = epaper.width();
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextDatum(TL_DATUM);
  epaper.setTextSize(1);
  epaper.setFreeFont(&FreeSerifBold24pt7b);
  epaper.drawString(BRAND_WEB, MARGIN + 4, 14);
  epaper.drawFastHLine(MARGIN, 72, W - 2 * MARGIN, TFT_BLACK);
  epaper.drawFastHLine(MARGIN, 74, W - 2 * MARGIN, TFT_BLACK);
  epaper.setFreeFont(&FreeSerifBold18pt7b);
  epaper.drawString(title, MARGIN + 4, 90);
  epaper.setFreeFont(&FreeSerif12pt7b);
  const char *L[5] = {l1, l2, l3, l4, l5};
  for (int i = 0; i < 5; i++) epaper.drawString(L[i], MARGIN + 4, 150 + i * 46);
  epaper.update();
}
void showInfo() {
  char a[48], b[48], c[48], d[48];
  snprintf(a, sizeof a, "1. Join Wi-Fi:  %s", AP_SSID);
  snprintf(b, sizeof b, "2. Password:  %s", AP_PASS);
  snprintf(c, sizeof c, "3. Open:  http://%s", ipStr.c_str());
  snprintf(d, sizeof d, "    or  http://%s.local", HOST_NAME);
  showMessage("Connect to your reader", a, b, c, d, "Use the web page to continue");
  infoShown = true;
}

// ---------- commands (from buttons or from the web remote) ----------
void runCmd(const String &c, int arg) {
  if (infoShown) { infoShown = false; drawPage(); return; }  // first press just returns to the book
  int ch = chapterOf(pageStart[page]);
  if (c == "next") showPage(page + 1);
  else if (c == "prev") showPage(page - 1);
  else if (c == "nextch") { if (ch + 1 < chCount()) showPage(pageOfChapter(ch + 1)); }
  else if (c == "prevch") { int t = (page > pageOfChapter(ch)) ? ch : max(ch - 1, 0); showPage(pageOfChapter(t)); }
  else if (c == "first") showPage(0);
  else if (c == "bigger") setSize(sizeIdx + 1);
  else if (c == "smaller") setSize(sizeIdx - 1);
  else if (c == "goto") showPage(arg - 1);
  else if (c == "redraw") drawPage();
}

// ---------- own Wi-Fi network + web server ----------
void startWifi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, 0, WIFI_MAX_CLIENTS);   // 1 phone at a time
  WiFi.setTxPower(WIFI_TX_DBM_ENUM);                         // lower radio power = longer battery life
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);          // 20 MHz channel: less radio power
  esp_wifi_set_inactive_time(WIFI_IF_AP, 20);                // free the single slot ~20 s after the phone leaves
  delay(200);
  ipStr = WiFi.softAPIP().toString();
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
  if (MDNS.begin(HOST_NAME)) MDNS.addService("http", "tcp", 80);
  Serial.printf("Wi-Fi network \"%s\" is up. Open http://%s\n", AP_SSID, ipStr.c_str());
}

void handleState() {   // first line: numbers, then the text currently on the screen
  char h[128];
  snprintf(h, sizeof h, "%d|%d|%d|%d|%d|%d|%d|%u|%d|%d\n", chapterOf(pageStart[page]) + 1, chCount(), page + 1, (int)pageStart.size(), sizeIdx + 1, NSIZES, bookSrc, (unsigned)BOOK_LEN, batPct, batCharging ? 1 : 0);
  String s(h);
  layoutPage(pageStart[page], false, &s);
  server.send(200, "text/plain", s);
}
void handleCmd() {
  touch();
  String c = server.arg("c"); int p = server.arg("p").toInt();
  server.send(200, "text/plain", "OK");                      // reply first, then redraw the slow e-ink screen
  runCmd(c, p);
}
void handleUpload() {
  touch();
  HTTPUpload &u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    uploadOk = false;
    String n = u.filename; int sl = n.lastIndexOf('/'); if (sl >= 0) n = n.substring(sl + 1);
    if (isBookFile(n)) {
      upFile = LittleFS.open("/" + n, "w");
      uploadOk = (bool)upFile;
    }
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (upFile && upFile.write(u.buf, u.currentSize) != u.currentSize) uploadOk = false;
  } else if (u.status == UPLOAD_FILE_END) {
    if (upFile) upFile.close();
  }
}
void startServer() {
  server.on("/", HTTP_GET, []() { touch(); server.send_P(200, "text/html", PAGE); });
  server.on("/state", HTTP_GET, handleState);
  server.on("/cmd", HTTP_GET, handleCmd);
  server.on("/info", HTTP_GET, []() {   // free space counts the old book as free, because it is deleted before a new upload
    char j[180];
    snprintf(j, sizeof j, "{\"free\":%u,\"sw\":%d,\"sh\":%d,\"foot\":%d,\"margin\":%d,\"bat\":%d}",
             (unsigned)(LittleFS.totalBytes() - LittleFS.usedBytes() + bookBytes()),
             epaper.width(), epaper.height(), FOOTER_H + 6, MARGIN, batPct);
    server.send(200, "application/json", j);
  });
  server.on("/begin", HTTP_POST, []() {
    touch();   // step 1 of loading a book: delete the previous book's cache
    resetToDemoState();
    cleanStorage();                          // identical free space for every new book
    server.send(200, "text/plain", "OK");
  });
  server.on("/clear", HTTP_POST, []() {
    touch();   // "Clear books" button: remove the book, its pictures and all cache
    resetToDemoState();
    prefs.putUInt("off", 0);
    cleanStorage();
    server.send(200, "text/plain", "OK");
    redrawFlag = true;
  });
  server.on("/upload", HTTP_POST, []() {
    if (uploadOk) server.send(200, "text/plain", "OK");
    else server.send(507, "text/plain", "Not enough space on the reader");
  }, handleUpload);
  server.on("/done", HTTP_POST, []() {
    touch();    // last step: tell the reader to format + show the new book
    server.send(200, "text/plain", "OK");
    reloadFlag = true;
  });
  // Phones test for internet when joining. "No internet" makes Android send the browser through mobile data, so
  // proxlight.local / 192.168.4.1 stop working. Answering the checks like a real internet connection avoids that
  // (and avoids the captive-portal pop-up).
  server.on("/generate_204", HTTP_GET, []() { server.send(204, "text/plain", ""); });
  server.on("/gen_204", HTTP_GET, []() { server.send(204, "text/plain", ""); });
  server.on("/hotspot-detect.html", HTTP_GET, []() { server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"); });
  server.on("/library/test/success.html", HTTP_GET, []() { server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"); });
  server.on("/connecttest.txt", HTTP_GET, []() { server.send(200, "text/plain", "Microsoft Connect Test"); });
  server.on("/ncsi.txt", HTTP_GET, []() { server.send(200, "text/plain", "Microsoft NCSI"); });
  server.on("/success.txt", HTTP_GET, []() { server.send(200, "text/plain", "success\n"); });
  server.onNotFound([]() {            // any other address typed on the phone -> the reader page
    server.sendHeader("Location", String("http://") + ipStr + "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
}

// ---------- buttons ----------
void action(int id, bool isLong) {
  Serial.printf("button %d %s\n", id, isLong ? "long" : "short");
  if (id == 0) runCmd(isLong ? "prevch" : "prev", 0);
  else if (id == 2) runCmd(isLong ? "nextch" : "next", 0);
  else if (isLong) { if (infoShown) runCmd("redraw", 0); else showInfo(); }
  else if (infoShown) runCmd("redraw", 0);
  else setSize((sizeIdx + 1) % NSIZES);
}
struct Btn { uint8_t pin; uint32_t t0; bool down; bool longDone; };
Btn btns[3] = {{PIN_PREV, 0, false, false}, {PIN_MENU, 0, false, false}, {PIN_NEXT, 0, false, false}};

void setup() {
  setCpuFrequencyMhz(CPU_MHZ);                               // slower CPU = much less power
  Serial.begin(115200);
#if ENABLE_BUTTONS
  for (auto &b : btns) pinMode(b.pin, INPUT_PULLUP);
#endif
  if (!LittleFS.begin(true)) Serial.println("LittleFS failed (pick a Partition Scheme that says 'with spiffs')");
  epaper.begin();
  epaper.setRotation(SCREEN_ROTATION);
  Serial.printf("Screen: %d x %d\n", epaper.width(), epaper.height());
  prefs.begin("reader", false);
  sizeIdx = prefs.getUChar("size", 0); if (sizeIdx >= NSIZES) sizeIdx = 0;
  analogReadResolution(12);
  if (updateBattery()) goToSleepEmpty();                      // too empty to run: protect the LiPo
  startWifi();
  startServer();
  loadBook();
  paginate();
  goToOffset(prefs.getUInt("off", 0));
  drawPage();                                                // footer shows brand + Wi-Fi name
  touch();
}
void goToSleepEmpty() {            // battery critically low: tell the user, then deep sleep (~10 uA) until reset/charge
  showMessage("Battery empty", "Please charge the reader", "with a USB-C cable,", "then press the reset button.");
  WiFi.mode(WIFI_OFF);
  esp_deep_sleep_start();
}
uint32_t lastBatCheck = 0; int lowCount = 0;
void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
  if (redrawFlag) {                                          // books were cleared: show the demo book
    redrawFlag = false; infoShown = false;
    loadBook(); paginate(); goToOffset(0); drawPage();
  }
  if (millis() - lastBatCheck > 120000) {                    // silent check every 2 min (no screen refresh)
    lastBatCheck = millis();
    int mv = readBatteryMv();
    if (mv > 0) {
      applyReading(mv); pushTrend(mv);
      lowCount = (mv < BAT_CRITICAL_MV && !batCharging) ? lowCount + 1 : 0;
      if (lowCount >= 2) goToSleepEmpty();
    }
  }
  if (reloadFlag) {                                          // a new book was just uploaded
    reloadFlag = false; infoShown = false;
    prefs.putUInt("off", 0);
    showMessage("Preparing your book", "Formatting text and pictures...", "The first page appears in a moment.");
    loadBook(); paginate(); goToOffset(0);                   // everything is laid out BEFORE the first page is shown
    if (bookSrc == 2) showMessage("Could not open the book", bookErr.c_str(), "Please send it again from the web page.");
    else drawPage();
  }
#if ENABLE_BUTTONS
  uint32_t now = millis();
  for (int i = 0; i < 3; i++) {
    Btn &b = btns[i];
    bool p = (digitalRead(b.pin) == BTN_PRESSED);
    if (p && !b.down) { b.down = true; b.t0 = now; b.longDone = false; }
    else if (p && b.down && !b.longDone && now - b.t0 >= LONG_PRESS_MS) { b.longDone = true; action(i, true); }
    else if (!p && b.down) { b.down = false; if (!b.longDone && now - b.t0 >= 30) action(i, false); }
  }
#endif
  delay(10);
}
