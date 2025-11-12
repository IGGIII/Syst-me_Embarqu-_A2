// Gestion 2 boutons + modes (standard / configuration / maintenance / economique)
// Capteurs BME280 + capteur de lumière (exemple)

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

// LED RGB pins
const int redpin = 10;
const int greenpin = 11;
const int bluepin = 9;

// Boutons (interrupt capable pins on Arduino Uno/Nano)
const int RED_BTN_PIN = 2;   // bouton rouge (D2)
const int GREEN_BTN_PIN = 3; // bouton vert (D3)

// Capteur lumière
const int LIGHT_ANALOG_PIN = A0;
const int thresholdvalue = 10;
float Rsensor = 0.0;

// Logging interval
int LOG_INTERVAL = 3000; // ms

// Debounce & timing
const unsigned long DEBOUNCE_MS = 50;
const unsigned long CONFIG_TIMEOUT_MS = 30UL * 60UL * 1000UL; // 30 minutes
const unsigned long LONG_PRESS_MS = 5000UL; // 5 secondes
const unsigned long CONFIG_PRESS_MS = 10000UL; // 10s pour entrer en configuration





// États
enum Mode {
  MODE_STANDARD = 0,
  MODE_CONFIGURATION = 1,
  MODE_MAINTENANCE = 2,
  MODE_ECONOMIQUE = 3
};

volatile Mode currentMode = MODE_STANDARD;
volatile Mode previousMode = MODE_STANDARD; // pour revenir depuis maintenance

// Variables ISR (volatiles)
volatile unsigned long redPressStartMicros = 0;
volatile unsigned long redLastEventMicros = 0;
volatile bool redActionPending = false;
volatile unsigned long redActionDurationMs = 0;

volatile unsigned long greenPressStartMicros = 0;
volatile unsigned long greenLastEventMicros = 0;
volatile bool greenActionPending = false;
volatile unsigned long greenActionDurationMs = 0;

// Indique qu'un mode a été changé par le traitement dans loop()
volatile bool modeChangedFlag = false;

// Suivi activité (ms) pour timeout configuration
unsigned long lastActivityMillis = 0;

// Fonctions de mode (non-ISR)
void mode_standard()      { Serial.println(">> Mode: STANDARD"); }
void mode_configuration() { Serial.println(">> Mode: CONFIGURATION (acquisition desactivee)"); }
void mode_maintenance()   { Serial.println(">> Mode: MAINTENANCE (acces serie / SD safe)"); }
void mode_economique()    { Serial.println(">> Mode: ECONOMIQUE (capteurs partiellement desactives)"); }

void couleur_change(int r,int g,int b){
  analogWrite(redpin, r);
  analogWrite(greenpin, g);
  analogWrite(bluepin, b);
}

// ISR pour le bouton rouge (très courte)
void redISR() {
  unsigned long now = micros();

  if ((now - redLastEventMicros) < (DEBOUNCE_MS * 1000UL)) return;
  redLastEventMicros = now;

  int state = digitalRead(RED_BTN_PIN); // LOW = appuyé, HIGH = relâché
  if (state == LOW) {
    redPressStartMicros = now;
  } else {
    if (redPressStartMicros == 0) return;
    unsigned long durMs = (now - redPressStartMicros) / 1000UL;
    redPressStartMicros = 0;
    // stocker l'action pour traitement hors ISR
    redActionDurationMs = durMs;
    redActionPending = true;
    // marquer activité
    lastActivityMillis = millis();
  }
}

// ISR pour le bouton vert (très courte)
void greenISR() {
  unsigned long now = micros();

  if ((now - greenLastEventMicros) < (DEBOUNCE_MS * 1000UL)) return;
  greenLastEventMicros = now;

  int state = digitalRead(GREEN_BTN_PIN); // LOW = appuyé, HIGH = relâché
  if (state == LOW) {
    greenPressStartMicros = now;
  } else {
    if (greenPressStartMicros == 0) return;
    unsigned long durMs = (now - greenPressStartMicros) / 1000UL;
    greenPressStartMicros = 0;
    greenActionDurationMs = durMs;
    greenActionPending = true;
    // marquer activité
    lastActivityMillis = millis();
  }
}

// Capteurs / acquisition (exemples)
void capteur_WPSE(){
  // Si acquisition désactivée (ex: config mode), on n'appelle pas bme
  if (currentMode == MODE_CONFIGURATION) {
    // acquisition désactivée
    delay(100); // petit délai pour ne pas boucler trop vite
    return;
  }

  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0F;  // in hPa
  float humidity = bme.readHumidity();
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

  Serial.print("Temp: "); Serial.print(temperature); Serial.print(" °C, ");
  Serial.print("Hum: "); Serial.print(humidity); Serial.print(" %, ");
  Serial.print("Pres: "); Serial.print(pressure); Serial.print(" hPa, ");
  Serial.print("Alt: "); Serial.print(altitude); Serial.print(" m");
  delay(10);
}



