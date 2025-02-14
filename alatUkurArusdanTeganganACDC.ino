#include <Ewma.h>
#include <EwmaT.h>
#include <Adafruit_ADS1X15.h>
#include <TFT_22_ILI9225.h>
#include <SPI.h>
#include <Wire.h>
#include <EmonLib.h>
#include <SD.h>

#define TFT_RST 8
#define TFT_RS 7
#define TFT_CS 9
#define TFT_SDI 11
#define TFT_CLK 13
#define TFT_LED 0
#define TFT_BRIGHTNESS 200

TFT_22_ILI9225 tft(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);

#define SCT_PIN A1
#define RECORD_PIN 5
#define SD_CS 4
#define GND3 2
#define GND4 3

EnergyMonitor emon1;
Adafruit_ADS1115 ads;
Ewma emwaArusAc(0.7);
Ewma emwaVoltDc(0.5);
Ewma emwaVoltAc(0.2);

const float R1 = 220000.0, R2 = 10000.0;
const float voltageMultiplier = (R1 + R2) / R2;
const float rShunt = 0.25;
const float multiplier = 0.0078125;
const float R1_ac = 1000000.0, R2_ac = 1000.0;
const float voltageMultiplier_ac = (R1_ac + R2_ac) / R2_ac;

float acVoltage = 0, dcVoltage = 0;
double acCurrent = 0;
float dcCurrent = 0;

File dataFile;
bool headerWritten = false, sdReady = false;
unsigned long lastUpdateTime = 0;
unsigned long lastDataReadTime = 0;
const unsigned long dataReadInterval = 5; // Interval pengambilan data dalam ms
const unsigned long displayUpdateInterval = 100; // Interval pembaruan tampilan dalam ms

// Variabel untuk menyimpan nilai sebelumnya pda LCD tft 
float prevAcVoltage = -9999, prevDcVoltage = -9999;
float prevAcCurrent = -9999, prevDcCurrent = -9999;

void setup() {
  Serial.begin(115200);
  analogReference(DEFAULT);

  ads.begin();
  ads.setGain(GAIN_SIXTEEN);
  // ads.setDataRate(RATE_ADS1115_860SPS);
  emon1.current(SCT_PIN, 108.225);

  tft.begin();
  tft.setOrientation(1);
  tft.setBacklight(true);
  tft.setFont(Terminal6x8);
  tft.setBackgroundColor(COLOR_BLACK);
  tft.clear();

  sdReady = SD.begin(SD_CS);
  pinMode(GND3, OUTPUT);
  pinMode(GND4, OUTPUT);
  pinMode(RECORD_PIN, INPUT_PULLUP);
}

void loop() {
  unsigned long currentMillis = millis();

  // Ambil data setiap dataReadInterval ms
  if (currentMillis - lastDataReadTime >= dataReadInterval) {
    lastDataReadTime = currentMillis;
    acVoltage = teganganAc();
    dcVoltage = teganganDc();
    acCurrent = arusAc();
    dcCurrent = arusDc();
  }

  // Pembaruan tampilan setiap displayUpdateInterval ms
  if (currentMillis - lastUpdateTime >= displayUpdateInterval) {
    lastUpdateTime = currentMillis;
    displayData();
  }

  // Simpan data ke SD card jika tombol ditekan
  if (sdReady && digitalRead(RECORD_PIN) == LOW) {
    if (!headerWritten) {
      writeHeader();
      headerWritten = true;
    }
    saveDataToSD();
  } else {
    sdReady = checkSdCard();
    headerWritten = false;
  }
}

void saveDataToSD() {
  dataFile = SD.open("data.CSV", FILE_WRITE);
  if (dataFile) {
    dataFile.print(acVoltage, 2); dataFile.print(", ");
    dataFile.print(dcVoltage, 2); dataFile.print(", ");
    dataFile.print(acCurrent, 2); dataFile.print(", ");
    dataFile.println(dcCurrent, 2);
    dataFile.close();
  }
}

void writeHeader() {
  dataFile = SD.open("data.CSV", FILE_WRITE);
  if (dataFile) {
    dataFile.println("AC Voltage (V), DC Voltage (V), AC Current (A), DC Current (A)");
    dataFile.close();
  }
}

