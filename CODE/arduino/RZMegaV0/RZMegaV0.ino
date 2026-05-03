/*
- V RZ ss RPI
- gauche-droite sens de la marche - merci RCO asso robo club ouest
- var modeCom pour choisir type de com  et modeRzB pour passer le type de fonctionnement  
- rappel : débrancher tous les RX quand téléversement sur une carte
- Bibliothèques : Encodeur (cf twoKnobs)-Encoder ** Sonar - Newping** servomoteur-Servo ** I2C - Wire ** 
                  LCDviaI2C - LiquidCrystal_I2C ** PuceLED - Adafruit_Neopixel ** Capteur de poids - HX711*/

#include <Encoder.h> 
#include <NewPing.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <HX711.h>
//#include "pitches.h"... ZZ REVOIR Buzzer pb compile avec #include <TonePitch.h>... https://www.arduino.cc/en/Tutorial/BuiltInExamples/toneMelody 


          /***Déclarations des objets Constantes et Variables générales***/ 
//Led programmable
int ringLedPin = 6, ringNumPixels = 12, rubanLedPin = 10, rubanNumPixels = 15;
Adafruit_NeoPixel ringPixels = Adafruit_NeoPixel(ringNumPixels, ringLedPin, NEO_RGB + NEO_KHZ800); // Led Ring
Adafruit_NeoPixel rubanPixels = Adafruit_NeoPixel(15, 10, NEO_RGB + NEO_KHZ400); //Led Ruban - LED WS2811 1903 - programmable - 60 mA par Led 15

//Detecteur de poids - force
#define DOUT 11 
#define CLK  12 
HX711 scale;                        //DOUT et CLK nécessaire pour bibliothèque capteurs de poids / force - utlisé pour la laisse du robot

//Encodeur des mvt moteurs
Encoder knobLeft(2,19);             //Moteur - creation des deux encodeurs avec les num. des pins spécifiques de mega ici sur 2 interruptions par encodeur
Encoder knobRight(18,3);            //une inversion entre canal A et B change +- des tics

//LCD
LiquidCrystal_I2C lcd(0x3F,16,2);   //ref et nbre caractères et lignes
#define SLAVE_ADDRESS 0x12          //défini l'adresse Mega pour I2C


//Ultrason (Us) - objets et variables associées pour renvoyer valeur sous conditions -PourInfo- trig: pair, couleur filet - echo : nbre impair, couleur pleine orange 
#define SONAR_NUM 9                 // Nbre de capteur Us
#define MAX_DISTANCE 200            // Max distance en cm A choisis  - max réel 400 mais var 200-400 pas interval utile
#define PING_INTERVAL 40            // Ping intervalle choisis en milliseconds 40ms mini à 115200 baud ou peut descendre à 33?
NewPing sonar[SONAR_NUM] = {        //sonars dans le tableau - sens horaire 
  {32, 33, MAX_DISTANCE}, {34, 35, MAX_DISTANCE}, {36, 37, MAX_DISTANCE}, //AvG-AvC-AvD
  {38, 39, MAX_DISTANCE}, {40, 41, MAX_DISTANCE}, {42, 43, MAX_DISTANCE}, //ArD-ArC-ArD
  {30, 31, MAX_DISTANCE}, {28, 29, MAX_DISTANCE}, {26, 27, MAX_DISTANCE}  //StE-StD-StG
};
          // Ultrason  Nom - Zone de danger 
const char* nomSonar[SONAR_NUM]={"AvG","AvC","AvD","ArD","ArC","ArG","StE","StD","StG"};  // Nom : tableau de string utilisé pour les prints
const unsigned int sonarDanger[SONAR_NUM]={15,10,15,15,10,15,20,20,20};                   // Danger : Limite choisis - cm 

         // Ultrason code du niveau de danger : coder l'état des voyants de sécurité des capteurs on crée un tableau codé : 0 non défini (artefact), 1 <danger, 2 intermédiaire, 3 pas de pb
unsigned int transfertSonarCode[SONAR_NUM]= {0};                                          // equivalent à {0,0,0,0,0,0,0,0,0} - uint8_t
unsigned int transfertSonarCodeOld[SONAR_NUM]={0};                                        //old permet de n'envoyer info que si chgment 

         // Variables de suivi du sonar
unsigned long pingTimer[SONAR_NUM];
unsigned int cmA[SONAR_NUM];                                                              //conserve distance en cm -cmA car on peut ajouter des var B et C avec même objectif
uint8_t currentSensor = 0;                                                                //capteur actif
int compteurNbCycleSensor = 0;                                                            //utile pour suivre le nombre de mesure par capteur 


//Mode de communication du Robot : Non défini = 0 - Bluetooth =1(liaison serie avec emetteur bluetooth) - Internet = 2 (liaison série avec  RPI) ****/
int modeCom; 
                 //Variables liées à la communication bluetooth
char oldBluetoothData, bluetoothData;                                                     // commande bluetooth reçue  sur port Serial2 
boolean bluetoothOk;                                                                      //bluetooth disponible
                 // Variables liées à la communication via RPI
const byte numChars =32;                                    // pas limitant en taille finale et pas limitant en durée (remplie avec zero si besoin) 
char oldRpiData[numChars], serialRpiData[numChars];         // valeurs netoyées de marque de début '$' et de fin '#' utilisé pour les envois de RPI sur série
boolean newRpiData = false, rpiOk = false;                  //rpi disponible en écoute pour echange internet via node-red
int lgRpiData = 0;                                          //lgeur rpi data après prépa val avec le caractére nul et sans les marque de debut et fin de message


//Mode de fonctionnement du robot - var qui influe de nombreuses fonctions
byte oldModeRzB, modeRzB;                     // s'utilise avec enum qui s'appui sur des constantes ayant une valeur byte pour entrer dans cas de switch case
enum {ARRET = 0 , VEILLE = 1 , SURVEILLANCE = 2 , AUTONOME = 3 , SUIVEUR = 4 , TELECOMMANDE = 5 }; // renvoie valeur de constante
boolean traitementArret = false;              // qd modeRzb Arrêt, ctrl l'initialisation.


//Temps - Variables de temps 
unsigned long currentMillis, oldMillis;       //actualisation avec currentMillis=millis()
int dure;  // calculé entre deux loops
unsigned long lcdDureeMini = 3000, lcdTempoTimer = 6000, lcdTimer; //-lcdDureeMini: provoque chgement etat allume eteind - lcdTempotimer: tps avant lecture - lcd timer: durée depuis dernière lecture

//LCD variables non liées au temps
int dataLcdReceived, lcdNbBouclescoll;        // lcdNbBouclescoll compteur de boucle incrémention scroll LCD (pour mvt)
byte lcdEtat;                                 //1 quand utilisable sinon 0

// Test - à utiliser uniquement pour debug
boolean test = 0, testRpi = 0, testLcd = 0, testUs = 0, testMoteur = 0, testPosTete = 0;
boolean testEncodeur = 0, testactiveMicro = 0, testDetectMvtIr = 0, testPhotoResist = 0, testOdometrie = 0;

/*Encodeurs & interruptions associées  - Traitement des nbre de tick 
- source club robot de l'ouest et https://makeitwithconrad.fr/tutoriel/robot-se-reperant-dans-lespace-grace-a-ses-encodeurs/ 
- Numéros des intéruptions (voir onenote programation arduino interruption)
- canalA encodeur Gauche - Pin 19 equivallent interrupt 4 - canalA encodeur Droit - equivallent interrupt 5 
- canalB encodeur Droit - equivallent interrupt 1 mais pas use en interrupt - canal B encodeur Gauche - equivallent interrupt 0 mais pas use en interrupt
- pin 19 - num interruption du pin 19 moteur G canal A interrup5 pin 18**/
const int pinMGcanA = 19, pinMDcanA = 18, pinMDcanB = 3, pinMGcanB = 2, interrupGA = 4, interrupDA = 5;
const int canalBG = 0, canalBD = 1; //pin 3 - canal B moteur Droit peut être mis sur pin numérique sans interruption ds ce programme
boolean odometrie = 0, oldOdometrie = 0 ; // activation odometrie inclue activation test variation encodeurs
int codeDir;//0 pour arrêt 1 pour calcul transmis à moteur

/** attention valeur à étalonner  en  réel sur le robot (qd modif moteur, poids,...)
 -  remettre const  qd val fixée 14 tic/mm  soit : constant pour transfo nbre tic en millimetre (vérifier valeur par test sur 1m de déplacement)
 -  coeff Angle -  tour 360° 4480 tic 1 tic = 0.08035 °// 72.5 tic/mm constante pour transfo nbre tic en degrés (vérifier valeur par test sur dix tours)
 -  Variables (compteurs) qui vont stocker le nombre de tics comptés pour chaque codeur*/
double coeffGLong = 0.0712, coeffDLong = 0.0712, coeffGAngl = 0.08035, coeffDAngl = 0.08035;//0.0138;//imp je met petite différence d'angle pour éviter les 0
long comptD = 0, comptG = 0, oldComptD = 0 , oldComptG = 0;
int nAsser;//nbre de boucle asservissement
double xR = 0. , yR = 0., xC = 1000. , yC = 1000.; // Variables contenant les positions du robots en x et y (R pour robot), idem cible avec val différente pour test
double dDist = 0. ,  dAngl = 0.;                   // variables contenant les delta : position et angle
double orientation = 0.;                           //Variable contenant le cap du robot
double aRC = 0. ,  consigneOrientation = 0.;       // aRC angle entre robot et cible et consigneOrientation
int signe = 1;                                     //Variable permettant de savoir si la cible est à gauche ou à droite du robot.
double distanceCible = 0.;                         //Variable contenant la distance entre le robot et la cible
double coeffP = 5.0 , coeffD = 3.0; //Variables parametrant l'asservissement en angle du robot               //Variables parametrant l'asservissement en angle du robot
String rzIni, rzCible, rcActuel; //chaine de type x158y210                   //3 chaines de type x158y41 pour ref position
double oR = 0.,erreurAngle = 0., erreurPre = 0., vAngl= 0. ,  deltaErreur = 0.;//vAngl et delaErreur - Variables utilisées pour asservir le robot
int cmdG = 0, cmdD = 0; ////Variables représentant les commandes à envoyer aux moteurs
// Fin Déclaration encodeurs

/************ventilo raspberry v ou ventilo controler par transistor HS****************/
//const int ventiloRpiPin = 49;// inactivé - directement sur RPI
//boolean ventiloOn=1 ;//ds setup à 0  too  activ par rpi zz a faire

/********* Création des servomoteurs / articulation - pin attach ds setup ainsi que val ini et correction
Base code  http://www.arduino.cc/en/Tutorial/Sweep  */
Servo servoTGD, servoTHB, servoBase; // création des 3 servomoteurs : thb tete haut bas tgd tegauche droite et base pour le corps
boolean contrManuelServoTGD=false, contrManuelServoTHB=false, contrManuelServoBase=false; // pour inverser !var initialiser dans le setup ?
// variable de position  du servo 0 position milieu du mvt potentiel, delta lié à fixation ini servo entre le 90 du moteur et le 90 de l'axe du robot
// positionReeldesServos suffixeR - position réel , Max et minimum acceptable ds le mvt, une ligne de var par servo angle reel - degre reel lu à un instant t
int posServoTGD,correctionPosServoTGD, posServoTGDR, oldPosServoTGDR, posServoTGDRMax=210, posServoTGDRMin=-30, degreTGD;  // MAX MIN ATTENTION  AUX SIGNES    
int posServoTHB,correctionPosServoTHB, posServoTHBR, oldPosServoTHBR, posServoTHBRMax=80, posServoTHBRMin=20, degreTHB;    // MAX MIN ATTENTION  AUX SIGNES 
int posServoBase,correctionPosServoBase, posServoBaseR, oldPosServoBaseR, posServoBaseRMax=80, posServoBaseRMin=20, degreBase;  // MAX MIN ATTENTION  AUX SIGNES   

/***********Moteurs propulsion constructeur*************************
 - acceleration : de 1 à 4 , 4 max pas de délai) 0 possible indirectement pour arrêt moteur
 - boiteVitesse : val de 0 à 9 - 1 seul caractère pour vitesse de réaction
 - directionMoteur val de : 0 arrêt à 8 sens des aiguilles d'une montre en partant de l'avant.*/
char acceleration, boiteVitesse, directionMoteur;
boolean autoriseMoteur = false;
//intermédiaire du constructeur - initialisation tableau de char qu'au moment de la déclaration - pointeur 1er caractère 0 et non 1 par contre declaration nbre caractère =1 pour 0/
char transMoteurP[8] =  {'<','0',',','0',',','0','>'}, oldTransMoteurP[8] = {'<','0',',','0',',','0','>'}, transMoteurAuto [20]; //conservation valeur de transMoteur antérieure

/*********Champs d'échange libre************/
String receptionMessage; //ini échange de message par boite send voir le caractere #

/**********LED - Gestion des Led neopixel, attention aux delays si utilisé, ils impactent tout le programme
- NeoPixel color format & data rate. See the strandtest example for
- verif les val possible sur https://wiki.mchobby.be/index.php?title=NeoPixel-UserGuide-Arduino&mobileaction=toggle_view_desktop 
- Chaques objets neopixels sur des broches différentes ******/
#define DELAYVAL 50 // Time (in milliseconds) pour effets sur pixels - ATTENTION impact tous le programme / durée loop original 500

/*********LED ring neopixel - variables  strip_a=Adafruit_Neopixel(ringNumPixels, ringLedPin, Neo_RGB + NEO_KHZ800)*****/
// 60 ma par led 12 led max 600 ma alors que la 5V de arduino max 500 prévoir alim externe- ini pin 10 - ini ringLedPin
boolean activeRing = 0;                   //activer ou non
String ringCouleurLibre, couleurRing;     // revoir pas ds setup
int intensiteRing, frequenceRing;         // int luz et frequence
int redRC, greenRC, blueRC;               // val RGB du ring
unsigned long ringTpsFin;                 // actualisation avecmillis()-evite affichage long

/*********creation objet lumière bande neopixel Adafruit_NeoPixel strip_b  Adafruit_Neopixel(15, 10, NEO_RGB + NEO_KHZ400 );*****/
boolean activeRubanLed=0;//activer ou non
boolean rubanFuite= false, rubanSuivre= false, rubanTourne= false, rubanEmotion= false;
int intensiteRubanLed, frequenceRubanLed;
int redRB,greenRB, blueRB;
unsigned long rubanTpsFin; // actualisation avecmillis()-evite affichage long
//#define DELAYVAL 50// Time (in milliseconds) pour effets sur pixels - ATTENTION impact tous le programme / durée loop original 500

