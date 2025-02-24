#include <Wire.h>
#include <Adafruit_ADS1X15.h>

float factorEscala = 0.1875F;

Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;

void setup() {
    Serial.begin(115200);

    ads1.begin(0x48);
    ads2.begin(0x49);
}

void loop() {

    float CO2_1_VOLT = readVoltage(ads1, 0);
    float CO2_2_VOLT = readVoltage(ads1, 1);
    float CO2_3_VOLT = readVoltage(ads1, 2);
    
    float CH4_1_adc = ads1.readADC_SingleEnded(3);
    float CH4_2_adc = ads2.readADC_SingleEnded(0);
    float CH4_3_adc = ads2.readADC_SingleEnded(1);

    float co2_1_calb = (3601.9 * CO2_1_VOLT * CO2_1_VOLT) - (1190.6 * CO2_1_VOLT) + 461.63;  //y = 7151,7x2 - 5227,7x + 1559,6
    float co2_2_calb = (3601.9 * CO2_2_VOLT * CO2_2_VOLT) - (1190.6 * CO2_2_VOLT) + 461.63;
    float co2_3_calb = (3601.9 * CO2_3_VOLT * CO2_3_VOLT) - (1190.6 * CO2_3_VOLT) + 461.63;
    
    float ch4_1_calb = (0.2995 * CH4_1_adc) - 56.349; //y = 0,2995x - 56,349 
    float ch4_2_calb = (1.0982 * CH4_2_adc) - 177.97;
    float ch4_3_calb = (0.4641 * CH4_3_adc) - 86.055;

    // Serial.print("CO2_1 :"); Serial.println(co2_1_calb);
    // Serial.print("CO2_2 :"); Serial.println(co2_2_calb);
    // Serial.print("CO2_3 :"); Serial.println(co2_3_calb);
    // Serial.println(" ");

    // Serial.print("CH4_1 volt :"); Serial.println(CH4_1_VOLT);
    // Serial.print("CH4_2 volt :"); Serial.println(CH4_2_VOLT);
    // Serial.print("CH4_3 volt :"); Serial.println(CH4_3_VOLT);
    Serial.println("---------------------------------");
    Serial.print("CH4_1 callib:"); Serial.println(ch4_1_calb);
    Serial.print("CH4_2 callib:"); Serial.println(ch4_2_calb);
    Serial.print("CH4_3 callib:"); Serial.println(ch4_3_calb);
    Serial.println("---------------------------------");
    
    delay(500);
}

float readVoltage(Adafruit_ADS1115 &ads, int channel) {
    float adc = ads.readADC_SingleEnded(channel);

    // Jika channel adalah 1, kalibrasi nilai ADC sebelum dikonversi ke tegangan
    if (&ads == &ads1 && channel == 0) {
        adc -= 470;  // Sesuaikan faktorKalibrasi dengan kebutuhan
        // Serial.print("adc co2 1 :"); Serial.println(adc);
    }

    // if (&ads == &ads1 && channel == 1) {
    //     // adc += 55;  // Sesuaikan faktorKalibrasi dengan kebutuhan
    //     Serial.print("adc co2 2 :"); Serial.println(adc);
    // }
    // if (&ads == &ads1 && channel == 2) {
    //     // adc += 55;  // Sesuaikan faktorKalibrasi dengan kebutuhan
    //     Serial.print("adc co2 2 :"); Serial.println(adc);
    // }

        if (&ads == &ads1 && channel == 3) {
        // adc -= 761;  // Sesuaikan faktorKalibrasi dengan kebutuhan
        Serial.print("adc CH4 1 :"); Serial.println(adc);
    }

        if (&ads == &ads2 && channel == 0) {
        // adc += 55;  // Sesuaikan faktorKalibrasi dengan kebutuhan
        Serial.print("adc CH4 2 :"); Serial.println(adc);
    }

        if (&ads == &ads2 && channel == 1) {
        // adc -= 162;  // Sesuaikan faktorKalibrasi dengan kebutuhan
        Serial.print("adc CH4 3 :"); Serial.println(adc);
    }

    float voltage = ((adc * factorEscala) / 1000.0);
    return voltage;
}

// -------------------------------------------------------------------------------------------------------------------------------------BATAS KODE--------------------------------------

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <MQUnifiedsensor.h>

#define PLACA "ESP32"
#define Voltage_Resolution 3.3
#define ADC_Bit_Resolution 16
#define TYPE_MQ135 "MQ-135"
#define TYPE_MQ4 "MQ-4"
#define RATIO_MQ135_CLEAN_AIR 3.6
#define RATIO_MQ4_CLEAN_AIR 4.4 // Sesuaikan nilai jika diperlukan


