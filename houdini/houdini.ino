#include <PN5180.h>
#include <PN5180ISO15693.h>
#include <DFRobotDFPlayerMini.h>

#define VERSION 1.0

//-----
//DEBUG
//-----
bool debugMode = false;

//-------------
//BOUTON BYPASS
//-------------
const int boutonBypass = 26;

//---------
//CARTE SON
//---------
HardwareSerial HardSerial(1);
DFRobotDFPlayerMini myDFPlayer;

//------
//RELAIS
//------
const int inPin = 25;
bool unlocked = false;

//----------
//CAPTEUR US
//----------
#define SOUND_SPEED 0.034
const int trigPin = 32;
const int echoPin = 33;
long duree;
float distanceCm;
const int minDistance = 50;
const int maxDistance = 150;

//--------
//LECTEURS
//--------
const int nbrLecteurs = 2;

//La déclaration des pins se fait dans cet ordre : NSS - BUSY - RST
PN5180ISO15693 nfc[] = {
  PN5180ISO15693(2, 15, 4),
  PN5180ISO15693(21, 5, 22)
};

uint8_t uidConnus[][8] = {
  {0xE0, 0x4, 0x1, 0x0, 0x87, 0x2B, 0x48, 0x8D}, //Carte veston
  {0xE0, 0x4, 0x1, 0x0, 0x87, 0x2B, 0x40, 0xB8} //Carte chapeau
};

bool retourErreur = false;
bool veston = false;
bool chapeau = false;


void setup() {
  HardSerial.begin(9600, SERIAL_8N1, 16, 17); //vitesse, type, RX, TX
  
  Serial.begin(115200);
  Serial.println(F("=================================="));
  Serial.println(F("Version du : " __DATE__ " " __TIME__));
  Serial.print(F("Version du programme : "));
  Serial.println(VERSION);

  for (int lecteur = 0; lecteur < nbrLecteurs; lecteur++) {
    Serial.print(F("*** LECTEUR NUMERO "));
    Serial.print(lecteur);
    Serial.println(" ***");
    nfc[lecteur].begin();

    Serial.println(F("----------------------------------"));
    Serial.println(F("PN5180 - Réinitialisation matérielle..."));
    nfc[lecteur].reset();

    readInformation(lecteur);

    Serial.println(F("----------------------------------"));
    Serial.println(F("Activation du champ RF...")); //RF = Radio Fréquence
    nfc[lecteur].setupRF();
  }

  Serial.println(F("----------------------------------"));
  Serial.println("Initialisation capteur US");
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  Serial.println(F("----------------------------------"));
  Serial.println("Initialisation du relais");
  pinMode(inPin, OUTPUT);
  digitalWrite(inPin, HIGH);


  Serial.println(F("----------------------------------"));
  Serial.println("Initialisation du bouton de Bypass");
  pinMode(boutonBypass, INPUT);

  Serial.println(F("----------------------------------"));
  Serial.println("Initialisation du lecteur son");
  while (!myDFPlayer.begin(HardSerial)) {
    Serial.println("Impossible de démarrer");
    delay(500);
  }
  
  Serial.println("Démarrage terminé");

  Serial.println("Pour rentrer en mode Debogage");
  Serial.println("Appuyez sur le bouton bypass");

  for(int bp = 5; bp > 0; bp--) {
    Serial.print(bp);
    Serial.println("...");
    delay(1000);
  }
  
  if (digitalRead(boutonBypass) == 1 || true) {
    debugMode = true;
    Serial.println("MODE DEBUG ACTIF");
    Serial.println("Pour quitter le mode debug");
    Serial.println("Appuyez sur le bouton RESET");
    delay(2000); //Pour éviter de débloquer le mécanisme lors du passage sur la boucle du programme
  }
}

void loop() {
  if (debugMode == true) {
    debugLoop();
  } else {
    normalLoop();
  }

  delay(1000); //Pour économiser des ressources et des cycles on met un delai 
}

void normalLoop() {  
  if (digitalRead(boutonBypass) == 1) {
    unlock();
  }

  if (retourErreur) {
    retourErreur = false; 
  }
  
  for (int lecteur = 0; lecteur < nbrLecteurs; lecteur++) {
    uint8_t uid[8];
    ISO15693ErrorCode rc = nfc[lecteur].getInventory(uid);
    
    if (ISO15693_EC_OK == rc) { //Si il n'y a aucune erreur
      if (compare(uid, lecteur) == true) { //Comparaison des uid connus avec celui qui est passé
        if (lecteur == 0) {
          veston = true;
        } else if (lecteur == 1){
          chapeau = true;
        }
      }
    } else { //Si il y a une erreur
      retourErreur = true; //On le signale

      if (lecteur == 0) {
        veston = false;
      } else if (lecteur == 1) {
        chapeau = false;
      }

      unlocked = false;
    }
  }

  if (veston == 1 && chapeau == 1) {
    myDFPlayer.setTimeOut(500);
    myDFPlayer.volume(1) ; // fixe le son à 5 (10 maximum)
    myDFPlayer.play(1); // Son : Mettez vous en place pour la photo...
    
    if (distance() == true && unlocked == false) {
      myDFPlayer.play(1); //Son : 3... 2... 1... 'son de photo'
      unlock(); //Ouverture !
    }
  }
}