/********Emotion RGB - ajout possible clignotement fondu... *******/
const int emotionBluePin = 46, emotionGreenPin = 44, emotionRedPin = 45;
int redMValue, greenMValue, blueMValue;
unsigned long boutonTpsFin;
/*********Controle manuel de la boule émotion  (Gyrophare)***********/
boolean boutonControl=1, boutonVert, boutonJaune, boutonRouge;//bouton G - J - R
int frequenceGyro, nbBoucleLoop;// frequence gyrophare (clignotement)- nb boucle use width gyrophare frequence

/********Alerte Sonore à générer par la tablette*******/
boolean alerteSonore;// envoyer son ds j



/**********Cameras actives*********/
boolean cameraHD;// camera  sur tablette activation C  voyant i confirm. validation

/********Detecteur de mvt avec infrarouge****************/
boolean detecteurMvt;// activer le détection de mvt bouton p et retour sur voyant P
const int mvtIrPin = 8;
int mvtIrVal=0;

/********Detecteur de luz arduino avec photoDiodeResitance pour présence absence luz et ou smartphone cellule plus précise  zz ************/
boolean photoRactive, photoSmartActive; // si smartActive, transmission tablette capteur luz smartphone en priorité sur celui d'arduino
int pinPhotoR=A1, valMinPhtoR=1024, valMaxPhotoR=0, valIntLuzPhotoR, oldValIntLuzPhotoR;   //pin de connexion pour la photorésistance en f°intensité luz - On initialise à l'envers pour restreindre champs min max 
int intLuzRSmart, oldValIntLuzRSmart;

/***********activeMicro*******/
boolean activeMicro;              //état activation des micros
int oldIntSmartSon;               //valeur reçu depuis smartphone calibrage peut être différent
int choixMicro;                   //1 pour micro et 2 pour Smart qd via RPI
/* Pour les micros du robot indépendant du smartphone, transmission si chgement de valeurs*/
/****Active micro en dev micro gche et droit revoir si pb choix micro****/
const int microAvIntSon = A1, microGcheIntSon = A2, microDrtIntSon = A3;  //A1 pin micro Avant gris -A2 pin micro Gauche - A3 pin micro Droit
int oldIntMSonAv, oldIntMSonGche, oldIntMSonDrt;                          //val en % du son reçu par micro Av

/**************Emetteur Active Buzzer 
- frequence du buzzer : 0 pas de delai entre impulsion et 10 max
- intensiteBuzzer : 0 inaidiblz et 10 max**********/
boolean activeBuzzer;//activer buzzer de RZ
const int activeBuzzerPin = 7;
unsigned long buzzerTpsFin;// var avec millis
int frequenceBuzzer, intensiteBuzzer;
//Bibliothèque Active Buzzer : Attention l'include prend de nombreuses places mémoire - zz activer ??
int melody[] = {262, 196,196, 220,196,40, 247, 262};//on entre fréquence des notes à partir d'un tableau de note
int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4 };// tableau de mémorisation de la durée des notes : 4 = noire, 8 = croche, etc.:



/**************************** Setup*******************************/
void setup() {
 
   // ini mode de fonctionnement
     modeCom = 0; // par défaut à 0 non défini - à 1 pour Bluetooth à 2 pour internet - voir dans loop pour modifier en cours de programme 
     modeRzB =oldModeRzB= 0;// Mode de fonctionnement du robot par défaut à arrêt moteur
   //iniTabBordRpi();
   //ini temps currentMillis
   currentMillis=millis();
   oldMillis=millis();
  
   //ini servomoteur - posServoTGD affichage apparant écran, mais on passe par posServoTGDR qui correspond au réel moteur et aux commandes passées à tablette décalage de 90
  // attention si changement à la main peut-être besoin d'écrire les val avec delais pour éviter mvt violent? zz
  servoTGD.attach(4);// la bibliothèque servo met par defaut dès l'attach le servo à 90° pour un mvt 0 à 180
  //degreTGD = servoTGD.read();
  correctionPosServoTGD  = 30;
  oldPosServoTGDR= posServoTGDR= degreTGD= correctionPosServoTGD; //permet d'initaliser une version initiale de posServoTGDR
  posServoTGD=degreTGD-correctionPosServoTGD;
  //if (testPosTete){Serial.print("servoangle : ");Serial.println (degreTGD);}
  servoTGD.write(correctionPosServoTGD);
  delay(300);
  servoTGD.detach();
    //iniservoTGD();
 

  /******base servo  zz avril à etalonner*****/
  servoBase.attach(9);            // la bibliothèque servo met par defaut dès l'attach le servo à 90° pour un mvt 0 à 180
  correctionPosServoBase =0;     // voir si zero vertical ou horizontal
  oldPosServoBaseR= posServoBase= degreBase= correctionPosServoBase;//servoBase.read();//initialise les valeurs à la position courante dans le setup pour éviter mvt intempestif
  servoBase.write(correctionPosServoBase);// varie de 0 à 90
  posServoBase = posServoBase+correctionPosServoBase;
  contrManuelServoBase = false;
  delay(1000);// ini 200
  servoBase.detach();
   
   //servoTHB Mvt Vertical zz déterminer les min max
  servoTHB.attach(5);// la bibliothèque servo met par defaut dès l'attach le servo à 90° pour un mvt 0 à 180
  correctionPosServoTHB=70;
  oldPosServoTHBR= posServoTHB= degreTHB= correctionPosServoTHB;//servoTHB.read();//initialise les valeurs à la position courante dans le setup pour éviter mvt intempestif
  posServoTHB = posServoTHB+correctionPosServoTHB;
  //position thb fonction corps 20 tete vers avant qd corps vertical 45 idem qd corps à l'équilibre un peut en arrière, 80 qd corps couché  vertical  et 85 coucher (zz averif) on garde la position précédente
  //servoTHB.write(degreTHB); // inutile ici car val basé sur lecture garder pour test ini
  servoTHB.write(correctionPosServoTHB);//test
  delay(100);
  servoTHB.detach();
  contrManuelServoTHB = false;
    
 
   //Moteurs deplacement
    acceleration='1';//de 1 à 4 , 4 max pas de délai) 0 possible indirectement pour arrêt moteur 
    boiteVitesse='0';//val de 0 à 9 - 1 seul caractère pour vitesse de réaction
    directionMoteur = '0';//  
    comptG  = -999;//variable position initial arbitraire seront automatiquement initialisée ds le loop (utilisé en mode oldPosition)
    comptD = -999;

  //ini main serial de la mega et liaison rpi si connexion usb
   Serial.begin(9600);//original 115200 - alternative 9600
   //Serial.print  ("*n"+String("Activer ON")+"#");//message pour champ infos de nodeRed sur RPI
  
   Serial2.begin(9600); //obligatoire - début liaison bluetooth
   bluetoothOk=true;// delay ? - zz A faire ajouter variable tablette OK à true ici et passe à false avec commande depuis RPI idem pour blutooth /économie
   //desactive a ce stadeiniTablette();//mise à zero de la tablette et des variables arduinos associées
 
  //ini Serial3 avec Uno - zz desactivation pour test bluetooth
   Serial3.begin(115200); //compilateur marque erreur si on a pas selectionné mega! - vini 9600


 /*l'emplacement de Serial1 et Serial4 utilisés pour interruptions liées aux encodeurs 
 Initialisation des pins des encodeurs canaux B des moteurs Gauche et Droit - source : - robotique club de l'ouest
  on peut réaliser le travail sur un canal et on observe la position du second permet d'économiser deux interruptions.
  L'idéal est d'utiliser des encodeurs non portée par le moteur (évite les erreurs due aux patinage quand rencontre obstacle.
  */
  
   /*** ini des autres capteurs et actionneurs - ini. des booleans *****/

//ini ring led neopixel lumière
redRC=0;greenRC=255;blueRC=0; // val RGB du ring in à vert
activeRing=1; ringLedPin=6; ringNumPixels=13;////create a new NeoPixel object dynamically with these values

//ringPixels = new Adafruit_NeoPixel(ringNumPixels, ringLedPin, ringPixelFormat);
//lancement des fonctions par indirection (->)
   //lancement des fonctions par indirection (->)
   ringPixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
   ringPixels.show();//en debut initialise à vide l'affichage 
   ringPixels.fill(ringPixels.Color(255,0,0),4,5);   // allume la led 1,2,3 et 4  en vert
   ringPixels.show();
   ringTpsFin=millis()+2000;

   activeRing=1;
   greenRC=255;redRC=0;blueRC=0;intensiteRing=50;//initialise val à bleu mais bonjour() modif
   ringColor();
     
 //ini ruban led neopixel lumière
   //create a new NeoPixel object dynamically with these values:
   activeRubanLed=1;
//   rubanPixels = new Adafruit_NeoPixel(rubanNumPixels, rubanLedPin, rubanPixelFormat);
   //lancement des fonctions par indirection (->)
   rubanPixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
   rubanPixels.clear();//en debut initialise a vide affichage 
   rubanPixels.fill(rubanPixels.Color(255,0,0),4,5);   // allume la led 1a 4  en vert
   rubanPixels.show();
   rubanTpsFin=millis()+2500;
   activeRubanLed=1; 
   greenRB=255;redRB=0;blueRB=0;intensiteRubanLed=50;//initialise val à vert
   rubanLedColor(); 
   
      /* Définition des couleurs */
int RED[3] = {255, 0, 0}; // Couleur Rouge
int GREEN[3] = {0, 255, 0}; // Couleur Verte
int CYAN[3] = {0, 255, 255}; // Couleur Cyan
int YELLOW[3] = {255, 125, 0}; // Couleur Jaune
int ORANGE[3] = {255, 40, 0}; // Couleur Orange
int PURPLE[3] = {255, 0 , 255}; // Couleur Violette
//int PINK[3] = {255, 0, 100}; // Couleur Rose
int BLUE[3] = {0, 0, 255}; // Couleur Bleu
int WHITE[3] = {255, 255, 255}; // Couleur Blanche  
 
 
 //Emotion RGB init (gyrophare)- Emetteur
   pinMode(emotionRedPin, OUTPUT);
   pinMode(emotionGreenPin, OUTPUT);
   pinMode(emotionBluePin, OUTPUT);
   frequenceGyro=0; //pas de clignotement max 10

 // Capteur de force HX711 (laisse) objet déclaré en début de programme

   // 2. Adjustment settings
const long LOADCELL_OFFSET = 50682624; //calibration trouver avec autre programme
const long LOADCELL_DIVIDER = 5895655;// division pour calibration ?

// 3. Initialize library - zz j'ai remplacé loadcell par scale
scale.begin(DOUT, CLK);
scale.set_scale(LOADCELL_DIVIDER);
scale.set_offset(LOADCELL_OFFSET);

// 4. Acquire reading - v bloquante
//Serial.print("Weight: ");
//Serial.println(loadcell.get_units(10), 2);

// 4. Acquire reading without blocking - ZZ renvoi sur serial intégration à faire
if (scale.wait_ready_timeout(1000)) {
    long reading = scale.get_units(10);
    Serial.print("Weight: ");
    Serial.println(reading, 2);
} else {
    Serial.println("HX711 not found.");
}
    
 //Detecteur de mvt - recepteur - Detecteur de mvt - il faut 1 seconde d'attente pour le calibrer
    pinMode (mvtIrPin,INPUT);

 //MicroAv Big sound - recepteur
    pinMode(microAvIntSon, INPUT);  //reception type potentiometre selon vol sonore
    pinMode(microGcheIntSon, INPUT);  //reception type potentiometre selon vol sonore
    pinMode(microDrtIntSon, INPUT);  //reception type potentiometre selon vol sonore
    oldIntMSonAv = 0;
    oldIntMSonGche = 0;
    oldIntMSonDrt = 0;
    
  //Detecteur de luz Smart 
  photoSmartActive= 0; // active transmission tablette capteur luz smartphone en priorité sur celui d'arduino
  oldValIntLuzRSmart=0;
  intLuzRSmart=0;

  //Detecteur de luz arduino
  photoRactive=0;//d
  oldValIntLuzPhotoR=0;

 //Buzzer - Emetteur son - zz en commentaire pour confort desactive 
    pinMode (activeBuzzerPin, OUTPUT);
    frequenceBuzzer=0;
    intensiteBuzzer=3;
    buzzerTpsFin=0;
       
 
 //Initialisation tablette & variables (utilisé aussi si arrêt)
   //iniTablette();
  
 /****** ini crista I2C LCD zz en commentaire qd pb*********/
   byte lcdEtat = 1;// utile ? déjà avant le setup
   lcd.init();
   lcd.begin(16,2);   // iInit the LCD for 16 chars 2 lines
   Wire.begin(SLAVE_ADDRESS);// ini communication I2C avec LCD
   //Bonjour
   char ligne1[] ="A votre service"; // char plus light que string mais ne pas oublié alors tableau de char
   char ligne2 []="Super RZ";
   lcdEcrire(ligne1,ligne2);//envoi message
   //courte pause
   delay(1000);
   //bonjour();
                        // initialize the lcd

   // Print a message to the LCD.
   lcd.backlight();
   lcd.print("Ici RZ, Bonjour!");
   for (int positionCounter = 0; positionCounter < 16; positionCounter++) {lcd.scrollDisplayLeft();}    // décale d'une position vers la gauche
   delay(2000);
   lcd.clear();
   lcd.noBacklight();
   if(test){Serial.println("fin setup");};
   delay(200);
}// end setup