float factorEscala = 0.1875F;

Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;

MQUnifiedsensor MQ135_1(PLACA, TYPE_MQ135);
MQUnifiedsensor MQ135_2(PLACA, TYPE_MQ135);
MQUnifiedsensor MQ135_3(PLACA, TYPE_MQ135);
MQUnifiedsensor MQ4_1(PLACA, TYPE_MQ4);
MQUnifiedsensor MQ4_2(PLACA, TYPE_MQ4);
MQUnifiedsensor MQ4_3(PLACA, TYPE_MQ4);

void setup() {
    Serial.begin(115200);
    delay(200);

    MQ135_1.setRegressionMethod(0);
    MQ135_2.setRegressionMethod(0);
    MQ135_3.setRegressionMethod(0);
    MQ4_1.setRegressionMethod(1);
    MQ4_2.setRegressionMethod(1);
    MQ4_3.setRegressionMethod(1);

    MQ135_1.init();
    MQ135_2.init();
    MQ135_3.init();
    MQ4_1.init();
    MQ4_2.init();
    MQ4_3.init();

    ads1.begin(0x48);
    ads2.begin(0x49);

    calibrateSensor(MQ135_1, ads1, 0, RATIO_MQ135_CLEAN_AIR, TYPE_MQ135);
    calibrateSensor(MQ135_2, ads1, 1, RATIO_MQ135_CLEAN_AIR, TYPE_MQ135);
    calibrateSensor(MQ135_3, ads1, 2, RATIO_MQ135_CLEAN_AIR, TYPE_MQ135);
    calibrateSensor(MQ4_1, ads1, 3, RATIO_MQ4_CLEAN_AIR, TYPE_MQ4);
    calibrateSensor(MQ4_2, ads2, 1, RATIO_MQ4_CLEAN_AIR, TYPE_MQ4);
    calibrateSensor(MQ4_3, ads2, 2, RATIO_MQ4_CLEAN_AIR, TYPE_MQ4);
}

void loop() {
    float CO2_1 = readSensor(MQ135_1, ads1, 0, 6555.5, 1.9015) ;
    // float CO2_1_CALB = (-0.0167 * CO2_1 * CO2_1) + (36.477 * CO2_1) - 11836; //  y = -0,0167CO2_12 + 36,477CO2_1 - 11836
    float CO2_2 = readSensor(MQ135_2, ads1, 1, 3772.2, 2.0361) ;
    // float CO2_2_CALB = (3E-06 * CO2_2 * CO2_2) + (0.0136 * CO2_2) + 519.39; // y = 3E-06CO2_12 + 0,0136CO2_1 + 519,39
    float CO2_3 = readSensor(MQ135_3, ads1, 2, 3772.2, 2.0361) ;
    // float CO2_3_CALB = (-0.0129 * CO2_3 * CO2_3) + (30.711 * CO2_3) - 10197; // y = -0,0129x2 + 30,711x - 10197

    float CH4_1 = readSensor(MQ4_1, ads1, 3, 381.92, -3.113);
    // float CH4_1_CALB = (11.482 * CO2_1) - 3416.2 //y = 11,482x - 3416,2
    float CH4_2 = readSensor(MQ4_2, ads2, 1, 381.92, -3.113);
    // float CH4_2_CALB = (11.482 * CO2_1) - 3416.2 //y = 11,482x - 3416,2
    float CH4_3 = readSensor(MQ4_3, ads2, 2, 381.92, -3.113);
    // float CO4_3_CALB = (11.482 * CO2_1) - 3416.2 //y = 11,482x - 3416,2

    float CO2_1_VOLT = readVoltage(ads1, 0);
    float CO2_2_VOLT = readVoltage(ads1, 1); //+ 0.35;
    float CO2_3_VOLT = readVoltage(ads1, 2); // + 0.19;

    float co2_1_calb = (3601.9 * CO2_1_VOLT * CO2_1_VOLT) - (1190.6 * CO2_1_VOLT) + 461.63;  //y = 7151,7x2 - 5227,7x + 1559,6
    float co2_2_calb = (3601.9 * CO2_2_VOLT * CO2_2_VOLT) - (1190.6 * CO2_2_VOLT) + 461.63;
    float co2_3_calb = (3601.9 * CO2_3_VOLT * CO2_3_VOLT) - (1190.6 * CO2_3_VOLT) + 461.63;
    
    float CO2_1_VOLT = readVoltage(ads1, 0);
    float CO2_2_VOLT = readVoltage(ads1, 1); //+ 0.35;
    float CO2_3_VOLT = readVoltage(ads1, 2); // + 0.19;

    float co2_1_calb = (3601.9 * CO2_1_VOLT * CO2_1_VOLT) - (1190.6 * CO2_1_VOLT) + 461.63;  //y = 7151,7x2 - 5227,7x + 1559,6
    float co2_2_calb = (3601.9 * CO2_2_VOLT * CO2_2_VOLT) - (1190.6 * CO2_2_VOLT) + 461.63;
    float co2_3_calb = (3601.9 * CO2_3_VOLT * CO2_3_VOLT) - (1190.6 * CO2_3_VOLT) + 461.63;

    // Serial.print("CO2_1 :"); Serial.println(co2_1_calb);
    // Serial.print("CO2_2 :"); Serial.println(co2_2_calb);
    // Serial.print("CO2_3 :"); Serial.println(co2_3_calb);
    // Serial.println(" ");
    // Serial.print("CO2_1 :"); Serial.println(CO2_1_VOLT);
    // Serial.print("CO2_2 :"); Serial.println(CO2_2_VOLT);
    // Serial.print("CO2_3 :"); Serial.println(CO2_3_VOLT);
    // Serial.println(" ");

    // Serial.print("CO2_1: "); Serial.print(CO2_1); Serial.print(" ppm | ");
    // Serial.print("CO2_2: "); Serial.print(CO2_2); Serial.print(" ppm | ");
    // Serial.print("CO2_3: "); Serial.print(CO2_3); Serial.print(" ppm | ");
    Serial.print("CH4_1: "); Serial.print(CH4_1); Serial.print(" ppm | ");
    Serial.print("CH4_2: "); Serial.print(CH4_2); Serial.print(" ppm | ");
    Serial.print("CH4_3: "); Serial.print(CH4_3); Serial.println(" ppm");
    
    delay(500);
}

