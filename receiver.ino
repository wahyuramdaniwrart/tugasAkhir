#include <Adafruit_ADS1X15.h>
#include <FirebaseRealtime.h>
#include <EmonLib.h>
#include <SPI.h>
#include <RH_NRF24.h>

// =====================================================================

// firebase
#define FIREBASE_REALTIME_URL "https://tugas-akhir-4ef4d-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_REALTIME_SECRET "QSbHty5oP7xwMVPlhi1RZWfnYBDf7QJK0I5Jo9kB"

char ssid[] = "wrart";
char pass[] = "ramdaniwahyu";

FirebaseRealtime firebaseRealtime;

// inisialisasi radio
RH_NRF24 nrf24(10, D0);

// Initialize SCT-019 (AC current sensor)
int sct_pin = A0;  // Pin for AC current sensor
double Irms, prevIrms = -1;  // AC current
EnergyMonitor emon1;

// Initialize ADS1115 (DC voltage and current readings)
Adafruit_ADS1115 ads;

float voltage = 0;
float vbat = 0;
float R1 = 220000.0, R2 = 10000.0;
float voltageMultiplier = (R1 + R2) / R2;  // Voltage divider factor for DC voltage
int16_t results;
float rShunt = 0.25;  // For 300A 75mV shunt resistor
float arus = 0;
float vShunt = 0;
float multiplier = 0.0078125;

// 3 phase voltage

float VoltageReadOffset = 0.0;
float Voltage = 0.0;
float Volt_ref = 0;
float calibrasi;

int gnd3 = D3;
int gnd4 = D4;

// Timing variables
unsigned long currentMillis = millis();
static unsigned long lastUpdateTime = 0;
static unsigned long lastSaveTime = 0;

// ===========================================================================

void setup() {
  Serial.begin(9600);
  // radio
  if (!nrf24.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
    Serial.println("setRF failed");
  // firebase
  firebaseRealtime.begin(FIREBASE_REALTIME_URL, FIREBASE_REALTIME_SECRET, ssid, pass);
  // Initialize ADS1115
  ads.begin(0x48);            // Initialize ADS11115           // Initialize ADS11115
  ads.setGain(GAIN_SIXTEEN);

  pinMode(gnd3, INPUT);
  pinMode(gnd4, INPUT);

  // Initialize AC current sensor (SCT-019)
  emon1.current(sct_pin, 108.225);
}

void loop() {
  // if (currentMillis - lastUpdateTime >= 2000) {

    firebase();
    delay(1000);
  // lastUpdateTime = currentMillis;
  // }
}

float radio() {
  if (nrf24.available()) {
    // Should be a message for us now   
    uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (nrf24.recv(buf, &len)) {
      // Convert the received buffer to a String
      String receivedData = String((char*)buf);
      Serial.print("got request: ");
      Serial.println(receivedData);
      
      // Convert the received String data to a float
      float receivedValue = receivedData.toFloat();
      
      // Send a reply
      uint8_t data[] = "And hello back to you";
      nrf24.send(data, sizeof(data));
      nrf24.waitPacketSent();
      Serial.println("Sent a reply");
      
      // Return the received value as a float
      return receivedValue;
    } else {
      Serial.println("recv failed");
      return 0.0; // Return 0.0 if reception failed
    }
  }
  
  // Return 0.0 if no data is available
  return 0.0;
}



void firebase() {
  float receivedMessage = radio();

  // save
  // DynamicJsonDocument saveDoc(1024);
  // saveDoc["voltageAc"] = 0.00;
  // saveDoc["voltageDc"] = 0.00;
  // saveDoc["currentAc"] = 0.00;
  // saveDoc["currentDc"] = 0.00;
  // String saveJSONData;
  // serializeJson(saveDoc, saveJSONData);
  // int saveResponseCode = firebaseRealtime.save("nodeMcu", "1", saveJSONData);
  // Serial.println("\nSave - response code: " + String(saveResponseCode));
  // saveDoc.clear();

  // update
  DynamicJsonDocument updateDoc(1024);
  updateDoc["voltageAc"] = teganganAc();
  updateDoc["voltageDc"] = receivedMessage;
  updateDoc["currentAc"] = arusAc();
  updateDoc["currentDc"] = arusDc();
  String updateJSONData;
  serializeJson(updateDoc, updateJSONData);
  int updateResponseCode = firebaseRealtime.save("nodeMcu", "1", updateJSONData, true);
  Serial.println("\nUpdate - response code: " + String(updateResponseCode));
  // updateDoc.clear();

  // // fetch
  // DynamicJsonDocument fetchDoc(1024);
  // int fetchResponseCode = firebaseRealtime.fetch("nodeMcu", "1", fetchDoc);
  // float voltageAc = fetchDoc["voltageAc"];
  // float voltageDc = fetchDoc["voltageDc"];
  // float currentAc = fetchDoc["currentDc"];
  // float currentDc = fetchDoc["currentDc"];
  // Serial.println("\nFetch - response code: " + String(fetchResponseCode));
  // Serial.println("voltageAc: " + String(voltageAc) + ", voltageDc: " + String(voltageDc) + ", currentAc: " + String(currentAc) + ", currentDc: " + String(currentDc));
  // fetchDoc.clear();

  // delete
  // int deleteResponseCode = remove("nodeMcu", "1");
  // Serial.println("\nDelete response code: " + String(deleteResponseCode));
}

float teganganAc() {
  pinMode(gnd3, OUTPUT);
  pinMode(gnd4, OUTPUT);
  digitalWrite(gnd3, LOW);
  digitalWrite(gnd4, LOW);

  float maxCalibrated = 0.0;  // Variable to store the highest calibrated value

  for (int i = 0; i < 10; i++) {
    // Read ADC value and convert to voltage
    float adc = ads.readADC_Differential_0_1();
    float voltage = 1001 * (adc * 0.007812) / 1000;

    // Apply calibration polynomial
    float calibratedValue = (0.0275 * pow(voltage, 2)) - (0.3552 * voltage) - 26.434;

    // Ensure the calibrated value is non-negative
    calibratedValue = max(calibratedValue, 0.0f);

    // Update maxCalibrated if the current calibratedValue is higher
    if (calibratedValue > maxCalibrated) {
      maxCalibrated = calibratedValue;
    }

    delayMicroseconds(10);  // Delay for sampling
  }

  return maxCalibrated;  // Return the highest calibrated value
}

// float teganganDc() {
//   // Read DC voltage using ADS11115
//   int16_t adc0 = ads1.readADC_SingleEnded(2);
//   voltage = adc0 * 0.125 / 1000;  // Convert to volts
//   vbat = voltage * voltageMultiplier;
//   vbat = vbat + 2.93;
//   vbat = max(vbat, 0.0f);  // Ensure non-negative value
//   return vbat;  // Return DC voltage
// }

double arusAc() {
  double newIrms = emon1.calcIrms(1500);
  newIrms = max(newIrms, 0.0);  // Ensure non-negative values
  newIrms -= 4; 

  if (newIrms < 0.00){
    newIrms = 0.00;
  }
  return newIrms;
}

float arusDc() {

  results = ads.readADC_Differential_2_3();
  vShunt = results * multiplier;
  arus = vShunt / rShunt ;
  if ( arus < 0.00 ) {
    arus = 0.00;
  }
  return arus;
}