/****************************************Loop**************/
void loop() {
   delay(50);//temps transfert réinitialisation ini 200
   //temps
   dure=millis()-oldMillis;// durée loop précédent
   oldMillis=currentMillis;//tps au début du tour précédent
   currentMillis=millis(); // tps au début du tour actuel
   int deltaMillis=currentMillis-oldMillis; // ? idem dure - revoir
   
   //ModeRzB - actu
   oldModeRzB=modeRzB;//actualisation du mode

   //tests divers
        //Serial.println("<M Serial Ok>");
   
   if (testPosTete){//servo tete de -30(vers droite ) à 210 vers gauche pour servo
    int teteLu=servoTGD.read(); int THBLu=servoTHB.read();int BaseLu=servoBase.read();
    Serial.print("TeteLu: ");Serial.println (teteLu); Serial.print(" BaseLu : ");Serial.println (BaseLu);Serial.print(" THBLU : ");Serial.println (THBLu);
   //servoTGD.write(-120);
   //servoTHB.write(-120);
   servoBase.attach(9);
   delay(1000);
   posServoBase=20;
   servoBase.write(posServoBase);
   delay(2000);
   posServoBase=170;
   servoBase.write(posServoBase);
   delay(2000);
   servoBase.detach();
   
   servoTGD.attach(4);
   posServoTGDR=-50;//30
   servoTGD.write(posServoTGDR);
   delay(2000);
   servoTGD.detach();
      
   servoTHB.attach(5);  
   posServoTHB=20;//85
   servoTHB.write(posServoTHB);//min 20 quelques soit position équilibre qd base au repos 45 qd vertical 2O, qd base horizontal 70? .ss ressort
   delay(2000);
   servoTHB.detach();   
     }
   /*
    //Serial.println("<MTps: "+String(dure)+">");//test
  
   //Serial.println("Val Moteur: "+String(transMoteurP));//test
   //int i=0;
   // for(i = 0 ; i < 8; i++){Serial.print(transMoteurP[i]) ; }  //test
   //servoTGD.detach(); zz modifjuin2021 car inutile? peut qd non mis limité utilsation pin 11 et 13 sur mega
   /*if(testRpi){
   Serial.println("<MW,"+String(dure)
               +","+String(modeRzB)
               +","+String(modeCom)
               +","+String(serialRpiData)
               +","+String(transMoteurP[1])
               +","+String(transMoteurP[3])
               +","+String(transMoteurP[5])+">");delay(20);///delay20
               };
    Serial.print("posServoTGDRmoteur :"); Serial.println(180-posServoTGDR);Serial.print("posServoTGD Tablette :"); Serial.println(posServoTGD);
   int degreTGD = servoTGD.read();Serial.print("servoangle : ");Serial.print (degreTGD)    */ 
 
      /*sortie des messages LCD si lcdDureMini passé...*/ 
    if ( (lcdTimer <=millis())|| lcdEtat==0){// supprimer le if pour vérifier si dysfonctionnement lié au timer  avant de le remettre || à la place de &&
    lcd.clear();//efface & remet au début (lcd.home que la seconde partie)
    lcd.noBacklight();//éteind le rétroéclairage - économise batterie
    }//end if

   if (Serial3){
      if(String(transMoteurP)==String(oldTransMoteurP)){if(testMoteur){Serial.println("serialOK-pas de modif");};}//Serial.print("IDENTIQUE");
      else{
      int j ;
      //Serial.println("ActualisationOldMoteur1 et envoiuno: "+String(transMoteurP)+">");//test nouvelle valeur avant envoie
      for(j = 0 ; j < 8; j++){oldTransMoteurP[j] = transMoteurP[j] ;}// taille tableau transmoteur = 8
      envoiUno();
      //if(testMoteur){Serial.println("<M dsloop-envoiuno: "+motr+"zz>");}//attention serial motr qui fini par > peut générer pbaffichage sur main du message envoyé
      delay(20);
      Serial.println("<M_Serial3 transmis>");delay(4000);
       }
    }
    else {Serial.println("<M_Serial3 non disponible>");
      }//test
  
     //transmission donnée via bluetooth
    if(modeCom==1 && Serial2.available()){
       if (test) {Serial2.print("*n" +String("Tablette active : ")+"*");}
       //Tant qu'il y a bytes dans le buffer serie2 lecture et Mode de communication bluetooth traitement
      
       while ((Serial2.available()>0)&& modeCom==1){
          bluetoothData = Serial2.read();  //read Serial
          delay(10);        
          
          if (test) {Serial2.print("*n" +String("AVfonctbluerecep")); Serial2.print(bluetoothData);}
          bluetoothReception();
          if (test) {Serial2.print("*n" +String("Code test:A/R commande:")); Serial2.print(bluetoothData);Serial2.print("*");}  //imprimer sur tablette ce qui viens d'être lu - Test
           
           delay(20);//chnge pour test - ini 100
          } //end While
          
      }
      
    
  //test si donnée disponible depuis rpi serie com et traitement
     if(modeCom==2 && Serial.available()){
       //Tant qu'il y a bytes dans le buffer serie lecture et netoyage du message détecte '$' comme début message
        //serialRpiData=;
        if (testRpi){Serial.println("<M_valArduc: "+String(serialRpiData)+">");delay(2000);}
        newRpiData == false; // initialisation de newRpiData IMP
        recvRpiWithStartEndMarkers();//passe newRpiData à True  ce qui permet d'entrer ds fonction traitement
        //String commentMM=String((char*)serialRpiData);
        if (testRpi){Serial.println("<M_valArduc: "+String(serialRpiData)+">");}//envoi de la dernière instruction reçu par arduino
        //if (testRpi){Serial.println("<M_valArduzaz>");}//envoi de la dernière instruction reçu par arduino
        delay(30);       
        rpiTraitement();
        delay(30);//chnge pour test - ini 100
        newRpiData = false;//IMP libère serialRpiData 
         } //end du if
 
   //Définir le mode de fonctionnement - switchcase
     choixModeRz ();

   // Affichage des résultats des encodeurs
   //Ajouter conditionnel en fonction du mode de fonctionnement ne déclencher que si moteurs actifs - desactivé pour test
   //cas d'un déplacement avec asservissement (mode déplacement automatique après def. de l'objectif (et calibration des constantes!)

  /*************lancement conditionnel de l'odometrie A FAIRE ajouter les conditions du passage de la var odometrie à 1*********/
  if(odometrie==1){
    long newLeft, newRight;
    newLeft = knobLeft.read();
    newRight = knobRight.read();
    //Test Odométrique inutile lancé direct à reception message ainsi que l'iniasservissement et le premier asservissement
    //if(oldOdometrie==0 && odometrie==1){oldOdometrie=odometrie;iniAsservissement();Serial.print("<Modométrie:"+String(odometrie)+">");}//odometrie pas d'actu de position avant transfert pt ini
    if (newLeft != comptG || newRight != comptD ) { //actualise nbre de ping gauche et droit
        comptG = newLeft;
        comptD = newRight;
        //Serial.println ("<ModocomptG:"+String(comptG)+" comptD:"+String(comptD)+">");delay(20);
        asservissement();
        }
    }//fin odometrie
    //if (testEncodeur==1){Serial.print("Résultats encodeurs compt G : "); Serial.print(comptG); Serial.print("   compt D : ");Serial.println(comptD);}
   
   // Gestion loop sonar avec condition condition d'entrer pour limiter échange ds test peut être desactivé provisoirement
   if (modeRzB != ARRET){
      if (modeCom==1){fSonarBlue();} //envoie des infos à androïd via bluetooth
      if (modeCom==2){fSonarRpi ();} // envoi par port serie /USB à RPI puis internet
    }
 
  /******************Gestion boucle liquid cristal *******************
  Envoi des messages reçus sur receptionMessage utiliser en statique du fait de Delay*/
   if(receptionMessage!=""){// test si message reçu via bluetooth avec #
      //if(testRpi){Serial.print("<MmessageLCDrecep>");}//permet de verifier si info parvient ici
      
      lcd.clear();
      byte lcdEtat = 1;
      lcd.backlight();
      int lgReceptionMessage=sizeof(receptionMessage);
      if(lgReceptionMessage<16){lcd.print(receptionMessage); delay(3000);lcd.clear();lcd.noBacklight();receptionMessage="";} 
      else{
         lcd.setCursor(0,0);
         for (int i = 0; i < 16 ; ++i){lcd.print(receptionMessage[i]);}
         lcd.setCursor(1,0);
         delay(1000);
         for (int i = 16; i < lgReceptionMessage && i<32 ; ++i){lcd.print(receptionMessage[i]);}
     }
   delay(3000);
         lcd.clear();
  }

  /**********activation ventilo V 2022 inutile directe RPI*****/
  //if(ventiloOn ==1){digitalWrite(ventiloRpiPin , HIGH );
  //} 
  //else {digitalWrite(ventiloRpiPin , LOW );
  //}


  /*******I2C   utilisé pour LCD /mega ok v 2022 janv pas de liaison fonctionnelle avec RPI /cde I2C detect*******/
  Wire.onReceive(receiveData);//emission I2C
  delay(20);// ajouter après ini 50
  Wire.onRequest(sendData);
  delay(20);//ajouter après
       
   
/*********Loop des capteurs avec transmis info. à tablette*********/
 
 /*****Loop de emotion color***********/
  // coupe Gyro /Emotion si nécessaire
  if ((millis()>boutonTpsFin && boutonTpsFin!=0) || boutonControl==0){ //condition coupe luz de boule
    boutonControl=0;   
    digitalWrite(emotionBluePin, LOW);blueMValue = 0;
    digitalWrite(emotionGreenPin, LOW);greenMValue = 0;
    digitalWrite(emotionRedPin, LOW);redMValue = 0;
    nbBoucleLoop=0;
    }
  else if (frequenceGyro>0){
    Serial.println ("<Mfreqgyrosup>"); 
    if (nbBoucleLoop<10){nbBoucleLoop=nbBoucleLoop +1;} else {nbBoucleLoop=0;}//compteur de boucle
    if (frequenceGyro==nbBoucleLoop){chgeEmotion();}
    else{lawEmotion();}//gyro eteind
    }

 /*****Loop de luz  neopixel - sortie - color/ring via Z depuis node-red à implémenter***********/
  // coupe ring si nécessaire
  if ((millis()>ringTpsFin && ringTpsFin != 0) ||activeRing==0){
    ringPixels.begin();
    ringPixels.clear();
    ringPixels.show();
    digitalWrite(ringLedPin, LOW);
    };

    // coupe ruban si nécessaire
  if ((millis()>rubanTpsFin && rubanTpsFin != 0) ||activeRubanLed==0){
    rubanPixels.begin();
    rubanPixels.clear();
    rubanPixels.show();
    digitalWrite(rubanLedPin, LOW);
    };

 /**************Loop de l'active buzzer sur RZ sortie - voir variante v10 ou avant***********/
  if(activeBuzzer && (buzzerTpsFin ==0)){claxon();}//ctre intuitif buzzer TpsFin=0 fait buzzer ss fin. Buzzer de RZ activer par activeBuzzer = true  Tone needs 2 arguments, but can take three
  else if((millis()>buzzerTpsFin) || activeBuzzer==0){
    activeBuzzerPin,LOW;
    activeBuzzer=0; 
    buzzerTpsFin==0;
    }//arrête claxon et ttes fonction utilisant buzzer
 
/*****************Loop du micro AV. - entrée ***************/
  if(activeMicro) {microTraitement();} //si actifrenvoi intensité en % sur moCom et 1 ou 2 via 'h'
   
/*****************Loop detecteur mvt ***************/
//tester activation qui dépend du mode de fonctionnement et actualisation spécifique si com tablette - //changer en fonction appelé si mode surveillance
  
  if(detecteurMvt==true && mvtIrVal!= digitalRead(mvtIrPin)){ //detect mvt activé et modif valeur
   // mvtIrVal== digitalRead(mvtIrPin);// stockage resultat mouvement ou non mvtIrVal = true ou false évite chgment serie inutile
    if (modeCom==1){
          if (mvtIrVal==0) {Serial2.print("*pR255G255B240*");digitalWrite(activeBuzzerPin,LOW);Serial2.print("*jV50*");}
          else {digitalWrite(activeBuzzerPin,HIGH);}
    }
    if (modeCom==2){
           if (mvtIrVal==0) {Serial.print("<p>");}//p min
          else {Serial.print("<P>");}//Pmaj
    }
  }
delay(30);
/*****loop des detecteurs de luz********************/
  if(photoRactive && photoSmartActive==0) {//reaction à detect luz arduino actif et luz smart inactif
   valIntLuzPhotoR= 1/10*analogRead(pinPhotoR); //resultat en % d'intensité luz 1024/100 arrondi
   if(oldValIntLuzPhotoR != valIntLuzPhotoR){
    if(modeCom==1){Serial2.print ("*d"+String(valIntLuzPhotoR)+"*");}
    if(modeCom==2){Serial.print ("<D"+String(valIntLuzPhotoR)+">");}// transmission des val du detecteur arduino
   }
  }
  if(photoSmartActive) {//reaction à detect luz Smart actif 
   if(oldValIntLuzRSmart != intLuzRSmart){Serial2.print ("*d"+String(intLuzRSmart)+"*");}// a verif
     }
/*Serial.print("<Mmessage apostrophe de arduino>");delay(50);
Serial.print("<M1>");delay(50);
Serial.print("<W>");delay(3000);*/
}// end loop


/********************** Les fonctions associées *********************************************/

// fonctions liées au liquid cristal
void receiveData(int byteCount){
    while(Wire.available()) {
        dataLcdReceived = Wire.read();
        Serial.print("Donnee recue : ");
        Serial.println(dataLcdReceived);
    }
}

void sendData(){
    int envoi = dataLcdReceived + 1;
    Wire.write(envoi);
}

   /******Entrer un texte défilant dans lCD*******
    une info. va être affichée la durée de base (sup à la durée test info disponible pour LCD
    unsigned long  lcdDureeMini = 3000; // temps d'affichage minimum influe lcdEtat
    unsigned long lcdTempoTimer = 2000;//lcd durée Cycle avant lecture de donnée ajouter condition dispo
    unsigned long lcdTimer; // conserve le temps depuis le dernier ping / millis
    byte lcdEtat = 1;//1 quand utilisable 0*/
void lcdEcrire(char* messageligne1,char* messageligne2) {  //L'étoile lui explique que tableau de char attenducontrôle . Rétroéclairage, affichage et blocage, libère lcd
  if (lcdEtat==1){//disponible
    lcdTimer = millis() + lcdDureeMini;  //temps avant LCD disponible
    lcd.cursor_on();
    lcd.blink_on();
    lcd.backlight();//allume le rétroéclairage
    //écrire le message
   if (testLcd) {Serial.println ("je suis dans boucle écriture LCD");};
    lcd.setCursor(0,0);
    lcd.print(messageligne1);
    delay(100);
    lcd.setCursor(0,1);
    lcd.print(messageligne2);
    delay(100);
    // option boucle affichage passant avec scrolDisplayLeft()
    lcdEtat=0;//indique au programme qu'un message est en cours d'affichage
  }//end if
  else if ( (lcdTimer <=millis())&& lcdEtat==0){//arrêter affichage pour entrerr dans affichage remettre lcdEtat à 1
    //Serial.print ("Je sort du blocage de l'affichage lcd");
    lcd.clear();//efface & remet au début (lcd.home que la seconde partie)
    lcd.noBacklight();//éteind le rétroéclairage - économise batterie
      }//end else if
  //À METTRE DS TEST
  else {
    Serial.print ("AV-Pb non prévu fonction lcdEcrire: ");
    Serial.print (lcdEtat);
    Serial.print(" timer : ");   
    Serial.print (lcdTimer);
    Serial.print ("temps actuel");
    Serial.print (millis()); 
  }//end else
  
}//end lcdEcrire

