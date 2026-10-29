#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define PIN_R 4        // lampu kamar
#define PIN_G 5        // lampu ruang tamu
#define PIN_B 6        // indikator bunyi (cari agent)
#define BUZZER_PIN 7   // opsional, buzzer di Wokwi

Adafruit_SSD1306 display(128, 64, &Wire, -1);

enum Mood { NORMAL, HAPPY, SAD, SLEEP, SURPRISE, SEARCH, LOVE };
Mood mood = NORMAL;
unsigned long moodUntil = 0;   // 0 = permanen sampai diganti

bool lampuKamar = false;
bool lampuTamu  = false;

// animasi mata
int curDx = 0, curDy = 0, tgtDx = 0, tgtDy = 0;
unsigned long nextLook = 0, nextBlink = 2500, blinkEnd = 0;
unsigned long lastFrame = 0, lastBeep = 0;
bool beepState = false;

void setMood(Mood m, unsigned long durasiMs = 0) {
  mood = m;
  moodUntil = durasiMs ? millis() + durasiMs : 0;
}

// ---------- GAMBAR WAJAH ----------
void mataNormal(int dx, int dy, int h) {
  int y = 12 + dy + (36 - h) / 2;
  display.fillRoundRect(20 + dx, y, 30, h, 9, SSD1306_WHITE);
  display.fillRoundRect(78 + dx, y, 30, h, 9, SSD1306_WHITE);
}

void mataBahagia() {            // ^ ^
  for (int t = 0; t < 4; t++) {
    display.drawLine(22, 38 + t, 35, 24 + t, SSD1306_WHITE);
    display.drawLine(35, 24 + t, 48, 38 + t, SSD1306_WHITE);
    display.drawLine(80, 38 + t, 93, 24 + t, SSD1306_WHITE);
    display.drawLine(93, 24 + t, 106, 38 + t, SSD1306_WHITE);
  }
}

void mataSedih() {
  mataNormal(0, 4, 30);
  // potong sudut luar atas supaya terlihat sedih
  display.fillTriangle(14, 10, 52, 10, 14, 30, SSD1306_BLACK);
  display.fillTriangle(114, 10, 76, 10, 114, 30, SSD1306_BLACK);
}