void debugLoop() {
  if (digitalRead(boutonBypass) == 1) {
    Serial.println("BYPASS");
    unlock();
  }

  if (retourErreur) {
    for (int lecteur = 0; lecteur < nbrLecteurs; lecteur++) {
      uint32_t irqStatus = nfc[lecteur].getIRQStatus();
      showIRQStatus(irqStatus);

      /*nfc[lecteur].reset();
      nfc[lecteur].setupRF();*/
    }
    retourErreur = false; 
  }
  
  for (int lecteur = 0; lecteur < nbrLecteurs; lecteur++) {
    uint8_t uid[8];
    ISO15693ErrorCode rc = nfc[lecteur].getInventory(uid);
    
    if (ISO15693_EC_OK == rc) { //Si il n'y a aucune erreur
      Serial.println("Comparaison des uids...");
      if (compare(uid, lecteur) == true) { //Comparaison des uid connus avec celui qui est passé
        if (lecteur == 0) {
          veston = true;
        } else if (lecteur == 1){
          chapeau = true;
        }
      }
    } else { //Si il y a une erreur
      retourErreur = true; //On le signale

      if (lecteur == 0) {
        veston = false;
      } else if (lecteur == 1) {
        chapeau = false;
      }

      unlocked = false;
    }
  }

  Serial.print("veston : ");
  Serial.println(veston);

  Serial.print("chapeau : ");
  Serial.println(chapeau);

  if (veston == 1 && chapeau == 1) {
    Serial.println("Veston et chapeau en place, en attente du capteur à ultrasons");
    myDFPlayer.setTimeOut(500);
    myDFPlayer.volume(1) ; // fixe le son à 5 (10 maximum)
    myDFPlayer.play(1); // Son : Mettez vous en place pour la photo...
    
    if (distance() == true && unlocked == false) {
      Serial.println("Coffre ouvert !");
      myDFPlayer.play(1); //Son : 3... 2... 1... 'son de photo'
      unlock(); //Ouverture !
    }
    
  }
}

bool compare(uint8_t currentUid[8], int j) {
  for(int i = 0; i < 8; i++) {
    if (debugMode == true) {
      Serial.print(currentUid[7-i]);
      Serial.print(":");
      Serial.print(uidConnus[j][i]);
      Serial.println(" / ");
    }
    
    if (currentUid[7-i] == uidConnus[j][i] && i == 7) {
      return true;
    }
  }
  return false;
}

bool distance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // trigPin active 10 micro secondes
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Lecteur de echoPin, nous retourne le temps de réponse en micro secondes
  duree = pulseIn(echoPin, HIGH);
  
  // Calcul de la distance parcourue
  distanceCm = duree * SOUND_SPEED/2;

  if (debugMode == true) {
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);
  }
  
  if (distanceCm > minDistance && distanceCm < maxDistance) { //Si la distance est comprise dans la plage choisie
    return true;
  }

  return false;
}

void unlock() {
  digitalWrite(inPin, LOW); // Fermeture du NO et ouverture du NC
  unlocked = true; 
  delay(1000); // On attend 1 seconde
  digitalWrite(inPin, HIGH); // Ouverture du NO et fermeture du NC
}


void readInformation(int lecteur) {
  Serial.println(F("----------------------------------"));
  Serial.println(F("Lecture de la version de production..."));
  uint8_t productVersion[2];
  nfc[lecteur].readEEprom(PRODUCT_VERSION, productVersion, sizeof(productVersion));
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
  nfc[lecteur].readEEprom(FIRMWARE_VERSION, firmwareVersion, sizeof(firmwareVersion));
  Serial.print(F("Version du logiciel = "));
  Serial.print(firmwareVersion[1]);
  Serial.print(".");
  Serial.println(firmwareVersion[0]);

  Serial.println(F("----------------------------------"));
  Serial.println(F("Lecture de la version de l'EEPROM..."));
  uint8_t eepromVersion[2];
  nfc[lecteur].readEEprom(EEPROM_VERSION, eepromVersion, sizeof(eepromVersion));
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