/*******************************Reception bluetooth************/
void bluetoothReception() {
  // Base lecture de tous les renvois de la tablette récupérer le prochain caractére depuis bluetooth
 //ARRET = 0 , VEILLE = 1 , SURVEILLANCE = 2 , AUTONOME = 3 , SUIVEUR = 4 , TELECOMMANDE = 5 
   Serial.print ("ds fonction blutooth reception - commande reçu :");
   delay(10);
   if (bluetoothData=='s'){modeCom=0;modeRzB=0;Serial2.print ("*m"+String("Arret")+"*");traitementArret=false;iniTablette();}  // le cas S traité en amont de la fonction A REVOIR DESACTIVE ICI EN mettant s à >1
   if (bluetoothData=='S'){modeCom=1;modeRzB=1;Serial2.print ("*m"+String("veille")+"*");}  // le cas S traité en amont de la fonction A REVOIR DESACTIVE ICI EN mettant s à >1

   if (bluetoothData=='N'){
        if (modeRzB < 5) {modeRzB = modeRzB+1;} else {modeRzB = 0;}//retour au début de la liste des modes
        if (modeRzB == 0)      { Serial2.print ("*m"+String("Arret")+"*");}
        else if (modeRzB == 1) { Serial2.print ("*m"+String("veille")+"*");}
        else if (modeRzB == 2) { Serial2.print ("*m"+String("surv passive")+"*");}
        else if (modeRzB == 3) { Serial2.print ("*m"+String("surv active")+"*");}
        else if (modeRzB == 4) { Serial2.print ("*m"+String("mode suiveur")+"*");}
        else if (modeRzB == 5) { Serial2.print ("*m"+String("telecommande")+"*");}
        delay(20);
        }//end if N

//aenlever if (bluetoothData=='U'){Serial.print("uuu");};

   //pilotage manuel des servomoteurs que pour tete
      //Tete
    if(bluetoothData == 'T'){//prise de contrôle manuel de la tête ou libération / interupteur
      
      contrManuelServoTGD = !contrManuelServoTGD;// actualisation variable controle manuelle
        if (contrManuelServoTGD){
          //int inverse=180-posServoTGDR;
          Serial2.print("*a"+String(posServoTGDR)+"*");//afficher position servo-moteur Tete   
          Serial2.print("*tR0G255B0*");//actualisation du voyant quand controle manuelle
        } 
        else {Serial2.print("*tR0G0B0*");}//afficher voyant noir si pas de controle manuel
       delay(5);
      }
    
    if(bluetoothData == 'B' && contrManuelServoTGD){//tete vers gauche - inversion moteur cadran 0 à droite 180 à gauche
      if(posServoTGDR>=3){posServoTGDR = posServoTGDR-2  ; servoTGD.attach(4);servoTGD.write(180-posServoTGDR);Serial2.print("*a"+String(posServoTGDR)+"*");}//attention doit s'arrêter à 1 sinon bug /0 
      }
    if(bluetoothData == 'D' && contrManuelServoTGD){//tete vers droite
        if(posServoTGDR<=177){posServoTGDR = posServoTGDR+2 ; servoTGD.attach(4);servoTGD.write(180-posServoTGDR);Serial2.print("*a"+String(posServoTGDR)+"*");}//attention doit s'arrêter à 1 sinon bug /0 
       }

   //Moteur - si modif d'une variable appeler fonction moteur - mettre le cas le plus probable en premier
    if(bluetoothData == '0'||bluetoothData == '1' || bluetoothData =='2' || bluetoothData =='3' || bluetoothData =='4'||bluetoothData =='5'||bluetoothData =='6'||bluetoothData =='7'||bluetoothData =='8' ){//valeur de 0 à 9  rappel constructeur :'<','A','0', ',','0', ',','0','>'
    transMoteurP[5]=directionMoteur=bluetoothData;
    }
    
    if(bluetoothData == 'V') {
    delay(20);
    transMoteurP[3]= Serial2.read(); //boiteVitesse valeur reçu suivante
    bluetoothData="";
    delay(20);//ini100
    Serial2.print (":"+String(transMoteurP)+":"); //? zz plante ????
     }
     
     if(bluetoothData == 'A'){
     delay(20);
     transMoteurP[1]= Serial2.read();//récup. de la valeur d'acceleration entre 1 et 4max
    delay(20);
     Serial2.print(":"+String(transMoteurP)+":");
     }
  
   //Détecteur de mouvement pilotage manuel    
     if(bluetoothData == 'P') {
              detecteurMvt = !detecteurMvt;
              if(detecteurMvt){
                if (mvtIrVal==1){Serial2.print("*pR0G255B0*");}
                else{Serial2.print("*pR255G255B240*");}
                 }
              else{Serial2.print("*pR0G0B0*");}//Pmaj et pmin si activé modif du voyant selon si mvt ou non vert mvt blanc activé ss mvt noir pas activé
     }
 
   //Détecteur big Sound Avant pilotage manuel - var intSon donne l'intensité pour la barre elle est traité dans le loop
    if(bluetoothData == 'Q') {
              activeMicro = !activeMicro; //activation micro AV
              if(activeMicro){Serial2.print("*qR0G255B0*");}//témoin d'activation du micro av
              else{Serial2.print("*qR0G0B0*");}
    }
    /*if(bluetoothData == 'I') {//desactivation photodiode sur arduino
              photoRactive = !photoRactive; //photodiode desactivée
              if(photoRactive){Serial2.print("*iR0G255B0*");}//témoin d'activation du micro av
              else{Serial2.print("*iR0G0B0*");}
    } */
 
   //pilotage manuel des émotions traitement dans le loop pour la suite
     if(bluetoothData == 'K') {boutonControl = !boutonControl;//si boutonControl==1 traité dans le loop
        if (boutonControl==0){Serial2.print("*kR0G0B0*");boutonVert = false; boutonJaune = false; boutonRouge = false;delay(5);}
        chgeEmotion();
     }
     if(bluetoothData == 'G' && boutonControl){boutonVert = true; boutonJaune = false; boutonRouge = false;delay(5);chgeEmotion();}
     if(bluetoothData == 'J' && boutonControl){boutonVert = false; boutonJaune = true; boutonRouge = false;delay(5);chgeEmotion();}    
     if(bluetoothData == 'R' && boutonControl){boutonVert = false; boutonJaune = false; boutonRouge = true;delay(5);chgeEmotion();}

  
   /**Pseudo Morse avec led  du corps depuis tablette - ZZ led verte non reinstalle*********/
     if(bluetoothData == 'F') {activeRing = !activeRing;if(activeRing){Serial2.print("*fR00G200B00*");delay(5);}else{Serial2.print("*fR00G00B00*");};} //bascule activation, désactivation de la lumière et du voyant de contrôle associé
     
   //activation bascule des cameras depuis tablette et actualisation voyants
      if(bluetoothData == 'C') {//camera
        cameraHD = !cameraHD;
        if(cameraHD){
          Serial2.print("*cR0G200B0*");delay(5);
          } 
        else {Serial2.print("*cR0G0B0*");
          } //voyant camera
        
        } //traitement dans envoi de mega 

   // Son - claxon depuis tablette sur buzzer Rz
      if(bluetoothData == 'E') { activeBuzzer= !activeBuzzer;if(activeBuzzer){Serial2.print("*eR00G200B00*");}else{Serial2.print("*eR00G00B00*");}delay(5);}//buzzer claxon bascule

    //Reception Par "Send" permet d'envoyer de multiples messages début # et fin #
      if(bluetoothData == '#'){
      boolean receptionSend=1;
      char transit;
      while ((Serial2.available()>0)&& modeCom==1 && receptionSend){
        transit =  Serial2.read();
        if (transit != '#'){receptionMessage+=transit;delay(10);}  //reception du message delay?
        
        delay(10);        
        if (test) {Serial2.print("*n" +String(receptionMessage)); Serial2.print("*");Serial.print(receptionMessage);}  //imprimer sur tablette ce qui viens d'être lu - Test
        delay(10);//chnge pour test - ini 100
         } //end While
      }//end du cas #
}//end reception


