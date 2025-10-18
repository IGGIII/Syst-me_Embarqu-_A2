// importer les biblothèques pour le capteur wpse335
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

#define LEDPIN 7
#define PUSHPIN A1   // ton bouton double est branché sur une entrée analogique

int randNumber = 0;
volatile int flag = 0;
volatile unsigned long start = 0;
volatile unsigned long temp = 0;
const int etat[4] = {1, 2, 3, 4};
volatile int currentIndex = 0; // commence à etat[0] = 1
int LOG_INTERVAL = 3000;


// Fonction de lecture analogique stabilisée
int readAnalogStable(int pin) {
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(pin);
    delay(5);
  }
  return total / 10;
}

// Seuil de détection pour le bouton fonctionnel (actif bas)
const int BUTTON_THRESHOLD = 50;  // en dessous de ~50 = considéré comme appuyé à 0 V

void mode_standard() { Serial.println("Vous etes passe en mode standard"); }
void mode_configuration() { Serial.println("Vous etes passe en mode configuration"); }
void mode_maintenance() { Serial.println("Vous etes passe en mode mode_maintenance"); }
void mode_economie() { Serial.println("Vous etes passe en mode economie"); }

void change() {
  unsigned long now = millis();
  int reading = readAnalogStable(PUSHPIN);

  bool pressed = (reading <= BUTTON_THRESHOLD); // actif bas = proche de 0 V

  if (pressed && flag == 0) {
    start = now;
    flag = 1;
    Serial.println("Bouton presse (tension basse detectee)");
  } else if (!pressed && flag == 1) {
    flag = 0;
    unsigned long duration = now - start;

    if (duration >= 4000UL) {
      mode_economie();
      currentIndex = 3;
    } else if (duration >= 3000UL) {
      mode_maintenance();
      currentIndex = 2;
    } else if (duration >= 2000UL) {
      mode_configuration();
      currentIndex = 1;
    } else {
      mode_standard();
      currentIndex = 0;
    }

    Serial.print("Duree appui : ");
    Serial.print(duration);
    Serial.println(" ms");
    Serial.print("Etat courant : ");
    Serial.println(etat[currentIndex]);
  }
}

void capteur(){
  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0F;  // in hPa
  float humidity = bme.readHumidity();
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" Â°C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Pressure: ");
  Serial.print(pressure);
  Serial.print(" hPa, Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");

  delay(LOG_INTERVAL);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);  // wait for serial

  if (!bme.begin(0x76)) {  // common address is 0x76 or 0x77
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1) delay(10);
  }
  //pinMode(LEDPIN, OUTPUT);
  Serial.print("Etat initial : ");
  Serial.println(etat[currentIndex]);
}

void loop() {
  change();
  delay(10);
  if (currentIndex == 0) {
    capteur();
  }
  if (currentIndex == 3) {
    // désactiver GPS
    LOG_INTERVAL = LOG_INTERVAL*2;
    capteur();
  }
}