void mataTidur() {
  display.fillRoundRect(20, 34, 30, 5, 2, SSD1306_WHITE);
  display.fillRoundRect(78, 34, 30, 5, 2, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(104, 8);  display.print("z");
  display.setCursor(112, 2);  display.print("Z");
}

void mataKaget() {
  display.fillRoundRect(18, 8, 34, 42, 12, SSD1306_WHITE);
  display.fillRoundRect(76, 8, 34, 42, 12, SSD1306_WHITE);
  display.fillCircle(35, 29, 5, SSD1306_BLACK);
  display.fillCircle(93, 29, 5, SSD1306_BLACK);
}

void hati(int cx, int cy) {
  display.fillCircle(cx - 6, cy, 7, SSD1306_WHITE);
  display.fillCircle(cx + 6, cy, 7, SSD1306_WHITE);
  display.fillTriangle(cx - 13, cy + 3, cx + 13, cy + 3, cx, cy + 18, SSD1306_WHITE);
}

void mulut(Mood m) {
  int cx = 64, cy = 54;
  if (m == SLEEP) return;
  if (m == SURPRISE) { display.drawCircle(cx, cy + 2, 3, SSD1306_WHITE); return; }
  for (int x = -7; x <= 7; x++) {
    int y = (m == SAD) ? cy + 4 - (x * x) / 12 : cy + (x * x) / 12;
    display.drawPixel(cx + x, y, SSD1306_WHITE);
    display.drawPixel(cx + x, y + 1, SSD1306_WHITE);
  }
}

void gambarWajah() {
  unsigned long now = millis();

  // lirik acak
  if (now > nextLook) {
    tgtDx = random(-8, 9);
    tgtDy = random(-4, 5);
    nextLook = now + random(1200, 3000);
  }
  curDx += (tgtDx - curDx) / 3;
  curDy += (tgtDy - curDy) / 3;

  // kedip
  int h = 36;
  if (now > nextBlink) { blinkEnd = now + 140; nextBlink = now + random(2000, 5000); }
  if (now < blinkEnd) h = 5;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  switch (mood) {
    case NORMAL:   mataNormal(curDx, curDy, h); mulut(NORMAL); break;
    case HAPPY:    mataBahagia(); mulut(HAPPY); break;
    case SAD:      mataSedih(); mulut(SAD); break;
    case SLEEP:    mataTidur(); break;
    case SURPRISE: mataKaget(); mulut(SURPRISE); break;
    case LOVE:     hati(35, 24); hati(93, 24); mulut(HAPPY); break;
    case SEARCH: {
      int dx = (int)(10 * sin(now / 180.0));   // mata geleng kiri-kanan
      mataNormal(dx, -2, h);
      display.setTextSize(1);
      display.setCursor(58, 56);
      display.print("?");
      break;
    }
  }
  display.display();
}

// ---------- OUTPUT ----------
void updateLampu() {
  digitalWrite(PIN_R, lampuKamar ? HIGH : LOW);
  digitalWrite(PIN_G, lampuTamu ? HIGH : LOW);
}

void updateCari() {
  if (mood != SEARCH) {
    digitalWrite(PIN_B, LOW);
    noTone(BUZZER_PIN);
    return;
  }
  if (millis() - lastBeep > 400) {
    lastBeep = millis();
    beepState = !beepState;
    digitalWrite(PIN_B, beepState);
    if (beepState) tone(BUZZER_PIN, 2000, 200);
  }
}

// ---------- PERINTAH ----------
void proses(String cmd) {
  cmd.toLowerCase();
  cmd.trim();
  if (cmd.length() == 0) return;

  bool nyala = cmd.indexOf("nyala") >= 0 || cmd.indexOf("hidup") >= 0 || cmd.indexOf("on") >= 0;
  bool mati  = cmd.indexOf("mati") >= 0 || cmd.indexOf("off") >= 0;

  if (cmd.indexOf("lampu kamar") >= 0) {
    if (nyala && !mati) { lampuKamar = true;  Serial.println("Robot: Lampu kamar nyala~"); setMood(HAPPY, 3000); }
    else if (mati)      { lampuKamar = false; Serial.println("Robot: Lampu kamar dimatikan"); setMood(NORMAL); }
  }
  else if (cmd.indexOf("lampu ruang tamu") >= 0 || cmd.indexOf("lampu tamu") >= 0) {
    if (nyala && !mati) { lampuTamu = true;  Serial.println("Robot: Lampu ruang tamu nyala~"); setMood(HAPPY, 3000); }
    else if (mati)      { lampuTamu = false; Serial.println("Robot: Lampu ruang tamu dimatikan"); setMood(NORMAL); }
  }
  else if (cmd.indexOf("semua") >= 0) {
    lampuKamar = lampuTamu = (nyala && !mati);
    Serial.println(lampuKamar ? "Robot: Semua lampu nyala" : "Robot: Semua lampu mati");
    setMood(lampuKamar ? HAPPY : NORMAL, 3000);
  }
  else if (cmd.indexOf("cari") >= 0 || cmd.indexOf("dimana") >= 0 || cmd.indexOf("hilang") >= 0) {
    Serial.println("Robot: Oke, aku cari agent kecil! Bip bip...");
    setMood(SEARCH);
  }
  else if (cmd.indexOf("ketemu") >= 0 || cmd.indexOf("stop") >= 0 || cmd.indexOf("berhenti") >= 0) {
    Serial.println("Robot: Yeay ketemu!");
    setMood(HAPPY, 3000);
  }
  else if (cmd.indexOf("tidur") >= 0) {
    Serial.println("Robot: Zzz...");
    setMood(SLEEP);
  }
  else if (cmd.indexOf("bangun") >= 0) {
    Serial.println("Robot: Aku bangun!");
    setMood(SURPRISE, 1500);
  }
  else if (cmd.indexOf("halo") >= 0 || cmd.indexOf("hai") >= 0) {
    Serial.println("Robot: Halo! Ada yang bisa kubantu?");
    setMood(HAPPY, 3000);
  }
  else if (cmd.indexOf("sedih") >= 0) {
    Serial.println("Robot: Hiks...");
    setMood(SAD, 3000);
  }
  else {
    Serial.println("Robot: Hah? Aku nggak ngerti.");
    setMood(SURPRISE, 1500);
  }
  updateLampu();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED tidak terdeteksi");
    while (true);
  }
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  randomSeed(analogRead(0));

  Serial.println("=== Robot Mochi siap ===");
  Serial.println("Contoh: nyalakan lampu kamar | matikan lampu ruang tamu");
  Serial.println("        cari agent | ketemu | tidur | bangun | halo ");
}

void loop() {
  if (Serial.available()) {
    proses(Serial.readStringUntil('\n'));
  }

  if (moodUntil && millis() > moodUntil) setMood(NORMAL);

  updateCari();

  if (millis() - lastFrame > 50) {
    lastFrame = millis();
    gambarWajah();
  }
}