/******************************* RpiReception************/
void rpiTraitement() {// données récupérée nettoyer par recRpiWithStartEndMarkers()
char directionMoteur='0';
 if (newRpiData == true) {  //voir si on peut arrêter avec newRpiData à false ds chaque cas traité ou alors faire else if
   if (testPosTete){Serial.print("servoangle : ");Serial.print(degreTGD);Serial.print(" commence: ");Serial.print(String(serialRpiData[0]));};
  
   if(serialRpiData[0]=='S'){modeCom=2; modeRzB=1;} //- rpi- AFaire si modeCom=2 désactiver bluetooth ?  
   else if(serialRpiData[0]=='s'){modeRzB=0;modeCom=0;}
  
          //change mode
   else if(serialRpiData[0]== 'N'){modeRzB = serialRpiData[1]-48;Serial.println("<M mode Atualisé>");} //convert ascii en num -48 /code  ok - généré aussi par joystick bouton central
       
         //gestion position tête
   else if(serialRpiData[0]== 'T'){//servos
      if(serialRpiData[1]== 't'){//cas servoTGD 
          contrManuelServoTGD = true;
            //Serial.print ("<MposServoTGDdeb: "+String(posServoTGD)+">");
            //delay(2000);
            //Serial.print ("<MposServoTGDR deb: "+String(posServoTGDR)+">");
            //delay(2000);   
          //servoTGD.attach(4);// zz utile ?
          //reconstruction de l'angle posServoTGD avec modif pr inversion montage négatif en positif
          if(serialRpiData[2]== '-'){//inni1 - conversion decimal et unité des caractères ascii ds lignes suivantes
             if(lgRpiData==6){posServoTGD= 100*(serialRpiData[3]-48)+10*(serialRpiData[4]-48)+(serialRpiData[5]-48);}
             else if (lgRpiData==5){posServoTGD= 10*(serialRpiData[3]-48)+(serialRpiData[4]-48);}//nega 2 chiffres on rend positif pour inversion montage
             else{posServoTGD= 1*(serialRpiData[3])-48;}//nega 1 chiffre - idem on rend positif pour inversion
           }
          else{
             if (lgRpiData==5){posServoTGD= -100*(serialRpiData[2]-48)-10*(serialRpiData[3]-48)-(serialRpiData[4]-48);}
             else if (lgRpiData==4){posServoTGD= -10*(serialRpiData[2]-48)-(serialRpiData[3]-48);}//positif 2 chiffres on rend négatif pour tenir compte inversion
             else{posServoTGD= -1*(serialRpiData[2]-48);}//positif 1 chiffre+ donc decalage lecture         
           }
         
         int posServoTGDDsNode= -posServoTGD ;//après cette inversion position de signe pour posServoTGD (pour tenir compte du montage) 
             posServoTGDR=posServoTGD+correctionPosServoTGD;
         //mvt progressif servo
         int deltaST=posServoTGDR-oldPosServoTGDR;
         int i;
         int pas = 3;//
         int dure = 10;// latence entre pas
         
         //if(deltaST>=0){for(i==0;i<=deltaST;i=i+pas){servoTGD.write(i);delay(dure);}}
         //else{for(i==0;i>=deltaST;i=i-pas){servoTGD.write(i);delay(dure);}}
         //servoTGD.write(posServoTGDR);delay(dure);  //ajustement pour compléter la boucle indépendamment du pas
          servoTGD.attach(4);
      
           //cas vers gauche
            for(oldPosServoTGDR;oldPosServoTGDR>=posServoTGDR;oldPosServoTGDR=oldPosServoTGDR-3){servoTGD.write(oldPosServoTGDR);delay(50);}//déplacement lent depuis l'ancienne valeurs
         //cas vers droite  (effet de bord écart d'un degré potentiellement corrigé par ligne suivant boucle for) 
            for(oldPosServoTGDR;oldPosServoTGDR<=posServoTGDR;oldPosServoTGDR=oldPosServoTGDR+3){servoTGD.write(oldPosServoTGDR);delay(50);}//déplacement lent depuis l'ancienne valeurs
            
            //servoTGD.write(posServoTGDR);
            //delay(2000);
            servoTGD.detach();
            //posServoTGD=posServoTGDR=oldPosServoTGDR;//utile?
            Serial.println ("<M-posServoTGDActu: "+String(posServoTGDDsNode)+">");
            delay(2000);
            Serial.println ("<M-posServoTGDRActu: "+String(posServoTGDR)+">");
            delay(2000);
           oldPosServoTGDR=posServoTGDR;//conservation de l'angle moteur actuel comme ref(permet d'actualiser si initServo suite à arrêt
         }//fin de T
      
      if(serialRpiData[1]== 'r'){//cas servoTronc = ServoBase - determiner les limites, potentiellement 0 à 270?, pas de val négative pour ce servo
             if(lgRpiData==5){posServoBase= +100*(serialRpiData[2]-48)+10*(serialRpiData[3]-48)+(serialRpiData[4]-48);}
             else if (lgRpiData==4){posServoBase= +10*(serialRpiData[2]-48)+(serialRpiData[3]-48);}//positif 2 chiffres 
             else{posServoBase= +1*(serialRpiData[2]-48);}
             servoBase.attach(9);
             //cas vers gauche
            for(oldPosServoBaseR;oldPosServoBaseR>=posServoBase;oldPosServoBaseR--){servoBase.write(oldPosServoBaseR);if(test){Serial.println("nvelle angle :"+String(oldPosServoBaseR));}delay(50);}//déplacement lent depuis l'ancienne valeurs
         //cas vers droite  (effet de bord écart d'un degré potentiellement corrigé par ligne suivant boucle for) 
            for(oldPosServoBaseR;oldPosServoBaseR<=posServoBase;oldPosServoBaseR++){servoBase.write(oldPosServoBaseR);if(test){Serial.println("nvelle angle :"+String(oldPosServoBaseR));}delay(50);}//déplacement lent depuis l'ancienne valeurs
             servoBase.write(posServoBase);
             delay(2000);
             servoTHB.detach();
             Serial.println ("<MposServoBase"+String(posServoBase)+" baseold:"+String(oldPosServoBaseR)+">");
             oldPosServoBaseR=posServoBase;//utile?
              
      }//fin de r (servoBase)
      
       if(serialRpiData[1]== 'c'){//cas servoTcou - servo THB (Tête Haut Bas)
             if(lgRpiData==5){posServoTHB= +100*(serialRpiData[2]-48)+10*(serialRpiData[3]-48)+(serialRpiData[4]-48);}
             else if (lgRpiData==4){posServoTHB= +10*(serialRpiData[2]-48)+(serialRpiData[3]-48);}//positif 2 chiffres on rend négatif pour tenir compte inversion
             else{posServoTHB= +1*(serialRpiData[2]-48);}
            // //cas vers gauche
            servoTHB.attach(5); 
            for(oldPosServoTHBR;oldPosServoTHBR>=posServoTHB;oldPosServoTHBR=oldPosServoTHBR-3){servoTHB.write(oldPosServoTHBR);if(test){Serial.println("nvelle angle :"+String(oldPosServoTHBR));}delay(50);}//déplacement lent depuis l'ancienne valeurs
            //cas vers droite  (effet de bord écart d'un degré potentiellement corrigé par ligne suivant boucle for) 
            for(oldPosServoTHBR;oldPosServoTHBR<=posServoTHB;oldPosServoTHBR=oldPosServoTHBR+3){servoTHB.write(oldPosServoTHBR);if(test){Serial.println("nvelle angle :"+String(oldPosServoTHBR));}delay(50);}//déplacement lent depuis l'ancienne valeurs
            //min 20 quelques soit position équilibre qd base au repos 45 qd vertical 2O, qd base horizontal 70? .ss ressort
             servoTHB.write(posServoTHB);
             delay(2000);
             servoTHB.detach();
       
            Serial.println ("<MoldPosServoTHBR"+String(oldPosServoTHBR)+" THB:"+String(posServoTHB)+">");          
            oldPosServoTHBR=posServoTHB;//utile?
      } //fin de c (servoTHB tete Haut Bas)    
     }//fin servo
      //Moteur    
  
    //else if(serialRpiData[0]== ( '1'||'2'||'3'|| '4'|| '5'|| '6'|| '7'|| '8')){transMoteurP[5]=serialRpiData[0];}
    else if(serialRpiData[0]== 'W'){transMoteurP[5]= serialRpiData[1];}//direction sens aiguille montre 1 devant
    else if(serialRpiData[0]== 'V'){transMoteurP[3]= serialRpiData[1];}//vitesse max 9
    else if(serialRpiData[0]== 'A'){transMoteurP[1]= serialRpiData[1];}//Acceleration max4
  
          //activ detect mvt
    else if(serialRpiData[0]== 'P'){detecteurMvt = true;if(testRpi){Serial.println("<Mdetect mvt>");}} //active detecteur de mvtattention avertir ailleurs via mvtIrVal==1 si mvt 
    else if(serialRpiData[0]== 'p'){detecteurMvt = false;}
  
          //activ Micro choix du micro et activation
    else if(serialRpiData[0]== 'Q'){choixMicro=(serialRpiData[1]-48);activeMicro = true;}// Choix du micro si 1 alors old microAv new micros si 2 microSmart
    else if(serialRpiData[0]== 'q'){if((serialRpiData[1]-48)==2 && choixMicro==2){ activeMicro = false;}//1 seul actif à la fois et qd on en coupe 1, il faut relancer l'autre ?
                                    else if((serialRpiData[1]-48)==1 && choixMicro==1){ activeMicro = false;}
                                   }//fin de q
   
           //activ detecteur deluz arduino
    //else if(serialRpiData[0]== 'I'){photoRactive = true;if(testRpi){Serial.println("<Mvaldetecteur de luz Ardu>");}delay(20) ;}
    //else if(serialRpiData[0]== 'i'){photoRactive = false;}
                              
           //activ detecteur deluz Smart
    else if(serialRpiData[0]== 'L'){photoSmartActive = true;if(testRpi){Serial.println("<Mphotoactive>");}delay(20);}
    else if(serialRpiData[0]== 'l'){photoSmartActive = false;}
  
  
  
            //activ Emotion luz RGB - Gyro
    else if(serialRpiData[0]== 'g'){boutonControl=false;chgeEmotion();}// a verif
    else if(serialRpiData[0]== 'G'){
      boutonControl=true;  // un des boutons Gyro activé 
        if(serialRpiData[1]==      'G'){ boutonControl=true; boutonVert = true; boutonJaune = false; boutonRouge = false;delay(5);chgeEmotion();}//bouton vert
        else if(serialRpiData[1]== 'J'){boutonControl=true; boutonVert = false; boutonJaune = true; boutonRouge = false;delay(5);chgeEmotion();}//jaune
        else if(serialRpiData[1]== 'R'){boutonControl=true;boutonVert = false; boutonJaune = false; boutonRouge = true;delay(5);chgeEmotion();}//Rouge
        else if(serialRpiData[1]== 'F'){
                String chercheFrequence =serialRpiData;//X en 3 val commence à 4
                chercheFrequence=chercheFrequence.substring (3);//string.substring(val,to) val inclus 'to' exclus
                frequenceGyro=chercheFrequence.toInt();//0 gyro continue frequence 10 max
                //frequenceGyro=serialRpiData[3]-'0'; // ds char les nombre se suivent en soustrayant '0' on obtient un int
                     Serial.println ("<M-fGyro: "+String(frequenceGyro)+">");
                     delay(10);}
     }
       
           //Led Ring Fonction communication comportement - a développer inclus  Led ring
    else if(serialRpiData[0]==  'F'){
      if(serialRpiData[1]==       'A'){alerte();Serial.println ("<M-ALERTEV>");}// fonction Alerte
      else if(serialRpiData[1]==  'R'){}//Rien affichage par defaut
      else if(serialRpiData[1]==  'O'){oui();}//fonction Oui
      else if(serialRpiData[1]==  'N'){non();}//fonction Non
      else if(serialRpiData[1]==  'P'){temoinLuz();}//fonction - temoin d'activite à programmer - a associer à code MPB ?- zz mai ds rpi detect mvt pb
      else if(serialRpiData[1]==  'B'){bonjour();}//fonction Bonjour
      else if(serialRpiData[1]==  'S'){sourire();}//fonction Sourire
      else if(serialRpiData[1]==  'T'){triste();Serial.println ("<M-F-Triste>");}//fonction Triste
      else if(serialRpiData[1]==  'X'){Serial.println ("<M-Fction Fin>");}
      else {Serial.println("<MPbComFction>");}
      }
//    else if(serialRpiData[0]==  'f'){}// Indique fin de fonction, mettre à zero les var de durée associer aux fonctions de comportement
  
           //activ Camera zz non utilise
    else if(serialRpiData[0]==  'C'){cameraHD = true;}
    else if(serialRpiData[0]==  'c'){cameraHD = false;}
    
           //activ Buzzer - A faire ajouter mélodie et fonctions Attention les buzzers peuvent se "coincer" qd pb ne pas changer cable immédiatement
    else if(serialRpiData[0]==  'B'){activeBuzzer = true; if(testRpi){Serial.println("<MBuzzer>");}
            if(serialRpiData[1]==  'F'){
                     String chercheFrequence =serialRpiData;//X en 3 val commence à 4
                     chercheFrequence=chercheFrequence.substring (2);//string.substring(val,to) val inclus 'to' exclus
                     frequenceBuzzer=chercheFrequence.toInt();
                     Serial.println ("<M-fBuzzer: "+String(frequenceBuzzer)+">");//0 defaut max frequence 10 
                     delay(2000);
                     } //BF active Buzzer et var frequenceBuzzer specifique != de 0
             
             if(serialRpiData[1]==  'I'){
                     String chercheIntensite =serialRpiData;//X en 3 val commence à 4
                     chercheIntensite=chercheIntensite.substring (2);//string.substring(val,to) val inclus 'to' exclus
                     intensiteBuzzer=chercheIntensite.toInt()*10;//varie de 0 à 250 val transmise 0 à 25
                     Serial.println ("<M-iBuzzer: "+String(intensiteBuzzer)+">");//0 defaut max frequence 10 
                     delay(2000);
                     } //BF active Buzzer et var intensiteBuzzer specifique != de 0
             }
    else if(serialRpiData[0]==  'b'){activeBuzzer = false;}
  
     //Reception de messages pour LCD 
    else if(serialRpiData[0]==  'M'){
         serialRpiData[0]=' ';
         receptionMessage=serialRpiData;//transmission à boucle pour traitement
         if(testRpi){Serial.print("<MmessageLCDenvoyé>");}
         delay(10);//delay ini10
         }
         
     //Ring de led Reception    
   else if(serialRpiData[0]==  'R'){
            activeRing = true; ringTpsFin=0; if(testRpi){Serial.print("<MringLed>");}
            if(serialRpiData[1]==  'F'){
                     String chercheFrequence =serialRpiData;//X en 3 val commence à 4
                     chercheFrequence=chercheFrequence.substring (2);//string.substring(val,to) val inclus 'to' exclus
                     frequenceRing=chercheFrequence.toInt();
                     Serial.println ("<M-fRing: "+String(frequenceRing)+">");//0 defaut max frequence 10 
                     delay(2000);
                     } 
             
            else if(serialRpiData[1]==  'I'){
                     String chercheIntensite =serialRpiData;
                     chercheIntensite=chercheIntensite.substring (2);//string.substring(val,to) val inclus 'to' exclus
                     intensiteRing=chercheIntensite.toInt()*10;//varie de 0 à 250 val transmise 0 à 25
                     Serial.println ("<M-iRing: "+String(intensiteRing)+">");//0 defaut max frequence 10 
                     delay(2000);
                     } 
            else if(serialRpiData[1]==  'C'){
                    String chercheCouleur =serialRpiData;
                    chercheCouleur=chercheCouleur.substring(6);//2+4 RCrgb(
                     int firstvirguleindex=chercheCouleur.indexOf(',');//index position première virgule
                     int secondvirguleindex=chercheCouleur.indexOf(',', firstvirguleindex+1);
                     String redS=chercheCouleur.substring(0,firstvirguleindex);
                     String greenS=chercheCouleur.substring(firstvirguleindex+1,secondvirguleindex);
                     String blueS=chercheCouleur.substring(secondvirguleindex+1);
                     redRC=redS.toInt();
                     greenRC=greenS.toInt();
                     blueRC=blueS.toInt();
                     //couleurRing=chercheCouleur.toInt();
                     Serial.println ("<M-Cring: G "+greenS+"R"+redS+"B"+blueS +">");//0 defaut max frequence 10 
                     } 
                     ringColor();
             }
    else if(serialRpiData[0]==  'r'){activeRing = false;}

      
  //Ruban LED RECEPTION
       //Ring de led Reception  - zz    
   else if(serialRpiData[0]==  'D'){
            activeRubanLed = true; rubanTpsFin=0; if(testRpi){Serial.print("<MrubanLed>");}
            if(serialRpiData[1]==  'F'){
                     String chercheFrequence =serialRpiData;//X en 3 val commence à 4
                     chercheFrequence=chercheFrequence.substring (2);//string.substring(val,to) val inclus 'to' exclus
                     frequenceRubanLed=chercheFrequence.toInt();
                     Serial.println ("<M-fRubLed: "+String(frequenceRubanLed)+">");//0 defaut max frequence 10 
                     delay(2000);//????
                     ringColor();
                     } 
             
            else if(serialRpiData[1]==  'I'){
                     String chercheIntensite =serialRpiData;
                     chercheIntensite=chercheIntensite.substring (2);//string.substring(val,to) val inclus 'to' exclus
                     intensiteRubanLed=chercheIntensite.toInt()*10;//varie de 0 à 250 val transmise 0 à 25
                     Serial.println ("<M-iRubLed: "+String(intensiteRubanLed)+">");//0 defaut max frequence 10 
                     delay(2000);
                     ringColor();
                     } 
            else if(serialRpiData[1]==  'C'){
                    String chercheCouleur =serialRpiData;
                    chercheCouleur=chercheCouleur.substring(6);//2+4 RCrgb(
                     int firstvirguleindex=chercheCouleur.indexOf(',');//index position première virgule
                     int secondvirguleindex=chercheCouleur.indexOf(',', firstvirguleindex+1);
                     String redS=chercheCouleur.substring(0,firstvirguleindex);
                     String greenS=chercheCouleur.substring(firstvirguleindex+1,secondvirguleindex);
                     String blueS=chercheCouleur.substring(secondvirguleindex+1);
                     redRB=redS.toInt();//RB pour ruban
                     greenRB=greenS.toInt();
                     blueRB=blueS.toInt();
                     //couleurRing=chercheCouleur.toInt();
                     Serial.println ("<M-Cruban: G "+greenS+"R"+redS+"B"+blueS +">");//0 defaut max frequence 10 
                     ringColor();
                     }
    // activation de complement ruban led à programme existant                  
            else if(serialRpiData[1]==  'P'){rubanFuite=true;}//fuite qd mvt se rapproche de US specifique
            else if(serialRpiData[1]==  'S'){rubanSuivre=true;}
            else if(serialRpiData[1]==  'T'){rubanTourne=true;}// clignotant fonction mvt direction moteur
            else if(serialRpiData[1]==  'E'){rubanEmotion=true;} // active dans une fonction emotion e ring une animation specifique du ruban 
                       }//fin cas ou ruban actif
    else if(serialRpiData[0]==  'd'){activeRubanLed = false;}

       
      //Odometrie & autonomie   
    else if(serialRpiData[0]==  'K'){
            if(serialRpiData[1]==  'O') {Serial.println("<Modométrie&ini>");}//{odometrie=1;iniAsservissement();}//odometrie active ET INITIALISATION - Serial.println("<Modométrie&ini>")
            else if (serialRpiData[1]==  'R') {
                      if(serialRpiData[2]==  'E') {
                        rzIni=serialRpiData;//X en 3 val commence à 4
                        //extraction des valeurs X etY
                        String interx=rzIni.substring (4, rzIni.indexOf('Y'));//string.substring(val,to) val inclus 'to' exclus
                        xR=interx.toDouble();//distance en cm sur carte dep rpi et en mm sur arduino / deplacements)
                        String intery=rzIni.substring (rzIni.indexOf('Y')+1);
                        yR=intery.toDouble();
                        
                        }
    
                      if(serialRpiData[2]==  'C') {
                        rzCible=serialRpiData;//
                        //extraction des valeurs X etY   et X en 3 val commence à 4
                        String intercx=rzCible.substring (4, rzCible.indexOf('Y'));//string.substring(val,to) val inclus 'to' exclus
                        xC=intercx.toDouble();//distance en cm sur rz vituel rpi et en mm sur arduino / deplacements reel)
                        String intercy=rzCible.substring (rzCible.indexOf('Y')+1);
                        yC=intercy.toDouble();
                        //initialisation odométrie après transfert info cible
                        iniAsservissement();
                        odometrie=1;
                        Serial.println("<MXcini"+String(xR)+"Yini"+String(yR)+"Xcible"+String(xC)+"Ycible"+String(yC)+">");
                        delay(10);//test
                        }
                  //Message à arduino}
                  codeDir=1;//activation moteur
                   Serial.println("<MXini"+String(xR)+"Yini"+String(yR)+"Xcible"+String(xC)+"Ycible"+String(yC)+">");
                   
                            
             } //fin R
                
         if(testRpi){Serial.println("<Modométrie"+String(xC)+">");}
         delay(10);
         }//fin de K
                          
    else{ Serial.println("<M ordre non reconnu par arduino>"); };//fin des else-if
  
   }//fin cas newRpiData==true

}//fin rpitraitement