void enterMode(Mode m) {
  if (m == MODE_MAINTENANCE) {
    previousMode = currentMode;
    currentMode = MODE_MAINTENANCE;
  } else {
    currentMode = m;
  }

  switch (currentMode) {
    case MODE_STANDARD:
      LOG_INTERVAL = 3000;
      couleur_change(0, 255, 0); //vert
      mode_standard();
      break;

    case MODE_CONFIGURATION:
      mode_configuration();
      couleur_change(255, 20, 200); //VIOLET
      lastActivityMillis = millis();
      break;

    case MODE_MAINTENANCE:
      mode_maintenance();
      couleur_change(230, 150, 0); //Orange
      break;

    case MODE_ECONOMIQUE:
      LOG_INTERVAL = max(1000, LOG_INTERVAL * 2);
      mode_economique();
      couleur_change(0, 0, 255); //bleu
      break;
  }

  modeChangedFlag = true;
}


void setup() {
  //Configuration des broches
  pinMode(RED_BTN_PIN, INPUT_PULLUP);
  pinMode(GREEN_BTN_PIN, INPUT_PULLUP);
  pinMode(redpin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  pinMode(bluepin, OUTPUT);

  // parametrage
  Serial.begin(9600);
  while (!Serial); // si carte supporte

  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1) delay(10);
  }


  // Aayan je pense que cette partie de code est inutile car le "if" selon ne peut pas foncionner car on ne peut pas allumer l arduino avec un boutton4, en revanche le else il faut qu y soit pour dire que lorsqu on l allume ca soit en mode STANDARD
  // Si on démarre avec bouton rouge enfoncé => MODE_CONFIGURATION
  if (digitalRead(RED_BTN_PIN) == LOW) {
    currentMode = MODE_CONFIGURATION;
    Serial.println("Startup: RED button held -> enter CONFIGURATION mode");
    lastActivityMillis = millis();
  } else {
    currentMode = MODE_STANDARD;
    enterMode(MODE_STANDARD);
    Serial.println("Systeme demarre en mode STANDARD");
  }



  // Attacher interruptions
  attachInterrupt(digitalPinToInterrupt(RED_BTN_PIN), redISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(GREEN_BTN_PIN), greenISR, CHANGE);

  Serial.print("Etat initial : ");
  switch (currentMode) {
    case MODE_STANDARD: Serial.println("STANDARD"); break;
    case MODE_CONFIGURATION: Serial.println("CONFIGURATION"); break;
    default: Serial.println("UNKNOWN"); break;
  }
}


float capteur_light_sensor() {
  int sensorValue = analogRead(LIGHT_ANALOG_PIN);
  if (sensorValue <= 0) sensorValue = 1; // protection
  float Rs = (float)(1023 - sensorValue) * 10.0 / sensorValue;
  Rsensor = Rs; // variable globale si tu l'utilises ailleurs

  // tu peux imprimer l'état mais NE CHANGE PAS la RGB ici
  
  Serial.print("Light analog: "); // Unité de la luminosité est la candela. 
  Serial.print(sensorValue);
  Serial.print("  Rsensor: ");
  Serial.println(Rsensor);

  // retourne la valeur pour décision hors fonction
  return Rsensor;
}


