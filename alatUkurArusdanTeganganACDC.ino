#include <EmonLib.h>
#include <QuickStats.h>
#include <arduinoFFT.h>
#include <Ewma.h>
#include <EwmaT.h>
#include <Adafruit_ADS1X15.h>
#include <TFT_22_ILI9225.h>
#include <SPI.h>
#include <Wire.h>
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

QuickStats statsAc; //initialize an instance of this class
QuickStats statsDc; //initialize an instance of this class
QuickStats statsADc; //initialize an instance of this class
#define NUM_READINGSAc 5  // Jumlah sampel untuk statistik
#define NUM_READINGSDc 10  // Jumlah sampel untuk statistik
#define NUM_READINGSADc 10  // Jumlah sampel untuk statistik
float adcReadingsAc[NUM_READINGSAc];  // Array untuk menyimpan hasil ADC
float adcReadingsDc[NUM_READINGSDc];  // Array untuk menyimpan hasil ADC
float adcReadingsADc[NUM_READINGSADc];  // Array untuk menyimpan hasil ADC


void setup() {
  Serial.begin(115200);
  analogReference(DEFAULT);

  ads.begin();
  ads.setGain(GAIN_SIXTEEN);
  // ads.setDataRate(RATE_ADS1115_860SPS);
  emon1.current(SCT_PIN, 108.225);

  tft.begin();
  tft.clear();
  tft.setOrientation(1);
  tft.setBacklight(true);
  tft.setFont(Terminal6x8);
  tft.setBackgroundColor(COLOR_BLACK);

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

    // Simpan data ke SD card jika tombol ditekan
    if (sdReady && digitalRead(RECORD_PIN) == LOW) {
      if (!headerWritten) {
        writeHeader();
        headerWritten = true;
        tft.fillRectangle(0, 120, tft.maxX(), 130, COLOR_YELLOW);
      }
      saveDataToSD();
        tft.fillRectangle(0, 120, tft.maxX(), 130, COLOR_GREEN);
    } else {
      sdReady = checkSdCard();
      headerWritten = false;
        tft.fillRectangle(0, 120, tft.maxX(), 130, COLOR_RED);
    }
  }
}

void saveDataToSD() {
  dataFile = SD.open("data.CSV", FILE_WRITE);
  if (dataFile) {
    dataFile.print(acVoltage, 2); dataFile.print(", ");
    dataFile.print(dcVoltage, 2); dataFile.print(", ");
    dataFile.print(acCurrent, 2); dataFile.print(", ");
    dataFile.println(dcCurrent, 2);
    dataFile.flush();
    dataFile.close();
  } else {
    Serial.print("gagal menulis data");
  }
}

void writeHeader() {
  dataFile = SD.open("data.CSV", FILE_WRITE);
  if (dataFile) {
    dataFile.println("AC Voltage (V), DC Voltage (V), AC Current (A), DC Current (A)");
    
    dataFile.flush();
    dataFile.close();
  } else {
    Serial.print("gagal menulis header");
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
  tft.drawText(45, 110, "STATUS REKAM DATA", COLOR_WHITE);

  tft.setFont(Trebuchet_MS16x21);

  // Perbarui hanya jika nilai berubah
  if (acVoltage != prevAcVoltage) {
    tft.drawText(20, 20, "        ", COLOR_BLACK);  // Hapus angka lama
    tft.drawText(20, 20, String(acVoltage, 2).c_str(), COLOR_WHITE);  // Tampilkan angka baru
    prevAcVoltage = acVoltage;  // Simpan nilai terbaru
  }

  if (dcVoltage != prevDcVoltage) {
    tft.drawText(120, 20, "        ", COLOR_BLACK);
    tft.drawText(120, 20, String(dcVoltage, 2).c_str(), COLOR_WHITE);
    prevDcVoltage = dcVoltage;
  }

  if (acCurrent != prevAcCurrent) {
    tft.drawText(20, 60, "        ", COLOR_BLACK);
    tft.drawText(20, 60, String(acCurrent, 2).c_str(), COLOR_WHITE);
    prevAcCurrent = acCurrent;
  }

  if (dcCurrent != prevDcCurrent) {
    tft.drawText(120, 60, "        ", COLOR_BLACK);
    tft.drawText(120, 60, String(dcCurrent, 2).c_str(), COLOR_WHITE);
    prevDcCurrent = dcCurrent;
  }

  // Garis bawah & nama hanya perlu digambar sekali
  tft.drawLine(0, 140, 218, 140, COLOR_WHITE);
  tft.setFont(Terminal11x16);
  tft.drawText(40, tft.maxY() - 25, "WAHYU RAMDANI");
}

float teganganAc() {
  digitalWrite(GND3, LOW);
  digitalWrite(GND4, LOW);

  // Mengisi array dengan hasil pembacaan ADC
  for (int i = 0; i < NUM_READINGSAc; i++) {
    adcReadingsAc[i] = ads.readADC_Differential_2_3();
  }

  // Mencari nilai maksimum dari pembacaan
  float adcMax = statsAc.maximum(adcReadingsAc, NUM_READINGSAc);

  // Konversi ke tegangan RMS
  float voltage = (adcMax * multiplier) / sqrt(2);
  float voltageCalb = ((0.0374 *voltage * voltage ) - (1.1304 * voltage) - 0.0294);

  if( voltageCalb < 0.00) {
      voltageCalb = 0.00;
  }

  return voltageCalb;
}


float teganganDc() {

  // Mengisi array dengan hasil pembacaan ADC
  for (int i = 0; i < NUM_READINGSDc; i++) {
    adcReadingsDc[i] = analogRead(A0) ;
  }

  // Mencari nilai maksimum dari pembDcaan
  float voltageCalb = statsDc.maximum(adcReadingsDc, NUM_READINGSDc);
  voltageCalb = voltageCalb * (5.0 / 1023.0) * voltageMultiplier + 3.55;

  if(voltageCalb > 80.00) {
    voltageCalb = 80.00;
  }
  
  return voltageCalb;
}

double arusAc() {
  float arusAc = emon1.calcIrms(750);
  float regresi = ((-0.0004 *arusAc * arusAc ) + (0.9066 * arusAc) - 0.299) - 1.5;  //y = 1,4903x - 36,511

  if(regresi < 0.00) {
    regresi = 0.00;
  }

  return regresi;
}

float arusDc() {
  

    // Mengisi array dengan hasil pembacaan ADC
  for (int i = 0; i < NUM_READINGSADc; i++) {
    adcReadingsADc[i] = ads.readADC_Differential_0_1();
  }

  float voltageShunt = statsADc.maximum(adcReadingsADc, NUM_READINGSADc);
  float vShunt = voltageShunt * multiplier;
  vShunt = vShunt / rShunt - 1.2 ;

  if(vShunt < 0.00) {
    vShunt = 0.00;
  }

  return vShunt;
}
