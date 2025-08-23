/****Sketch du Uno pour le controle des moteurs avec reception des commandes depuis mega
 echange TX RX du seul port serie de la Uno avec le serial 3 de la mega
 Parcage multiple du code recu sécu arrêt automatique avec timer à désactiver pour essai
 ne tiens pas compte d'une correction de vitesse entre moteur à programmer sur la mega
 attention mettre interupteur digital sur off mais interrupteur alim général sur onsur on*****/

#include "DualVNH5019MotorShield.h"

DualVNH5019MotorShield md; // constructeur ? v par defaut
const byte numChars = 32;  // constante espace memoire
 char receivedChars[numChars]; //tableaux des valeurs ascii reçu limitées par mémoire réservée injecter valeurs reçu A FAIRE
 char tempChars[numChars];        // temporary array for use when parsing
 boolean newData = false;  // déclaration et initialisation d'une nouvelle reception de commande
const int ledPin = 2;  // pin de contrôle LED témoin 
 int tpsDelais = 0; // temps de délais en milliseconde(ici type unsigned long???) entre chaque unité de vitesse ajouté à 0 pas de délais
int vBoite;//vitesse reçu depuis mega en telecommande
int directionR;
 int vM1; //Vitesse et sens de rotation du moteur 1 max = -400 et +400
 int vM2;
 int oldvM1=0;
 int oldvM2=0;
 unsigned long oldMillisM1 = 0;        //temps à la dernière modification de M1 qd acceleration au dceceleration
 unsigned long oldMillisM2 = 0;        //temps à la dernière modification de M2 qd var vitesse progressive
//variables  de parcage
char messageFromPC[numChars] = {0};
int codeAction = 0;//au chargement option moteur à zero
float floatFromPC = 0.0;//pas utile pour l'instant
long timer; //suivi du temps
const long TIME_OUT = 1000; //Voir si pbtemps - latence en ms sans commande avant arrêt moteurs et temps max pour var vitesse

/**********Test fonctionnement moteur*************/
void stopIfFault()
{
  if (md.getM1Fault())
  {
    Serial.println("M1 fault");
    while(1);
  }
  if (md.getM2Fault())
  {
    Serial.println("M2 fault");
    while(1);
  }
}

/********************************Initialisation***********************************/
void setup()
{
  Serial.begin(9600);// ? ou 9600 - 115200 ou intermédiaire
  Serial.println("Dual VNH5019 Motor Shield");
  pinMode(ledPin, OUTPUT);  
  md.init();
  oldvM1= oldvM2=vM1=vM2=directionR=vBoite=0;//initialiser les moteurs
}

/*Loop - Boucle gestion moteur - Reception des données codées du type <xx,vdm1,vdm2> exple : <xx,-200,-400> : ici marque de début puis commande neutre xx + , + vitesse moteur1 et direction (- arrière),... - Parçage  -Switch commande reçu - Renvoie vers commande spécifique
********/

void loop(){
    recvWithStartEndMarkers();
    if (newData == true) {//la boucle attend en continue de nouvelles données si elle en reçoit elle s'execute
        strcpy(tempChars, receivedChars);// copie receivedChars dans tempChars pour protéger l'original
         //   because strtok() used in parseData() replaces the commas with \0
        parseData();//lecture du code reçu du type <x,x,x>
        Serial.print("receptionUno - Action: ");Serial.print(codeAction); Serial.print("  vBoite : ");Serial.print(vBoite);Serial.print("  directionR : ");Serial.println(directionR);
        conversionV();//conversion en vitesse moteur
        //installer sous programme pour traiter le cas où pas de modif des variables de vitesses)
        showParsedData();//commentaire pour test
        //transcription en commande moteur incluant détermination de l'accélération delais
        //analyseCode inclus l'envoi des données de vitesse finale quand tpsDelais =0
        analyseCode();
        Serial.print("tpsDelais : ");Serial.println(tpsDelais);
         if (tpsDelais != 0 && ((oldvM1 != vM1) || (oldvM2 != vM2))){acceleration(tpsDelais,oldvM1,oldvM2,vM1,vM2);// va en fonction des parametre atteindre la vitesse dans un temps x 
         }
         md.setSpeeds(vM1,vM2);
      }//end du newData
      //
      oldvM1=vM1;//premet de conserver vM1 précédent pour calculer les accelerations en fonction delta ds boucle suivante
      oldvM2=vM2;
      newData = false;//sortie de boucle

  
     /*If the robot does not receive any command, stop it
      if (newData==false && (millis() - timer >= TIME_OUT)) {
      setSpeeds(0,0);
      isActing = false;*/
    
}//end loop