void calibrateSensor(MQUnifiedsensor &sensor, Adafruit_ADS1115 &ads, int channel, float ratio, const char* sensorType) {
    Serial.print("Calibrating "); Serial.print(sensorType); Serial.println("...");
    float calcR0 = 0;
    sensor.setRL(22);
    for (int i = 0; i < 10; i++) {
        float voltage = readVoltage(ads, channel);
        sensor.externalADCUpdate(voltage);
        calcR0 += sensor.calibrate(ratio);
        Serial.print(".");
    }
    sensor.setR0(calcR0 / 10);
    Serial.println(" done!");
}

float readSensor(MQUnifiedsensor &sensor, Adafruit_ADS1115 &ads, int channel, float a, float b) {
    float voltage = readVoltage(ads, channel);
    sensor.externalADCUpdate(voltage);
    sensor.setRL(22);
    sensor.setA(a);
    sensor.setB(b);
    return sensor.readSensor();
}

float readVoltage(Adafruit_ADS1115 &ads, int channel) {
    float adc = ads.readADC_SingleEnded(channel);

    // Jika channel adalah 1, kalibrasi nilai ADC sebelum dikonversi ke tegangan
    // if (&ads == &ads1 && channel == 0) {
    //     adc -= 618;  // Sesuaikan faktorKalibrasi dengan kebutuhan
    //     // Serial.print("adc co2 1 :"); Serial.println(adc);
    // }

    // if (&ads == &ads1 && channel == 1) {
    //     // adc += 55;  // Sesuaikan faktorKalibrasi dengan kebutuhan
    //     Serial.print("adc co2 2 :"); Serial.println(adc);
    // }
    // if (&ads == &ads1 && channel == 2) {
    //     // adc += 55;  // Sesuaikan faktorKalibrasi dengan kebutuhan
    //     Serial.print("adc co2 2 :"); Serial.println(adc);
    // }

        // if (&ads == &ads1 && channel == 2) {
    //     // adc += 55;  // Sesuaikan faktorKalibrasi dengan kebutuhan
    //     Serial.print("adc co2 2 :"); Serial.println(adc);
    // }

        // if (&ads == &ads1 && channel == 2) {
    //     // adc += 55;  // Sesuaikan faktorKalibrasi dengan kebutuhan
    //     Serial.print("adc co2 2 :"); Serial.println(adc);
    // }

        // if (&ads == &ads1 && channel == 2) {
    //     // adc += 55;  // Sesuaikan faktorKalibrasi dengan kebutuhan
    //     Serial.print("adc co2 2 :"); Serial.println(adc);
    // }

    float voltage = ((adc * factorEscala) / 1000.0);
    return voltage;
}