/******fonction sonar inutile qd arrêt vbluetooth et vrpi*Avec HC-SR04 blocage possible si dépasse distance max trop souvent ou inversement trop prêt (2cm min)*******************/
void fSonarBlue(){
      // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
      //faire alternative de traitement selon le mode
    
for (uint8_t i = 0; i < SONAR_NUM; i++) {// activer ou désctiver partie test sur Serial
    delay(15);
   
    if( sonar[i].ping_cm() == 0 ) { transfertSonarCode[i]=0; }
    else if( sonar[i].ping_cm()< sonarDanger[i]) { transfertSonarCode[i]=1; }
    else if ( sonar[i].ping_cm() > 2* sonarDanger[i]) { transfertSonarCode[i]=3; }
    else{ transfertSonarCode[i]=2;};
    if(testUs){Serial2.print("US"+ String(i)+"-"+String(sonar[i].ping_cm())+"cm-codedanger:"+String(transfertSonarCode[i])+"  --");}//test
    if(transfertSonarCode[i]!= transfertSonarCodeOld[i]){//test si modif info code couleur avant envoi
      if(transfertSonarCode[i]==1){Serial2.print ("*"+String(i)+"R255G0B0*");}
      else if(transfertSonarCode[i]==2){Serial2.print ("*"+String(i)+"R255G125B0*");}
      else{Serial2.print ("*"+String(i)+"R0G204B0*");};
      transfertSonarCodeOld[i]=transfertSonarCode[i];
      }
   }//end for
    if(testUs){Serial2.println();}; 
}

void fSonarRpi(){ //idem fsonar hors drapeau à terme
      // Wait 50ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
      //faire alternative de traitement selon le mode
    if(test){Serial.println("debut sonar");}
for (uint8_t i = 0; i < SONAR_NUM; i++) {// activer ou désctiver partie test sur Serial
    delay(15);// 9 US 270ms /us ini delay40 descendu à 10
   //abrev couleur en anglais r-o-g (green et non vert
    if( sonar[i].ping_cm() == 0 ) { transfertSonarCode[i]=0; }
    else if( sonar[i].ping_cm()< sonarDanger[i]) { transfertSonarCode[i]=1; }
    else if ( sonar[i].ping_cm() > 2* sonarDanger[i]) { transfertSonarCode[i]=3; }
    else{ transfertSonarCode[i]=2;};
    if(transfertSonarCode[i]!= transfertSonarCodeOld[i]){//test si modif info code couleur avant envoi
      if(transfertSonarCode[i]==1){Serial.print ("<*r"+String(i)+">");}//danger Rge R255G0B0
      else if(transfertSonarCode[i]==2){Serial.print ("<*o"+String(i)+">");}//intermediaire Orange R255G125B0
      else{Serial.print ("<*g"+String(i)+">");};//distance ok vert "R0G204B0
      transfertSonarCodeOld[i]=transfertSonarCode[i];
       }
   }//end for
   if(test){Serial.println("fin sonar");}
}
/******envoi des données moteurs à UNO**/
void envoiUno(){  // 
      for (int i = 0; i <= 7; i+= 1) { //
           Serial3.print(transMoteurP[i]);
           delay(10);  //ini10 
         }
     if(testMoteur){Serial.print(" ds fonction envoiUno sur serial3: ");Serial.print(transMoteurP);Serial.println("finenvoiuno");delay(100);};    
 }
 
 void choixModeRz ()
{
   switch (modeRzB){
         case  ARRET : //0 arret brutal moteur
         if(Serial3){transMoteurP[1]=transMoteurP[3]=transMoteurP[5]='0' ;envoiUno();}
         autoriseMoteur = false; 
         //Serial.print("<0>");//Serial3.print("<0>");// envoi à la Uno l'ordre d'arrêt moteur
         if(modeCom==1){Serial.println("modComtab");iniTablette();} //initialisation des variables 
         else if(modeCom==2){iniTabBordRpi();modeRzB=1;Serial.print("<Marret>");}//idem
         else if(modeCom==0){
          //choix du mode de communication
          Serial2.print ("*n"+String("Activer la tablette ou le site")+"*");
                  if (Serial2.available()>0){ //Bluetooth liaison tablette
                     bluetoothData = Serial2.read();  delay(10);  //read Serial
                     if (test) {Serial2.println("*n" +String("AVfonctbluerecep")); Serial2.print(bluetoothData);}
                     if (bluetoothData == 'S'){modeCom=1;iniTablette();Serial2.print("*n"+String("Tablette active - Choisir le mode ")+"*");} 
                     if (bluetoothData =='s'){iniTablette();Serial2.print("*n"+String("Mettre sur ON ")+"*");}   
                  }  
                  if (Serial.available()>0){
                    //Serial.println("<M serial reçu deb traitement>");// test provisoire
                    String recu  = Serial.readStringUntil('\n');// provisoire ?  original : Serial.readStringUntil('\n')
                     delay(30);
                     String recuv = "<Mrecu :"+recu+">";//prepa envoi
                     delay(30);
                     Serial.println(recuv);delay(3000);// pour test 3000
                     if (recu=="$S#"){modeCom=2; contrManuelServoTGD = 1;iniTabBordRpi();delay(20); Serial.println ("<Mchoisir le mode>");delay(20);}
                     //else{Serial.println("<MPB/"+recu+"/ >");delay(3000); }       
                  }
         }//fin cas modeCom==0  
         break;
         
          
         case VEILLE ://1 mode inactif attend ordre minimalise conso
           traitementArret= false;
           autoriseMoteur = true;// fevrier
           odometrie=0;//fevrier
           if(modeCom==2 && modeRzB!=oldModeRzB){Serial.print("<Mveille-ok>");}
           
          //mettre moteur à l'arrêt sans passer par A2// voir si nécessaire
          //  A faire voir comment éteindre Rpi s en serie ?
          //Pour bluetooth echange activation des capteurs par echange avec le programme arduino ordre - envoi - retour  mise à jour voyant activation 
          //Pour Wifi & node-red activation sur page internet (css) actu direct des voyants et transmission des val. à arduino
          //Ligne suivante envoi fonction spécifique pour Wifi et node-red
          //if(modeCom==2){ventiloOn=1;} // janvier 2022 ctrl ventilo par alim RPI sur RPI directe / économie de batterie
          //Serial.println("<modComVEILLE>");delay(20);
           break;         
          
         case SURVEILLANCE://surveillance sans déplacement
           traitementArret= false;
          //capteurs actifs moteurs mvt éteind
          //detecteurs mvt, détecteurs son,...
           mvtIrVal= digitalRead(mvtIrPin);// stockage resultat mouvement ou non mvtIrVal = true ou false
            if (mvtIrVal==LOW){digitalWrite(activeBuzzerPin,LOW);if (modeCom==1){Serial.print("*pR255G255B240*");}}
            else{digitalWrite(activeBuzzerPin,HIGH);if (modeCom==1){Serial.print("*pR0G255B0*");Serial.print("*jV50*");}}
           detecteurMvt=1;//cohérance affichage 
          // if (mvtIrVal==1){Serial.print("Mouvement détecté!");} else{Serial.print("Pas de mouvement!");digitalWrite(activeBuzzerPin,LOW);} // test
          //veille reconnaissance vocale
          //mvt servo. actifs
          //prepa donnée pour communication
          //sous menu d'option / capteurs actifs : caméra,...
          //Serial.println("<modSurveillance");delay(2000);
           break;
            
         case AUTONOME ://3 mode surveillance et mouvement autonome possible
         traitementArret= false;
         if(modeCom==2){Serial.print("<Mautonome-ok>");}//ventilo normalement déjà actif // alim ventilo directe par RPI ventiloOn=1;
         //idem 2 avec ensemble des capteurs actifs  en plus programme de réaction autonome aux capteurs avec moteurs actifs
           //capteurs actifs moteurs mvt possible
          //detecteurs mvt, détecteurs son,...
          // mvtIrVal= digitalRead(mvtIrPin);// stockage resultat mouvement ou non mvtIrVal = true ou false
         // if (mvtIrVal==0){digitalWrite(activeBuzzerPin,LOW);if (modeCom==1){Serial.print("*pR255G255B240*");}}
          //else{digitalWrite(activeBuzzerPin,HIGH);if (modeCom==1){Serial.print("*pR0G255B0*");Serial.print("*jV50*");}}
           //detecteurMvt=1;//cohérance affichage 
         // actu. des var autorisant mvt
           autoriseMoteur = true;
           //if(odometrie==1){Serial.print("<M Odométrie activée>");}else{Serial.print("<M Odométrie Non Activée>");}
           delay(100);//lecture info 
         // sous programme spécifique : écoute ordre vocaux ou et odometrie, comportement fuite ou suivi A finir
         
           break; 
         case SUIVEUR ://4 mode suiveur
        traitementArret= false;//fevrier
        autoriseMoteur = true;//febrier
        //variante A3 évolué avec ensemble des capteurs actifs   programme de gestion de suivi en environnement complexe
           break; 
         case TELECOMMANDE :   //5  //mode contrôlé par humain via bluetooth v(seulement?) sous menu pref?
          if(modeCom==2){Serial.print("<Mtelecommande-ok>");}
          traitementArret= false;//Serial.println();
          autoriseMoteur = true;//fevrier
          if(testOdometrie==1){odometrie=1;} else {odometrie=0;};//fevrier
          //Serial.println("Case télécommande");
          //detecteurMvt=1;
         // activeMicro=1;
           break;
           
         default :       //on prend le mode veille par defaut sous forme numérique directe : ratrapage erreur
           modeRzB = 0;
           //Serial.println();
           if(modeCom==2){Serial.println ("<M_PBmodeRZpasse à arrêt sur arduino>");delay(30);}
   }   
  }
 //Initialisation tablette
void  iniTablette(){ // qd modeCom change ou que l'on passe sur arrêt pour le modeRzB - ini des boolean à false
   if (traitementArret==false){
   //zz ventilo rpi ? - bluetooth ?
   Serial2.print ("*m"+String("Arret")+"*");
   transMoteurP[1]= 0 ;transMoteurP[3]= 0; transMoteurP[5]=directionMoteur=0;// Moteur Arrêt -source d'erreur verif zz -PAS D'ACTU sur AFICHAGE TABLETTE de V et A
   contrManuelServoTGD =0; Serial2.print("*tR0G0B0*"); Serial2.print("*a90*"); // Angle servo tête - ini du cadran et position réel tête
   if(posServoTGDR!=correctionPosServoTGD){iniservoTGD();}
   Serial2.print("*jV00*");  alerteSonore = false;                                                //Alerte sonore sur tablette - V de 0 à 100 ne pas confondre avec l'alerte sur le robot
   detecteurMvt=0; mvtIrVal= 0; Serial2.print("*pR0G0B0*");                                       //Detecteur mvt P
   activeMicro=0;oldIntMSonAv=0;oldIntMSonGche=0;oldIntMSonDrt=0;oldIntSmartSon;Serial2.print("*h00*"); Serial2.print("*qR0G0B0*");                      //activeMicroQ activ voyant, h barre d'intenssité son, q voyant d'activation son - Micro avant
   //photoRactive=0;oldValIntLuzPhotoR=0;Serial2.print("*iR0G0B0*");Serial2.print("*d*");           //Photodetection à inactif  - voir si ok ?("*d""*")zz utliser smartphone si possible A FAIRE
   cameraHD=0;Serial2.print("*cR0G0B0*");                                                         //Caméra 1
   activeBuzzer=0;digitalWrite(activeBuzzerPin , LOW);Serial2.print("*eR0G0B0*");                 //Son sur RZ
   activeRing=0;Serial2.print("*fR0G0B0*");                                   //luz RZ (ancienne lumière corps)
   boutonControl=0;Serial2.print("*kR0G0B0*");                               //Emotion communication couleur
   boutonVert = false; //bouton G
   boutonJaune = false;//bouton J
   boutonRouge = false; //bouton R 
   //Emotion RGB init - Emetteur
    digitalWrite(emotionBluePin, LOW);blueMValue = 0;
    digitalWrite(emotionGreenPin, LOW);greenMValue = 0;
    digitalWrite(emotionRedPin, LOW);redMValue = 0;
    receptionMessage="";
    autoriseMoteur = false;// fevrier
    odometrie=0;//fevrier
     }
    if(modeCom!=0){traitementArret= true;} ///var  type drapeaux à false pour les mode autres que arrêt et si mode com=0 pour permettre l'initialisation via la fonction iniTabBordRpi qui ne peut être traité autrement (false attendu)œ
}
 
 //Initialisation Tableau de bord RPI