/*******************Fonctions**************/

/***Reception des commandes************/
 void recvWithStartEndMarkers() {  
  static boolean recvInProgress = false;  //static signifie que la variable est conservée entre deux appel de la fonction
  static byte ndx = 0;  
  char startMarker = '<';  
  char endMarker = '>';  
  char rc;  //stockage d'un caractère reçu
  while (Serial.available() > 0 && newData == false) {  //il faudra une condition pour identifier la source info Mega ou rpi directe
      rc = Serial.read();  

      if (recvInProgress == true) {  //reception en cours
            if (rc != endMarker) {  
                receivedChars[ndx] = rc;  //rc caractere recu - receivedChars tableau de valeur ascii
                Serial.print("caractère reçu : "); Serial.print(rc); Serial.print("-Nbre : "); Serial.println(ndx);
                ndx++;  
                if (ndx >= numChars) {ndx = numChars - 1;}  //gestion dépassement mémoire ?
                }  
             else {  //marqueur de fin atteind, contrôle sortie du While indication attente de nouvelle valeur via newData
                receivedChars[ndx] = '\0'; // terminate the string  et préparation pour le stockage d'une nouvelle commande
                recvInProgress = false;  
                ndx = 0;  
                newData = true;  // sortie du recvInProgress et du while
                }  
          } //fin du cas recvInProgress 
       else if (rc == startMarker) { recvInProgress = true;} // Reception d'une nvelle commande avant la fin de la précédente reste dans le While pas d'ini ndx? zz
  }  //end While
 }

/***********Parcage des données multiples*********/
void parseData() {      // split the data into its parts

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    codeAction = atoi(strtokIndx);  //strcpy(codeAction, strtokIndx); // copy it to codeAction A0.... ou autre, première virgule comme repère
 
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    vBoite = atoi(strtokIndx);     // convert this part to an integer
    //vM1 remplace
    strtokIndx = strtok(NULL, ",");
    directionR = atoi(strtokIndx);     //Pour  convertir en float remplacer atoi par atof
    //vM2 remplace
}

/************analyseCode*****************/
void analyseCode(){
        
        switch (codeAction){  //traite l'option d'acceleration ou autre action si d'autres commandes ajouter à la place de A...
         case  0: //arret brutal
          md.setSpeeds(0,0);
          break;
         case 1:
          tpsDelais=16; // acceleration faible - tps en milliseconde
          break;         
         case 2:
           tpsDelais=4;   //acceleration moyenne - tps en milliseconde
           break; 
         case 3:
           tpsDelais=2;   //acceleration rapide - tps en milliseconde
           break; 
         case 4:     //vitesse max direct - tps en milliseconde - mettre Am et pas AM
           tpsDelais=0;
           md.setSpeeds(vM1,vM2);
         default:       //acceleration moyenne
          tpsDelais=5;
          break;       
          }
        }

 /************************conversionV******************/
 void conversionV(){
    //lié à codage tablette directionR vboite
     int vprov = 40*vBoite;
     int vprovDxt = (2*vprov)/3;
     int vprovUnt = vprov/3;
     Serial.print("vprov z dxt z Unt: ");Serial.print(vprov);Serial.print(vprovDxt);Serial.print(vprovUnt); 
     switch (directionR){
    case 0:
       vM1=vM2=0;
       break; 
    case 1: //avant
       vM1=vM2= vprov;//pour conversion en valeur sur 360 (on ne va pas au max de 400)
       break;
    case 2: // avant droit
       vM1=vprov;
       vM2=vprovUnt;
       break;
     case 3: // droite
       vM1=vprovDxt;
       vM2=0;
       break;
     case 4: // arrière droit
       vM1= -vprovDxt;
       vM2= -vprovUnt;
       break;
     case 5: // arrière
       vM1= vM2 = -vprovDxt;
       break;
     case 6: // arrière gauche
       vM1= -vprovUnt;
       vM2= -vprovDxt;
       break; 
     case 7: // gauche
       vM1= 0;
       vM2= vprovDxt;
       break;        
     case 8: // avant gauche
       vM1=vprovDxt;
       vM2= vprov;   
      }
 }
 /**********Acceleration*************** fonction setBrakes(int m1Brake, int m2Brake) pour freiner pas utilisé ici**/

