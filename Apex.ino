#include <SoftwareSerial.h>
#include "DHT.h"

SoftwareSerial BTSerial(10, 11);

// Delay between sensor reads, in seconds
#define READ_DELAY 3

#define DHTPIN 2    // Digital pin connected to the DHT sensor

#define MQ_7_PIN A0
#define MQ_135_PIN A1

// MQ-7 Variables
#define MQ7_DEFAULTPPM 392 //default ppm of CO2 for calibration
#define MQ7_DEFAULTRO 41763 //default Ro for MQ135_DEFAULTPPM ppm of CO2
#define MQ7_SCALINGFACTOR 116.6020682 //CO2 gas value
#define MQ7_EXPONENT -2.769034857 //CO2 gas value
#define MQ7_MAXRSRO 2.428 //for CO2
#define MQ7_MINRSRO 0.358 //for CO2

// MQ-135 Variables
/// The load resistance on the board
#define RLOAD 10.0
/// Calibration resistance at atmospheric level
#define RZERO 76.63
/// Parameters for calculating ppm of air from sensor resistance
#define PARA 116.6020682
#define PARB 2.769034857

/// Parameters to model temperature and humidity dependence
#define CORA 0.00035
#define CORB 0.02718
#define CORC 1.39538
#define CORD 0.0018

/// Atmospheric air level for calibration purposes
#define ATMOCO2 397.13

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  Serial.println("Started...");

  BTSerial.begin(9600);
  BTSerial.println("BT Started...");

  dht.begin();
}

void loop() {

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  } else {
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C "));
    Serial.print(F("Heat index: "));
    Serial.print(hic);
    Serial.print(F("°C \n"));
  }

  float COLevel = 0;

  for (int i = 0; i <= 100; i++)
  {
    COLevel += analogRead(MQ_7_PIN);
    delay(1);
  }

  COLevel /= 100.0;

  long mq7_ro = mq7_get_Ro(COLevel, MQ7_DEFAULTPPM);

  long COLevel_ppm = mq7_getppm(COLevel, mq7_ro);

  Serial.print("Carbon Monoxide Level : ");
  Serial.print(COLevel_ppm);
  Serial.println(" PPM");

  float GasLevel_ppm = mq135_getPPM(t, h, MQ_135_PIN);

  Serial.print("Gas Level : ");
  Serial.print(GasLevel_ppm);
  Serial.println(" PPM");

  String str = "H:" + String(h,2) + ";T:" + String(t,2) + ";HI:" + String(hic, 2) + ";C:" + String(COLevel_ppm) + ";G:" + String(GasLevel_ppm, 2) + "\n";

  char charArray[50];

  str.toCharArray(charArray, 50);
  BTSerial.write(charArray);
  Serial.print(charArray);

  delay(READ_DELAY * 1000);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MQ-7 Functions

// Calculate sensor resistance for MQ-7
// Here Ro means sensor resistance at 100ppm CO in the clean air.
long mq7_get_Ro(long resvalue, double ppm) {
  return (long)(resvalue * exp( log(MQ7_SCALINGFACTOR / ppm) / MQ7_EXPONENT ));
}

// Get the ppm concentration
double mq7_getppm(long resvalue, long ro) {
  double ret = 0;
  double validinterval = 0;
  validinterval = resvalue / (double)ro;
  if (validinterval < MQ7_MAXRSRO && validinterval > MQ7_MINRSRO) {
    ret = (double)MQ7_SCALINGFACTOR * pow( ((double)resvalue / ro), MQ7_EXPONENT);
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////
// MQ-135 Functions

float getCorrectionFactor(float t, float h) {
  return CORA * t * t - CORB * t + CORC - (h - 33.) * CORD;
}

float getResistance(uint8_t pin) {
  int val = analogRead(pin);
  return ((1023. / (float)val) * 5. - 1.) * RLOAD;
}

float getCorrectedResistance(float t, float h, uint8_t pin) {
  return getResistance(pin) / getCorrectionFactor(t, h);
}

float mq135_getPPM(float t, float h, uint8_t pin) {
  return PARA * pow((getCorrectedResistance(t, h, pin) / RZERO), -PARB);
}