void  iniTabBordRpi(){ //zz revoir qd modeCom change ou que l'on passe sur arrêt pour le modeRzB - ini des boolean à false
   if (traitementArret==false){
       transMoteurP[1]= '0' ;transMoteurP[3]= '0'; transMoteurP[5]='0';// Moteur Arrêt -source d'erreur verif zz -PAS D'ACTU sur AFICHAGE TABLETTE de V et A
       contrManuelServoTGD =1;  // Angle servo tête - ini du cadran et position réel tête
       if(posServoTGDR!=correctionPosServoTGD){iniservoTGD();}
       alerteSonore = false; //Alerte sonore sur tablette - V de 0 à 100 ne pas confondre avec l'alerte sur le robot
       detecteurMvt=0; mvtIrVal= 0; 
       activeMicro=0;oldIntMSonAv=0;oldIntMSonGche=0;oldIntMSonDrt=0;oldIntSmartSon=0; //activeMicroQ activ voyant, h barre 
       //photoRactive=0;oldValIntLuzPhotoR=0;//Photodetection à inactif - on ne garde que photodetection smart
       cameraHD=0; //pas utilisé
       activeBuzzer=0;digitalWrite(activeBuzzerPin , LOW);//Son sur RZ
       activeRing=1;//luz RZ (ancienne lumière corps)
       boutonControl=1;//Emotion communication couleur
       boutonVert = false; //bouton G
       boutonJaune = false;//bouton J
       boutonRouge = false; //bouton R 
       //Emotion RGB init - Emetteur
        digitalWrite(emotionBluePin, LOW);blueMValue = 0;
        digitalWrite(emotionGreenPin, LOW);greenMValue = 0;
        digitalWrite(emotionRedPin, LOW);redMValue = 0;
        receptionMessage="";
        autoriseMoteur = false;// fevrier
        odometrie=0;//fevrier
        if(testRpi){"<M_Fin iniTabBordRpi>";}
    
      }
    
   if(modeCom!=0){traitementArret= true;}; // on le repasse à false dans tous les modes autres que ARRET 
}


 
void iniservoTGD(){  //servo positionne pour faire 180° vers la gauche / sens avance et 60 vers la droite pos tt droit 30
  posServoTGD= 0;//varie de -90 à +90 théoriquement
  degreTGD=servoTGD.read();
  
  posServoTGDR=posServoTGD+correctionPosServoTGD;//posServoTGD=valcalcule à partir de l'affichage /signe et correctionPosServo / ajuster 0 affichage à 90 moteur en tenant compte du décalage au moment de l'installation 
  //correctionPosServoTGD val de correction pour obtenir via la commande write.servo appliqué à posServoTGDR la val adhoc
  // à l'arrêt (et au démarrage) ajuster la valeur envoyé au servo à posServoTGDRcorrigé
  if (degreTGD>posServoTGDR) {
    int delta = abs(degreTGD-posServoTGDR);
    int i ;
    for(i = delta; i>=1; i--)
      {
        degreTGD= degreTGD-1;
        servoTGD.write(degreTGD);
      }
  }
  else if (degreTGD<posServoTGDR) {
    int delta = abs(posServoTGDR-degreTGD);
    int i ;
    for(i = 0; i<=delta; i++)
      {
        degreTGD= degreTGD+1;
        servoTGD.write(degreTGD);
      }
  }
  contrManuelServoTGD = false; // pour inverser !var initialiser dans le setup ?
}
/*****Fonctions réception des messages de RPI sur serie optimisé voir one note serie*******/
void recvRpiWithStartEndMarkers(){
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '$';
    char endMarker = '#';
    char rc;
 
    while (Serial.available() > 0 && newRpiData == false) {
        rc =Serial.read() ; //readStringUntil('\n') // //until('\n') permet d'enlever le caractère retourligne qd pb verif si origine this
        if (recvInProgress == true) {
                if (rc != endMarker) {
                  serialRpiData[ndx] = rc;
                  ndx++;
                  if (ndx >= numChars) {ndx = numChars - 1; }
                 }
                else {
                 lgRpiData=ndx;//on garde le caractère nul pour retransformer en string la valeur reçue
                 serialRpiData[ndx] = '\0'; // terminate the string
                 recvInProgress = false;
                // Serial.println("<nndx"+String(ndx)+">");delay(2000);
                 ndx = 0;
                 newRpiData = true;
                 }
         }

        else if (rc == startMarker) {recvInProgress = true;}
    }
}//end fonction
/*****Fonctions liées au Buzzer*******/

void claxon(){  // assocition à mode veille à faire}
  
  unsigned char i,j;
  if(activeBuzzer)
  {
    //output an frequency
    for(i=0;i<80;i++)
      {
      analogWrite(activeBuzzerPin,HIGH);
      delay(1);//wait for 1ms
      analogWrite(activeBuzzerPin,LOW);
      delay(1);//wait for 1ms
      }
    //output another frequency
    for(i=0;i<20;i++)
      {
      analogWrite(activeBuzzerPin,intensiteBuzzer);//0à255
      delay(frequenceBuzzer);//ini 2ms - ici pas la frequence au sens de tone(broche,fréquence,durée du signal avec fin par noTone(broche) )
      analogWrite(activeBuzzerPin,LOW);
      delay(frequenceBuzzer);//
      }
  }
}

/* void melodie(){ //boucle pour parcourir les notes de la mélodie voir pb avec fonction tone / interruptions ?
     for (int thisNote = 0; thisNote < 8; thisNote++) { // thisNote de 0 à 7
           // pour calculer la durée de la note, on divise 1 seconde par le type de la note
           //ainsi noire = 1000 / 4 sec, croche = 1000/8 sec, etc...
        int noteDuration = 1000/noteDurations[thisNote];
           // joue la note sur la broche 3 pendant la durée voulue
        tone(activeBuzzerPin, melody[thisNote],noteDuration);//arg1 pin buzzer, arg 2 frequence en Htz, arg3 durée
           // pour distinguer les notes, laisser une pause entre elles- la durée de la note + 30% fonctionne bien :
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes); // delai entre les notes
     }
     noTone(3);
}*/

/*void sonArduinoStop(){noTone(3);} // stoppe la production de son sur la broche 3 :*/
    

/****Potentiomettre micro (big sound?)***Penser à régler vis de sensibiliter - intensité son moyen - direction son AFAIRE au moin coté RPI***/
void microTraitement(){// différent sur android directe et RPI - si micro smartphone activé on ne donne que la direction son le plus fort. Sinon l'intensité moyenne
    int deltaIntSignificative = 5 ; // val arbitraire en % - A étalonner pour transmission d'une modification de l'orientation du son
    int minMAv= 1024; int minMGche = 1024; int minMDrt = 1024; //initialisation à chaque séquence de lecture  
    int maxMAv = 0; int maxMGche = 0; int maxMDrt = 0;
    if(choixMicro!=0){
    for (int i = 0; i < 5; ++i) { //Peut être étalonne pour efficacite sur x mesures
      int valMAv = analogRead(microAvIntSon); minMAv = min(minMAv, valMAv); maxMAv = max(maxMAv, valMAv);// microAvIntSon intensité mesurée directement sur microphone
      int valMGche = analogRead(microGcheIntSon); minMGche = min(minMGche, valMGche); maxMGche = max(maxMGche, valMGche);
      int valMDrt = analogRead(microDrtIntSon); minMDrt = min(minMDrt, valMDrt); maxMDrt = max(maxMDrt, valMDrt);
       }
    //Mappage des données obtenues en % de écart 
   int intSonMAv=map((maxMAv-minMAv),0,1023,0,100);//map(valeur,min,max,transMin,transMax); ici convertion de 1024 valeurs en var %
   int intSonMGche=map((maxMGche-minMGche),0,1023,0,100);//map(valeur,min,max,transMin,transMax); ici convertion de 1024 valeurs en var %
   int intSonMDrt=map((maxMDrt-minMDrt),0,1023,0,100);//map(valeur,min,max,transMin,transMax); ici convertion de 1024 valeurs en var %
   //Test si on a besoin d'actualiser la valeur affichée et envoi si nécessaire
   if (oldIntMSonAv!=intSonMAv || oldIntMSonGche!=intSonMGche || oldIntMSonDrt!=intSonMDrt){
       
      oldIntMSonAv=intSonMAv;oldIntMSonGche=intSonMGche;oldIntMSonDrt=intSonMDrt;//actu du resultat de la dernière séquence de mesure passe en old
      // Cas liaison bluetooth - juste moyenne intensité des micros
      if(modeCom==1){int moyenneIntMicros= (intSonMAv + intSonMGche+intSonMDrt)/3 ;Serial2.print("*h"+String(moyenneIntMicros)+"*");} // Tablette - bluetooth
      // Cas liaison RPI - Wifi - juste transmission de la direction 
      if(modeCom==2){ // on transmet l'écart au volume sonor central si cet écart est supérieur à un deltaSignificatif et en précisant l'orientation
          if ( abs(intSonMAv-intSonMGche) >= deltaIntSignificative || abs(intSonMAv-intSonMDrt) >= deltaIntSignificative) { //Test si dépassement de la val considéré comme significative 
            int deltaIntSon;
            if( intSonMGche>intSonMDrt) {deltaIntSon= intSonMGche-intSonMAv ; Serial.print("<Hg"+String(deltaIntSon)+">");}//delta son à gauche en % par rapport à Av
            else {deltaIntSon= intSonMDrt-intSonMAv ; Serial.print("<Hd"+String(deltaIntSon)+">");}
            //pas d'enregistrement d'uneOldValeur car l'information reste pertinente pour un suivi et si réaction servo moteur du cou permet de savoir si mvt rotation suffisant
             } 
      }//end modeCom2
   }//end test de var intensite sonor
  }//end cas ou micro activé
}//end fonction micro   

/*****Fonction de emotionControl (manuel) - Passer sur pin analog /place dispo si nécessaire***/
 void chgeEmotion(){
  if (boutonControl){
    Serial.println("<MBtonChgeEmo>");
    boutonTpsFin=0; // désactive le ctrl de loop par rapport au tps
    if (boutonVert) {analogWrite(emotionGreenPin, 255);analogWrite(emotionRedPin, 0);Serial2.print("*kR0G200B0*");delay(5);}//pas de bleu ici analogWrite(emotionBluePin, 0);
    else if (boutonRouge){analogWrite(emotionGreenPin, 0);analogWrite(emotionRedPin, 255);Serial2.print("*kR200G0B0*");delay(5);}//pas de bleu ici analogWrite(emotionBluePin, 0);
    else if (boutonJaune){analogWrite(emotionGreenPin, 75);analogWrite(emotionRedPin, 255);Serial2.print("*kR255G165B0*");delay(5);}////pas de bleu ici analogWrite(emotionBluePin, 0);
    else {Serial2.print("*kR144G144B144*");delay(5);}//pas de bouton couleur enclenché mais emotion active
     }
  // la gestion de frequence se fait avec fction specifique
  else {analogWrite(emotionGreenPin, 0);analogWrite(emotionRedPin, 0);analogWrite(emotionBluePin, 0);Serial2.print("*kR0G0B0*");delay(5);}//contrôle manuel activé et non enclenché*
  }

 void lawEmotion(){
    digitalWrite(emotionBluePin, LOW);blueMValue = 0;
    digitalWrite(emotionGreenPin, LOW);greenMValue = 0;
    digitalWrite(emotionRedPin, LOW);redMValue = 0;
    }
/****** fonctions Liées à ring Neopixel*********/
// ajouter un retour vers rpi via reception messageqd l fonction est fini pour mettre le menu de fonction à rien
//Allumer toutes les pixels du ring 1 seconde avec parametre couleur, intensite et frequence
void ringColor(){// color Vert-rouge-bleu
    activeRing=1;
    ringPixels.begin();
    ringPixels.clear(); // Set all pixel colors to 'off'
    //for(int i=0; i<ringNumPixels; i++) {
    //ringPixels.setPixelColor(i, ringPixels.Color(greenRC, redRC, blueRC));
     //}
     ringPixels.fill(ringPixels.Color(greenRC,redRC,blueRC),0,ringNumPixels);   // allume la led 1,2,3 et 4  en vert
     ringPixels.setBrightness(intensiteRing);
     ringPixels.show();
     ringTpsFin= millis()+2000; //défini fin de l'affichage
    
}

//led ring bouche triste en gris 2 lignes nécessaires / led allumées
void triste()
  {
    activeRing=1;
    ringTpsFin= millis()+3000;
    ringPixels.begin();
    ringPixels.clear();//en debut initialise à vide l'affichage 
    ringPixels.fill(ringPixels.Color(128,128,128),10,2);   // allume les leds du numéro à partir de 10 2 leds 1,2,3 et 4  en vert
    ringPixels.fill(ringPixels.Color(128,128,128),0,3);   // allume la led 0 à 2 inclus
    ringPixels.show();
    
  }

void parleBche() {// forme bouche qd il parle
    activeRing=1;
    ringPixels.begin();
    ringPixels.clear();
    ringPixels.fill(ringPixels.Color(161, 40, 134),5,3);   // allume 5 leds à partir de led 4: 4,5,6,7,8  en vert
    ringPixels.show();
    ringPixels.setBrightness(50);
    ringTpsFin= millis()+2000;
   }

void bonjour()   {
    activeRing=1;
    ringPixels.begin();
    ringPixels.clear();
    ringPixels.fill(ringPixels.Color(125,0,0),4,5);   // allume 5 leds à partir de led 4: 4,5,6,7,8  en vert
    ringPixels.show();

    analogWrite(emotionGreenPin, 255);

    activeBuzzer=1;
    frequenceBuzzer=2;
    claxon();
 //debut code ruban intégré
    if (rubanEmotion==true){
      activeRubanLed=1;
      rubanPixels.begin();
      rubanPixels.clear();
      rubanPixels.fill(ringPixels.Color(125,0,0),1,8);   // allume  à partir de led 1 (la zero est celle à l'intérieur) 8 leds  en vert
      rubanPixels.show();
      rubanTpsFin= millis()+1800;
      }
 //fin code ruban integré et fin de la fonction
    boutonTpsFin=millis()+2001;
    ringTpsFin= millis()+2002;
    buzzerTpsFin=millis()+2003;
    }

void sourire()
   {
    activeRing=1;
    ringPixels.begin();
    ringPixels.clear();
    ringPixels.fill(ringPixels.Color(155,0,0),4,5);   // allume la led 1,2,3 et 4  en vert
    ringPixels.show();
    ringTpsFin= millis()+2000;
   }

void alerte()
   {
    
        
    analogWrite(emotionRedPin, 255);

    activeBuzzer=1;
    frequenceBuzzer=2;
    claxon();
    
    activeRing=1;
    ringPixels.begin();
    ringPixels.clear();
    for (int i = 0; i <= ringNumPixels; i= i+2)
    {
    ringPixels.fill(ringPixels.Color(0,255,0),i,1);   // allume la led 1,2,3 et 4  en vert
    ringPixels.fill(ringPixels.Color(0,0,0),i+1,1);   // allume la led 1,2,3 et 4  en vert
    }
    ringPixels.show();
    delay(300);
    for (int i = 1; i <= ringNumPixels; i= i+2)
    {
    ringPixels.fill(ringPixels.Color(0,255,0),i,1);   // allume la led 1,2,3 et 4  en vert
    ringPixels.fill(ringPixels.Color(0,0,0),i+1,1);   // allume la led 1,2,3 et 4  en vert
    }
    ringPixels.show();
    delay(200);

 
    boutonTpsFin=millis()+2000;
    //ringTpsFin= millis()+2000;
    buzzerTpsFin=millis()+2000;
    }

void non()
  {
  int posServoTransitoireP;//val min bonjour selon position ini
  int posServoTransitoireM;//val max possible
  int angleNon=20; // mvt en plus et en moins de la val degreTGD de déplacement pr Non;
  Serial.print ("<Mdsfction>");
  servoTGD.attach(4);
  degreTGD=servoTGD.read();
  posServoTGDR=degreTGD;
  //cohérance bjour et angle min max
  posServoTransitoireP=degreTGD-angleNon;
  posServoTransitoireM=degreTGD+angleNon;
  if(posServoTransitoireP < posServoTGDRMin){posServoTransitoireP=posServoTGDRMin;}
  if(posServoTransitoireM > posServoTGDRMax){posServoTransitoireM=posServoTGDRMax;}
  for(int i= degreTGD; i<=posServoTransitoireM; i++){servoTGD.write(i);delay(10);}
  for(int i= posServoTransitoireM; i>=posServoTransitoireP; i--){servoTGD.write(i);delay(10);}
  for(int i= posServoTransitoireP; i<=degreTGD; i++){servoTGD.write(i);delay(10);}
  servoTGD.detach();
  oldPosServoTGDR=posServoTGDR=degreTGD; // actu des variables
  }
  