//normalement les changements de mods c est bon 
void change_mod() {
  // Récupérer atomiquement les flags et durées transmis par les ISR
  noInterrupts();
  bool redPending = redActionPending;
  unsigned long redDur = redActionDurationMs;
  redActionPending = false;

  bool greenPending = greenActionPending;
  unsigned long greenDur = greenActionDurationMs;
  greenActionPending = false;
  interrupts();

  // Si on est en CONFIG et timeout 30min => retour STANDARD
  if (currentMode == MODE_CONFIGURATION) {
    if (millis() - lastActivityMillis >= CONFIG_TIMEOUT_MS) {
      Serial.println("CONFIGURATION timeout (30 min inactive) -> return to STANDARD");
      enterMode(MODE_STANDARD);
      return; // on sort, on a déjà traité la condition prioritaire
    }
  }

  // Si aucune action, on sort vite
  if (!redPending && !greenPending) return;

  // Mettre à jour lastActivity si une action utilisateur a eu lieu
  lastActivityMillis = millis();

  // Prioriser le bouton rouge si les deux sont pendants (on peut changer l'ordre si tu veux)
  if (redPending) {
    // switch selon le mode courant
    switch (currentMode) {

      case MODE_STANDARD:
        // STANDARD + RED
        if (redDur >= CONFIG_PRESS_MS) {
          // 10s -> MODE_CONFIGURATION
          Serial.println("RED 10s in STANDARD -> enter CONFIGURATION");
          enterMode(MODE_CONFIGURATION);
        } 
        else if (redDur >= LONG_PRESS_MS) {
          // 5s -> MAINTENANCE
          Serial.println("RED 5s in STANDARD -> enter MAINTENANCE");
          previousMode = currentMode;
          enterMode(MODE_MAINTENANCE);
        } 
        else {
          // short press: pas de changement
          Serial.print("RED short press in STANDARD (");
          Serial.print(redDur);
          Serial.println(" ms) - no mode change");
        }
        break;

      case MODE_CONFIGURATION:
      if (redDur >= LONG_PRESS_MS){
        // En CONFIG, tout appui RED ramène au STANDARD (selon ta règle)
        Serial.println("RED press in CONFIGURATION -> return to STANDARD");
        enterMode(MODE_STANDARD);
      }
      else {
        Serial.print("RED short press in CONFIGURATION (");
        Serial.print(redDur);
        Serial.println(" ms) - no mode change");      
      }
        break;

      case MODE_MAINTENANCE:
        // En MAINTENANCE, RED long -> revenir au previousMode
        if (redDur >= LONG_PRESS_MS) {
          Serial.println("RED 5s in MAINTENANCE -> return to previous mode");
          enterMode(previousMode);
        } 
        else {
          Serial.print("RED short press in MAINTENANCE (");
          Serial.print(redDur);
          Serial.println(" ms) - no mode change");
        }
        break;

      case MODE_ECONOMIQUE:
        // En ECO, règles : short RED -> STANDARD ; long RED -> MAINTENANCE
        if (redDur >= LONG_PRESS_MS) {
          Serial.println("RED 5s in ECONOMIQUE -> enter MAINTENANCE");
          previousMode = currentMode;
          enterMode(MODE_MAINTENANCE);
        } 
        if else (redDur >= LONG_PRESS_MS){
          Serial.println("RED short press in ECONOMIQUE -> return to STANDARD");
          enterMode(MODE_STANDARD);
        }
        else {
          Serial.print("RED short press in ECONOMIQUE (");
          Serial.print(redDur);
          Serial.println(" ms) - no mode change");
        }
        break;
    } // end switch red
  } // end if redPending

  // Puis traitement du bouton vert (si présent ET pas déjà pris en charge par red)
  if (greenPending) {
    switch (currentMode) {
      case MODE_STANDARD:
        // STANDARD + GREEN long -> ECONOMIQUE
        if (greenDur >= LONG_PRESS_MS) {
          Serial.println("GREEN 5s in STANDARD -> enter ECONOMIQUE");
          enterMode(MODE_ECONOMIQUE);
        } 
        else {
          Serial.print("GREEN short press in STANDARD (");
          Serial.print(greenDur);
          Serial.println(" ms) - no mode change");
        }
        break;

      case MODE_CONFIGURATION:
        if (greenDur >= LONG_PRESS_MS) {
          // En CONFIG, on considère que tout appui ramène au STANDARD (tu peux modifier)
          Serial.println("GREEN press in CONFIGURATION -> return to STANDARD");
          enterMode(MODE_STANDARD);
        } 
       /*
       else if (lastActivityMillis == 1800000){
        Serial.print("Ca fait 30 min que le systeme est inactif. Passage en mode STANDARD automatique");
        enterMode(MODE_STANDARD);
       }
       */
        else {
          Serial.print("GREEN short press in CONFIGURATION (");
          Serial.print(greenDur);
          Serial.println(" ms) - no mode change");
        }   
        break;

      case MODE_MAINTENANCE:
        // En MAINTENANCE, ignore GREEN (sauf si tu veux autre chose)
        Serial.print("GREEN press in MAINTENANCE (");
        Serial.print(greenDur);
        Serial.println(" ms) - ignored");
        break;

      case MODE_ECONOMIQUE:
        // ECO : GREEN n'a pas d'effet (ECONOMIQUE accessible seulement depuis STANDARD)
        Serial.print("GREEN press in ECONOMIQUE (");
        Serial.print(greenDur);
        Serial.println(" ms) - ignored");
        break;
    } // end switch green
  } // end if greenPending
}


void loop() {
change_mod();
}