void acceleration(int tempo,int oldm1f, int oldm2f, int m1f,int m2f)//la valeur de ses var locale a été prise qd appel de la fonction en argument à partir de var globale
{
   int varOldNewM1;//var locale différence absolue entre vitesse précédente et vitesse à atteindre pour le moteur 1
   int varOldNewM2;
   int accelereM1; //1 ou -1 :var accelere ou ralenti determine si boucle incrementation ou decrementation
   int accelereM2;
   
   if (m1f==0 && m2f==00){//cas moteur mis à zero par utilisateur inutile d'aller plus loin
   Serial.println("Moteur arrêté...Pas d'accélération"); 
   }
  //déterminer si accelere ou pas et calcul différence moteur 
   else if (m1f >= oldm1f && oldm1f >= 0){varOldNewM1 = m1f-oldm1f; accelereM1=1;}
   else if (m1f >= oldm1f && oldm1f <= 0 && m1f >= 0){varOldNewM1 = m1f-oldm1f; accelereM1=1;}
   else if (m1f >= oldm1f && oldm1f <= 0 && m1f <= 0){varOldNewM1 = m1f-oldm1f; accelereM1=1;}   
   else if (m1f <= oldm1f && oldm1f >= 0 && m1f >= 0){varOldNewM1 = oldm1f-m1f; accelereM1=-1;}
   else if (m1f <= oldm1f && oldm1f >= 0 && m1f <= 0){varOldNewM1 = oldm1f-m1f; accelereM1=-1;}  
   else if (m1f <= oldm1f && oldm1f <= 0 && m1f <= 0){varOldNewM1 = oldm1f-m1f; accelereM1=-1;}   

   else if (m2f >= oldm2f && oldm2f >= 0){varOldNewM2 = m2f-oldm2f; accelereM2=1;}
   else if (m2f >= oldm2f && oldm2f <= 0 && m2f >= 0){varOldNewM2 = m2f-oldm2f; accelereM2=1;}
   else if (m2f >= oldm2f && oldm2f <= 0 && m2f <= 0){varOldNewM2 = m2f-oldm2f; accelereM2=1;}   
   else if (m2f <= oldm2f && oldm2f >= 0 && m2f >= 0){varOldNewM2 = oldm2f-m2f; accelereM2=-1;}
   else if (m2f <= oldm2f && oldm2f >= 0 && m2f <= 0){varOldNewM2 = oldm2f-m2f; accelereM2=-1;}  
   else if (m2f <= oldm2f && oldm2f <= 0 && m2f <= 0){varOldNewM2 = oldm2f-m2f; accelereM2=-1;}   
  
   else {Serial.println("cas non prévu dans calcul varoldNew");}

  //Initialisation temps 
    unsigned long currentMillis = millis();
    int oldMillis = currentMillis;

   /*Pour l'arriver en même temps des deux moteurs à la vitesse finale de chacun
    on garde le delais fixer pour le changement d'une unité de vitesse  
     de delui ayant le plus de variation de vitesse et on calcul en conséquence 
     l'implémentation de vitesse du plus lent (sera inférieur à 1*/

     // cas 1 var vitesse M1 >= à M2 
      if (varOldNewM1>=varOldNewM2){   //M1 sup ou = M2
         if ((accelereM1 == accelereM2) && (accelereM1>0)){ //accelere si increment decrement vitesse si marche arrière ralenti  et passe en marche avant accelere
            oldMillis = currentMillis;
            for (int i = oldm1f; i <= m1f; i++){ //v avec for...do ...while lourd! boucle en continue
                  do{
                      int j = oldm2f+(varOldNewM2/varOldNewM1);
                      //desactivepourtest stopIfFault();
                       md.setSpeeds(i,j); //à la fin égal 
                       Serial.print ("M1>M2 et les deux positifs");
                       Serial.print(i);
                       Serial.print("#");
                       Serial.println(j);
                       currentMillis = millis();//avance le temps dans la boucle do while 
                      }while (currentMillis - oldMillis >= tempo);//syntaxe controle ok, que la condition après le while dans le cas d'un do{}while()
                  oldMillis = currentMillis = millis();//réinitialise var de temps avant prochaine boucle for
                } //end for
            } //end if accelere  

          if ((accelereM1 == accelereM2) && (accelereM1<0)){ //si marche avant ralenti et passe en marche arrière, si en arrière accelere marche arriere
            oldMillis = currentMillis;
            for (int i = oldm1f; i <= m1f; i--){ //v avec for...do ...while lourd! boucle en continue
                  do{
                      int j = oldm2f-(varOldNewM2/varOldNewM1);
                       //desactivpourteststopIfFault();
                       md.setSpeeds(i,j); //à la fin égal 
                       currentMillis = millis();//avance le temps dans la boucle do while 
                      }while (currentMillis - oldMillis >= tempo);//syntaxe controle ok, que la condition après le while dans le cas d'un do{}while()
                  oldMillis = currentMillis = millis();//réinitialise var de temps avant prochaine boucle for
                } //end for
            } //end if accelere  
       }//end cas 1

    // cas 2 var vitesse M2 >= à M1 
      if (varOldNewM2>=varOldNewM1){   //M1 sup ou = M2
          if ((accelereM2 == accelereM1) && (accelereM2>0)){ //accelere si increment decrement vitesse si marche arrière ralenti  et passe en marche avant accelere
            oldMillis = currentMillis;
            for (int j = oldm2f; j <= m2f; j++){ //v avec for...do ...while lourd! boucle en continue
                  do{
                      int i = oldm1f+(varOldNewM1/varOldNewM2);
                       //desactivpourteststopIfFault();
                       md.setSpeeds(i,j); //à la fin égal 
                       currentMillis = millis();//avance le temps dans la boucle do while 
                      }while (currentMillis - oldMillis >= tempo);//syntaxe controle ok, que la condition après le while dans le cas d'un do{}while()
                  oldMillis = currentMillis = millis();//réinitialise var de temps avant prochaine boucle for
                } //end for
            } //end if accelere  

          if ((accelereM2 == accelereM1) && (accelereM2<0)){ //si marche avant ralenti et passe en marche arrière, si en arrière accelere marche arriere
            oldMillis = currentMillis;
            for (int j = oldm2f; j <= m2f; j--){ //v avec for...do ...while lourd! boucle en continue
                  do{
                      int i = oldm1f-(varOldNewM1/varOldNewM2);
                       //desactivpourteststopIfFault();
                       md.setSpeeds(i,j); //à la fin égal 
                       currentMillis = millis();//avance le temps dans la boucle do while 
                      }while (currentMillis - oldMillis >= tempo);//syntaxe controle ok, que la condition après le while dans le cas d'un do{}while()
                  oldMillis = currentMillis = millis();//réinitialise var de temps avant prochaine boucle for
                } //end for
            } //end if accelere  
       }//end cas 2
//injecter dans les variables globales de vitesse vM1 et vM2 ? OU PAS...
}//fin de la fonction

   /*****affichage des resultats*******/
  void showParsedData() {
    Serial.print("Code de l'action ");
    Serial.println(codeAction);
    Serial.print("Vitesse et direction M1 ");
    Serial.println(vM1);
    Serial.print("Vitesse et direction M2 ");
    Serial.println(vM2);
  }
