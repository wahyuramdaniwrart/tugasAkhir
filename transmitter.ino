#include <SPI.h>
#include <RH_NRF24.h>
#include <Adafruit_ADS1X15.h>

// Initialize ADS1115 (DC voltage and current readings)
Adafruit_ADS1115 ads;

RH_NRF24 nrf24(6, 7); // use this to be electrically compatible with Mirf

float voltage = 0;
float vbat = 0;
float R1 = 1109000.0, R2 = 9910.0;
float voltageMultiplier = (R1 + R2) / R2; // Voltage divider factor for DC voltage

void setup()
{
    // ads.begin(0x48); // Initialize ADS1115
    Serial.begin(9600);
    analogReference(INTERNAL);
    while (!Serial)
        ; // wait for serial port to connect. Needed for Leonardo only
    if (!nrf24.init())
        Serial.println("init failed");
    // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
    if (!nrf24.setChannel(1))
        Serial.println("setChannel failed");
    if (!nrf24.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm))
        Serial.println("setRF failed");
}

void loop()
{
    float dcVoltage = teganganDc();

    // Prepare data to send
    char data[20];
    dtostrf(dcVoltage, 6, 2, data); // Convert dcVoltage to string with 2 decimal precision

    Serial.print("Sending to nrf24_server: ");
    Serial.println(data);

    // Send DC voltage to nrf24_server
    nrf24.send((uint8_t *)data, strlen(data) + 1);
    nrf24.waitPacketSent();

    // Now wait for a reply
    uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (nrf24.waitAvailableTimeout(500))
    {
        // Should be a reply message for us now
        if (nrf24.recv(buf, &len))
        {
            Serial.print("Got reply: ");
            Serial.println((char *)buf);
        }
        else
        {
            Serial.println("Receive failed");
        }
    }
    else
    {
        Serial.println("No reply, is nrf24_server running?");
    }
    delay(400);
}

// float teganganDc() {
//   ads.setGain(GAIN_ONE);
//   // Read DC voltage using ADS1115
//   int16_t adc0 = ads.readADC_SingleEnded(1);
//   voltage = adc0 * 0.125 / 1000;  // Convert to volts
//   vbat = voltage * voltageMultiplier;
//   vbat = max(vbat, 0.0f);  // Ensure non-negative value
//   Serial.println(vbat);
//   return vbat;  // Return DC voltage
// }

float teganganDc()
{
    float value = analogRead(A1);
    voltage = (value * 1.1) / 1023 * ((R1 + R2) / R2);
    voltage = max(voltage, 0.0f); // Ensure non-negative value
    Serial.println(voltage);
    return voltage; // Return DC voltage
}
