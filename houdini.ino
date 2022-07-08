#include <PN5180.h>
#include <PN5180ISO15693.h>

#define VERSION 0.1

const int trigPin = 21;
const int echoPin = 4;
#define SOUND_SPEED 0.034
long duration;
float distanceCm;

const int numberReaders = 2;

//NSS - BUSY - RST
PN5180ISO15693 nfc[] = {
  PN5180ISO15693(16, 5, 17),
  PN5180ISO15693(32, 33, 25)
};

uint8_t savedUid[][8] = {
  {0xE0, 0x4, 0x1, 0x0, 0x87, 0x2B, 0x48, 0x8D}, //Carte veston
  {0xE0, 0x4, 0x1, 0x0, 0x87, 0x2B, 0x40, 0xB8} //Carte chapeau
};

bool errorFlag = false;

bool veston = false;
bool chapeau = false;

void setup() {  
  Serial.begin(115200);
  Serial.println(F("=================================="));
  Serial.println(F("Version du : " __DATE__ " " __TIME__));
  Serial.print(F("Version du programme : "));
  Serial.println(VERSION);

  for (int reader = 0; reader < numberReaders; reader++) {
    if (reader == 0){
      Serial.println(F("*** LECTEUR NUMERO 1 ***"));
    } else if (reader == 1) {
      Serial.println(F("*** LECTEUR NUMERO 2 ***"));
    }
    nfc[reader].begin();

    Serial.println(F("----------------------------------"));
    Serial.println(F("PN5180 - Réinitialisation matérielle..."));
    nfc[reader].reset();

    readInformation(reader);

    Serial.println(F("----------------------------------"));
    Serial.println(F("Activation du champ RF...")); //RF = Radio Fréquence
    nfc[reader].setupRF();
  }

  Serial.println(F("----------------------------------"));
  Serial.println("Initialisation capteur US");
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
}

void loop() {
  for (int reader = 0; reader < numberReaders; reader++) {
    if (errorFlag) {
      uint32_t irqStatus = nfc[reader].getIRQStatus();
      showIRQStatus(irqStatus);

      nfc[reader].reset();
      nfc[reader].setupRF();

      errorFlag = false;
    }

    uint8_t uid[8];
    ISO15693ErrorCode rc = nfc[reader].getInventory(uid);
    if (ISO15693_EC_OK == rc) {
      if (compare(uid, reader) == true) {
        if (reader == 0) {
          veston = true;
        } else if (reader == 1){
          chapeau = true;
        }
      }
      
    } else {
      errorFlag = true;

      if (reader == 0) {
        veston = false;
      } else if (reader == 1) {
        chapeau = false;
      }
    }
  }

  Serial.print("veston : ");
  Serial.println(veston);

  Serial.print("chapeau : ");
  Serial.println(chapeau);

  if (veston == 1 && chapeau == 1) {
    Serial.println("En attente du capteur US"); //US = Ultra Son
    if (distance() == true) {
      Serial.println("Déverrouillage");
    }
  }

}

bool compare(uint8_t currentUid[8], int j) {
  for(int i = 0; i < 8; i++) {
    /*Serial.print(currentUid[7-i]);
    Serial.print(" : ");
    Serial.print(savedUid[j][i]);
    Serial.print(" / ");*/
    if (currentUid[7-i] == savedUid[j][i] && i == 7) {
      return true;
    }
  }
  return false;
}

bool distance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  // Prints the distance in the Serial Monitor
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);

  if (distanceCm > 50 && distanceCm < 150) {
    return true;
  }

  return false;
}

void readInformation(int reader) {
  Serial.println(F("----------------------------------"));
    Serial.println(F("Lecture de la version de production..."));
    uint8_t productVersion[2];
    nfc[reader].readEEprom(PRODUCT_VERSION, productVersion, sizeof(productVersion));
    Serial.print(F("Version de production = "));
    Serial.print(productVersion[1]);
    Serial.print(".");
    Serial.println(productVersion[0]);

    if (0xff == productVersion[1]) { // if product version 255, the initialization failed
      Serial.println(F("Echec de l'initialisation !?"));
      Serial.println(F("Redémarrez l'ESP32..."));
      Serial.flush();
      exit(-1); // halt
    }
  
    Serial.println(F("----------------------------------"));
    Serial.println(F("Lecture de la version du logiciel..."));
    uint8_t firmwareVersion[2];
    nfc[reader].readEEprom(FIRMWARE_VERSION, firmwareVersion, sizeof(firmwareVersion));
    Serial.print(F("Version du logiciel = "));
    Serial.print(firmwareVersion[1]);
    Serial.print(".");
    Serial.println(firmwareVersion[0]);

    Serial.println(F("----------------------------------"));
    Serial.println(F("Lecture de la version de l'EEPROM..."));
    uint8_t eepromVersion[2];
    nfc[reader].readEEprom(EEPROM_VERSION, eepromVersion, sizeof(eepromVersion));
    Serial.print(F("Version de l'EEPROM = "));
    Serial.print(eepromVersion[1]);
    Serial.print(".");
    Serial.println(eepromVersion[0]);
}

void showIRQStatus(uint32_t irqStatus) {
  Serial.print(F("IRQ-Status 0x"));
  Serial.print(irqStatus, HEX);
  Serial.print(": [ ");
  if (irqStatus & (1<< 0)) Serial.print(F("RQ "));
  if (irqStatus & (1<< 1)) Serial.print(F("TX "));
  if (irqStatus & (1<< 2)) Serial.print(F("IDLE "));
  if (irqStatus & (1<< 3)) Serial.print(F("MODE_DETECTED "));
  if (irqStatus & (1<< 4)) Serial.print(F("CARD_ACTIVATED "));
  if (irqStatus & (1<< 5)) Serial.print(F("STATE_CHANGE "));
  if (irqStatus & (1<< 6)) Serial.print(F("RFOFF_DET "));
  if (irqStatus & (1<< 7)) Serial.print(F("RFON_DET "));
  if (irqStatus & (1<< 8)) Serial.print(F("TX_RFOFF "));
  if (irqStatus & (1<< 9)) Serial.print(F("TX_RFON "));
  if (irqStatus & (1<<10)) Serial.print(F("RF_ACTIVE_ERROR "));
  if (irqStatus & (1<<11)) Serial.print(F("TIMER0 "));
  if (irqStatus & (1<<12)) Serial.print(F("TIMER1 "));
  if (irqStatus & (1<<13)) Serial.print(F("TIMER2 "));
  if (irqStatus & (1<<14)) Serial.print(F("RX_SOF_DET "));
  if (irqStatus & (1<<15)) Serial.print(F("RX_SC_DET "));
  if (irqStatus & (1<<16)) Serial.print(F("TEMPSENS_ERROR "));
  if (irqStatus & (1<<17)) Serial.print(F("GENERAL_ERROR "));
  if (irqStatus & (1<<18)) Serial.print(F("HV_ERROR "));
  if (irqStatus & (1<<19)) Serial.print(F("LPCD "));
  Serial.println("]");
}