void oui()//
  {
  int posServoTransitoireP;//val min bonjour selon position ini
  int posServoTransitoireM;//val max possible
  int angleOui=20; // mvt en plus et en moins de la val degreTHB de déplacement pr Oui;
  sourire();
  servoTHB.attach(5);
  degreTHB=servoTHB.read();
  posServoTHBR=degreTHB;
  //cohérance bjour et angle min max
  posServoTransitoireP=degreTHB-angleOui;
  posServoTransitoireM=degreTHB+angleOui;
  if(posServoTransitoireP < posServoTHBRMin){posServoTransitoireP=posServoTHBRMin;}
  if(posServoTransitoireM > posServoTHBRMax){posServoTransitoireM=posServoTHBRMax;}
  for(int i= degreTHB; i<=posServoTransitoireM; i++){servoTHB.write(i);delay(10);}
  for(int i= posServoTransitoireM; i>=posServoTransitoireP; i--){servoTHB.write(i);delay(10);}
  for(int i= posServoTransitoireP; i<=degreTHB; i++){servoTGD.write(i);delay(10);}
  servoTHB.detach();
  oldPosServoTHBR=posServoTHBR=degreTHB; // actu des variables
  }

void temoinLuz(){ // pas de durée
      activeRing=1;
    ringPixels.begin();
    ringPixels.clear();
    ringPixels.fill(ringPixels.Color(161, 40, 134),5,3);   // allume 5 leds à partir de led 4: 4,5,6,7,8  en vert
    ringPixels.show();
    ringPixels.setBrightness(50);
  }
//Chenillard avec delay


/****** fonctions Liées à rubanLed Neopixel *********/
 
// ajouter un retour vers rpi via reception messageqd l fonction est fini pour mettre le menu de fonction à rien
//Allumer toutes les pixels du ruban 1 seconde avec parametre couleur, intensite et frequence
void rubanLedColor(){// color Vert-rouge-bleu
    activeRubanLed=1;
    rubanPixels.begin();
    rubanPixels.clear(); // Set all pixel colors to 'off'
    //for(int i=0; i<rubanNumPixels; i++) {
    //rubanPixels.setPixelColor(i, rubanPixels.Color(greenRBC, redRBC, blueRBC));
     //}
     rubanPixels.fill(rubanPixels.Color(greenRB,redRB,blueRB),0,rubanNumPixels);   // allume la led 1,2,3 et 4  en vert- ? voir setup
     rubanPixels.setBrightness(intensiteRing);
     rubanPixels.show();
     rubanTpsFin= millis()+2000; //défini fin de l'affichage
 }

/*Pour ruban on déclenchera certaines actions en fonction en utilisant programme tiers
 *             else if(serialRpiData[1]==  'P'){rubanFuite=true;}//fuite qd mvt se rapproche de US specifique
            else if(serialRpiData[1]==  'S'){rubanSuivre=true;}
            else if(serialRpiData[1]==  'T'){rubanTourne=true;}// clignotant fonction mvt direction moteur
            else if(serialRpiData[1]==  'E'){rubanEmotion=true;}
 */








/** fonction odométrique d'asservissement limite précision choisis 200 mm
 Suppression accidentel pb ?
  Fonction d'intéruption pour le codeur gauche Fonction appelée par un tic du codeur gauche puis droit. 
On regarde le sens de rotation de celui-ci (on teste l'état de la voie déphasée) et on 
incrémente ou décrémente le compteur ticksCodeurG. */

   

void asservissement(){
  //comptG monte qd moteur G vers l'avant si moteur G seul tourne à droite -Md tourne à 0.965 de la v du gauche
  //4480 TIC par tour soit 314 mm soit 14tic/mm soit 1 tic=0.07mm  //coeffGLong = 0.0712mm; // 14 tic/mm  soit : constant pour transfo nbre tic en millimetre (vérifier valeur par test sur 1m de déplacement)
  //On envoiifo à RPI via "<kab"+string(var)+">" ou ab est un case à traiter de valeur var
  coeffDLong = 0.0712;//0.0714;
  coeffGLong=  0.0712;
  coeffGAngl = 0.08035;//0.0138; //1 tour 360° 4480 tic 1 tic = 0.08035 °//=0.001402 rad constante pour transfo nbre tic en degrés (vérifier valeur par test sur dix tours)
  coeffDAngl = 0.08035;//0.0138;//
  int deltaVGD=round(197/207);
  nAsser=nAsser+1;//Cpte boucle
  int kra = 1;// qd 0 arrêt
 //xC=3000;//test
 //yC=0;
 //test
 //On commence par calculer les variations de position en distance et en angle
  Serial.print ("<MnbreBcle"+String(nAsser)+" timedebloop:"+String(dure)+">");
  dDist = (coeffGLong*(comptG-oldComptG) + coeffDLong*(comptD-oldComptD))/2;//coeffGlong=1/nbre tic/mm
  dAngl =  coeffDAngl*(comptD-oldComptD)-coeffGAngl*(comptG-oldComptG);
  oldComptG=comptG;
  oldComptD=comptD;
  //dAngl = 0;//pour test
  //Actualisation de la position du robot en xy et en orientation
  xR += dDist*cos(dAngl);//si angle 0 cos0=1 sin0=0
  yR += dDist*sin(dAngl);
  orientation += dAngl;// inversion horaire et anti-horaire
  //int xRob=xR;
  //int yRob=yR;
  //Envoie de la position courante à la rpi (s'enclenche si chgement comptG ou comptD
  //Serial.print ("<krx"+String(xR)+">");
  //Serial.print ("<kry"+String(yR)+">");
 
 //  système de déplacement optimisé

 //On calcule la distance séparant le robot de sa cible
  distanceCible = sqrt((xC-xR)*(xC-xR)+(yC-yR)*(yC-yR));//sqrt racine carré
 //On regarde si la cible est à gauche ou à droite du robot
  if(yR > yC){signe = 1;} //ori>
  else {signe = -1;}
  
  //On calcule l'angle entre le robot et la cible 
 if(xC==xR){xR=xR+1;};
 if(yC==yR){yR=yR+1;};
 aRC = consigneOrientation = signe * acos (1);//((xC-xR)/((xC-xR)*(xC-xR)*(yC-yR)*(yC-yR)));//precaution pour éviter nan due à delta on ajoute 1 si egal

 //On détermine les commandes à envoyer aux moteurs
  cmdD = abs((int)distanceCible);
  int disCib= cmdD;//distance cible - cmdD modifié après
  
  //on fixe un maximum à la vitesse
  if(cmdD>200) { cmdD = 200; }
  cmdG = cmdD;//déplacement tout droit 
 
 //Calcul de l'erreur
oR = erreurAngle =  consigneOrientation - orientation;
  
 //Calcul de la différence entre l'erreur au coup précédent et l'erreur actuelle.
vAngl= deltaErreur = erreurAngle - erreurPre;
   
 //Mise en mémoire de l'erreur actuelle
  erreurPre  = erreurAngle;
 //Calcul de la valeur à envoyer aux moteurs pour tourner - en tic? en mm
 //coeffP = 5.0  coeffD = 3.0;
 coeffP=3;//2.7 ini coeffP 0,5 et coeffD 0,4 à 255 max moteur av 5 3
 coeffD=2;//1.25
  //intégration de l'erreur dans limite vitesse 0 à 250
  int dC = coeffP*erreurAngle + coeffD * deltaErreur;//dC delta commande
  cmdG -= dC;//
  cmdD += dC;
  if(cmdD>210) {cmdD=210;}//207 / delta v DG
  else if(cmdD < 0) {cmdD =0;}
  if(cmdG>210) {cmdG = 200;}// 193 obtenu par 200x fraction de vitesse  D/G compensé vitesse mg sup à md
  else if(cmdG < 0) {cmdG = 0;}
  int mG= cmdG;
  int mD= cmdD;
      //arrêt
    if((cmdG==cmdD && cmdG==0) ||(cmdD<200 && cmdG<200)&& modeRzB==3){Serial.print ("<kra>");codeDir=0;//arrête moteur
     sprintf (transMoteurAuto, "$%d,%d,%d>\n",codeDir,cmdG,cmdD);//transMoteurAuto array de taille 20
     kra = 0;
      //Serial3.print(transMoteurAuto);//envoi du print sur uno en une passe ? ou voir pour f° envoi Uno
      for (int i = 0; i <= 19; i+= 1) { //
               Serial3.print(transMoteurAuto[i]);
               delay(10);  //ini10 
      }
      Serial.print ("<MArrive:codeDir,cmdG,cmdD"+String(comptG)+" comptD:"+String(comptD)+">");
       erreurPre=0.;
       orientation=0.;
      delay(100);
  }//end if
   //poursuite
  if( codeDir==1 && modeRzB==3){//mode 3 autonome permet bloquer transmission moteur pour enregistrer des déplacements télécommandé
    //cmd indice de vitesse  conversion ds Uno pour 
    //équivallence avec un max de 1 hors automatique = v=(1/200)*40
      sprintf (transMoteurAuto, "$%d,%d,%d>\n",codeDir,mG,mD);//transMoteurAuto array de taille 20
      //Serial3.print(transMoteurAuto);//envoi du print sur uno en une passe ? ou voir pour f° envoi Uno
      for (int i = 0; i <= 19; i+= 1) { //
               Serial3.print(transMoteurAuto[i]);
               delay(10);  //ini10 
      }
//      Serial.print ("<kmg"+String(cmdG)+">");// pour test graph
//      Serial.print ("<kmd"+String(cmdD)+">");// pour test graph
//      Serial.print ("<kdc"+String(dC/10)+">");// pour test graph

    }
//  double aRC = consigneOrientation;
//  double oR =erreurAngle;
//  double vAngl=deltaErreur;
  int pCG = cmdG;
  int pCD = cmdD;
  //construction d'une chaine jason pour tableau
  Serial.print("<ktb");//entrer dans case
//  Serial.print("{\"comptG\":");//ticG//2 premieres ligne jason - barre d'échappement - comme en parti construit ds rpi on enlève {
  Serial.print(",\"comptG\":");// nb tic
  Serial.print(comptG);
  Serial.print(",\"comptD\":");// 
  Serial.print(comptD); 
  Serial.print(",\"dDist\":");//
  Serial.print(dDist);
  Serial.print(",\"dAngl\":");//
  Serial.print(dAngl);
  Serial.print(",\"oR\":");//orientation courante//
  Serial.print(oR);
  Serial.print(",\"xR\":");//
  Serial.print(xR);
  Serial.print(",\"yR\":");//
  Serial.print(yR);
  Serial.print(",\"disCib\":");//Distance cible 
  Serial.print(disCib);//distance cible
  Serial.print(",\"aRC\":");//consigne orientation 
  Serial.print(aRC);
  Serial.print(",\"vAngl\":");//evolAngle
  Serial.print(vAngl);
  Serial.print(",\"dC\":");//delta Commande
  Serial.print(dC);
  Serial.print(",\"mG\":");//ordre moteur 
  Serial.print(mG);
  Serial.print(",\"mD\":");//
  Serial.print(mD);
  Serial.print(",\"kra\":");//arret attention decalage
  Serial.print(kra);   
  Serial.println("}>");//dernière ligne des jasons et fermeture du case avec > -println
  delay(10);
  }
    /*variable à choisir de distance. 
   Dépend de la vitesse du robot et du temps de déplacement 
   réglé avec un timer. déplacement réalisé calcule par nb tic
  avant modif orientation et distance - autoriseMoteur voir var - tout droit :
   transMoteurP[5]= '1';//direction sens aiguille  1 devant -verif type val - chge v relative des moteurs
  transMoteurP[3]= '1';//vitesse max 9 - garder constant 
  transMoteurP[1]= '4';//Acceleration max4 pas de transition de vitesse test ini
  //Attention A faire traitement des obstacles /USon + avertissement si Animal dont humain (detecteur mvt)
 
  
  construction de la commande moteur transMoteurAuto 15 caractères
  transMoteurAuto[0]='$'; // val 0 va indiquer à uno cas traitement deplacement automatique _ array of char
  transMoteurAuto[1]='1'; // codeDir - val 0 arrêt - val 1 libre on pourait ajouterd'autres case dans le stch de UNO
  
  Serial.print ("<McomptG:"+String(comptG)+" comptD:"+String(comptD)+">");
  }
  
  else if(cmdG==cmdD && cmdG==0) {
    transMoteurP[1]= '4';transMoteurP[3]= '0';transMoteurP[5]= '1'; Serial.print ("<kra>");}//Stop moteur et signal arrive à rpi chge cible en ini sur rpi
  else if(cmdG==200){transMoteurP[1]= '4';transMoteurP[3]= '1';transMoteurP[5]= '3';}//à droite 90° soit dir3
  else {transMoteurP[1]= '4';transMoteurP[3]= '1';transMoteurP[5]= '7';}//à gauche 0 soit dir 7

 }*/


void iniAsservissement(){
comptG = comptD = 1;//ecart avec val knob enclenche le passage dans la boucle d'asservissement et la récup des val ini
dAngl= 0.01;//angle à a cible
erreurPre=0.01;
//xR=yR=1; // à sup + tard
orientation=0;
nAsser=0;
//reinitialiser les encodeurs - enn marche avant sur axe des x
//xR et yR val position initiale du trajet  du robot pour ini odo. on passe ds knob
//ini oldComptG evite erreur odométrie qd comparaison ancienne et nouvelle val
// il faudra traité le cas ou étape avec reprise des valeurs courantes réelles de comptG et comptD sans réinitialisation
int inix=round( xR/coeffDLong);
oldComptG = oldComptD = inix;
knobLeft.write(inix);
knobRight.write(inix);//evite demarrage à 0
//xR=yR=0;
Serial.print ("<Mini asservissement ok>");
Serial.println("<MXiniasser"+String(xR)+"Yini"+String(yR)+"Xcible"+String(xC)+"Ycible"+String(yC)+">");
delay(100);
}

// Fin fonction principale

/* Fonction d'intéruption pour le codeur gauche -  Fonction appelée par un tic du codeur gauche. 
On regarde le sens de rotation de celui-ci (on teste l'état de la voie déphasée) et on incrémente ou décrémente le compteur comptG.
void ReagirG(){  
  if(digitalRead(canalBG) == HIGH)  { comptG--;}
  else{comptG++;}      
}
Fin fonction d'intéruption pour le codeur gauche - Fonction d'intéruption pour le codeur droit.
 Fonction appelée par un tic du codeur droit. On regarde le sens de rotation de celui-ci (on teste l'état de la voie déphasée)
 et on incrémente ou décrémente le compteur comptD.
void ReagirD()
{     
  if(digitalRead(canalBD) == HIGH)
  {
    comptD ++;
  }
  else
  {
    comptD--;     
  }
}
// Fin fonction d'intéruption pour le codeur droit*/