bool checkSdCard() {
  return SD.begin(SD_CS);
}

void displayData() {
  // Menampilkan label hanya sekali (tidak perlu diperbarui setiap waktu)
  tft.setFont(Terminal6x8);
  tft.drawText(20, 10, "V AC", COLOR_GRAY);
  tft.drawText(120, 10, "V DC", COLOR_GRAY);
  tft.drawText(20, 50, "A AC", COLOR_GRAY);
  tft.drawText(120, 50, "A DC", COLOR_GRAY);

  tft.setFont(Trebuchet_MS16x21);

  // Perbarui hanya jika nilai berubah
  if (acVoltage != prevAcVoltage) {
    tft.drawText(20, 20, "       ", COLOR_BLACK);  // Hapus angka lama
    tft.drawText(20, 20, String(acVoltage, 2).c_str(), COLOR_WHITE);  // Tampilkan angka baru
    prevAcVoltage = acVoltage;  // Simpan nilai terbaru
  }

  if (dcVoltage != prevDcVoltage) {
    tft.drawText(120, 20, "       ", COLOR_BLACK);
    tft.drawText(120, 20, String(dcVoltage, 2).c_str(), COLOR_WHITE);
    prevDcVoltage = dcVoltage;
  }

  if (acCurrent != prevAcCurrent) {
    tft.drawText(20, 60, "       ", COLOR_BLACK);
    tft.drawText(20, 60, String(acCurrent, 2).c_str(), COLOR_WHITE);
    prevAcCurrent = acCurrent;
  }

  if (dcCurrent != prevDcCurrent) {
    tft.drawText(120, 60, "       ", COLOR_BLACK);
    tft.drawText(120, 60, String(dcCurrent, 2).c_str(), COLOR_WHITE);
    prevDcCurrent = dcCurrent;
  }

  // Garis bawah & nama hanya perlu digambar sekali
  tft.drawLine(0, 140, 218, 140, COLOR_WHITE);
  tft.setFont(Terminal11x16);
  tft.drawText(40, tft.maxY() - 25, "WAHYU RAMDANI");
}

// float teganganAc() {
//   digitalWrite(GND3, LOW);
//   digitalWrite(GND4, LOW);

//   float maxVoltage = 0.0;
//     float adc = (ads.readADC_Differential_2_3() * multiplier) * 1001;
//     // maxVoltage = max(maxVoltage, adc);

//     // if (adc > maxVoltage) {
//     //         maxVoltage = adc;
//     //     }
//     // maxVoltage = emwaVoltAc.filter(maxVoltage);

//   // return max(0.0, (0.0061 * maxVoltage * maxVoltage) + (0.5942 * maxVoltage) - 9.5626);
//   return adc;
// }
float teganganAc() {
  digitalWrite(GND3, LOW);
  digitalWrite(GND4, LOW);

  float maxVoltage = 0.0;
  unsigned long startTime = micros();
  while (micros() - startTime < 100) { // 2ms sampling
    float adc = ads.readADC_Differential_2_3() * multiplier;
    // maxVoltage = max(maxVoltage, adc);

    if (adc > maxVoltage) {
            maxVoltage = adc;
        }
    maxVoltage = emwaVoltAc.filter(maxVoltage);

  }
  return max(0.0, (0.0138 * maxVoltage * maxVoltage) + (0.4179 * maxVoltage) - 10.3);
  // return maxVoltage;
}

float teganganDc() {
  float voltageDc = (analogRead(A0) * (5.0 / 1023.0) * voltageMultiplier) + 5.47;
  return emwaVoltDc.filter(max(0.0, voltageDc));
}

double arusAc() {
  float arusAc = max(emon1.calcIrms(750), 0.0);
  float calibArusAc = arusAc - 0.8;

  if(calibArusAc < 0.00) {
    calibArusAc = 0.00;
  }

  return emwaArusAc.filter(calibArusAc);
}

float arusDc() {
  float vShunt = ads.readADC_Differential_0_1() * multiplier;
  return max(0.0, (vShunt / rShunt) - 1.2);
}
