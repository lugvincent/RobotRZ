#A placer dans le dossier home/Xbox
# coding: utf-8
#prepare intro joystick
from __future__ import print_function
#sys peut être utile quand on utilise import
import sys
import xbox
#prepare MQTT et serialisation jason des infos joy
import paho.mqtt.client as mqtt
import json
import socket #pour recup IPlocal
import time
#time.sleep(5)
usleep = lambda x: time.sleep(x/1000000.0) #usleep(100) sleep during 100us

usleep(100) #sleep during 100us
#import os #print....
#zz import utile? : import sys import os, import time, import os,
#upload interval in seconds et var width temp

#Serveur synonyme de Broker ici - A modif si via html non local choose broker
#Informations de connexion au broker MQTT :  adresse IP, login, mot de passe a adapter aux besoins login = "login_a_moi"  pwd = "mon_password"
#broker = "192.168.1.40" - #login = "pi" - #pwd = "thLEga99?"
# on se connecte au broker - client connect adresse du broker, port, 3me arg duree de vie de la connexion max 0 pour desactiver
#client = mqtt.Client() - #client.username_pw_set(username=login,password=pwd)
#client.connect(broker, 1883, 2) - #client.publish(sujet, msg, qos=0, retain=False)
Serveur = socket.gethostbyname(socket.gethostname())#recup val serveur local
client = mqtt.Client()#creation d'une nouvelle instance
#using default MQTT port and 60 seconds keepalive interval
client.connect(Serveur, 1883, 60) #connection au broker ini 60 /3 jamais 0
 
#prepare souscription à un message qui sera fait en fin de boucle ici pour sortir de la boucle etfermer le cas échéant le script python
messageRecu="Rien"
def on_message(client, userdata, message):# on pouvait aussi recup qos et retain en demandant message.qos
    messageRecu=str(message.payload.decode("utf-8"))
    messageRTopic=message.topic
    print("messagereçu: ",messageRecu)#si egal à F "fermer" arrête le script; si C ouvre le script souris
    print("message topic: ", messageRTopic)
    if messageRecu=="F" :
        sys.exit()
        print ("Message fermer le script")
    elif messageRecu=="C" :
        
        print ("activation du script souris")
        
client.on_message=on_message        #attach function to callback - fction executée à reception d'un message

#Construction des lignes de data fournis par joystick pour le terminal mis ds la var show
# Format floating point number to string format -x.xxx
# fonction modif pour renvoie des données ds mqtt
def fmtFloat(n):
    return '{:5.2f}'.format(n)# :6.3f ini chger en fonction du codage des flow f virgule fixe, 6 lgueur champ 3 nbre chiffre apres virgule format(n) n ce qui sera ecrit
# Ini Print one or more values without a line feed changer en envoi MQTT
def show(*args):
    for arg in args:
        print(arg, end='')
# Print true or false value based on a boolean, without linefeed
def showIf(tbVar, boolean, ifTrue, ifFalse=' '):
    if boolean:
        show(ifTrue)
        tbVar[ifTrue]= 1 #permet d'entrer les vals boolean ds mqtt comme nbre zz voir si pb
        
    else:
        show(ifFalse)
        tbVar[ifTrue]= 0 #permet d'entrer les vals boolean ds mqtt
        
# Instantiate the controller avec message si erreur (exemple sortie du fichier sans touche select du joystick!) 
while True:
    try :
        joy = xbox.Joystick()
        break
    except:
        print('appuyer lguementsur le bouton central pour reinitialiser ou Enlever et remettre le dongle bluetooth losangique ')
        client.publish('xboxEtat','E' , qos=1, retain=False)# E pour attente - zz si bton colore mettre orange
        client.disconnect()
        exit('enlever et remettre  le dongle')
# procedure de coordination Node-red /Xbox
client.publish('xboxEtat','D' , qos=1, retain=False)# D pour attente - zz si bton colore mettre orange
while not joy.connected() :
     time.sleep( 0.5 )# attente 1s
print('appuyez sur le bouton central pour demarrer')
print('')
while not joy.Guide() :
    time.sleep( 0.5 )# attente 1s    

print('Joystick ok choisir le mode : Start A pour pilotage, Start B pour  souris RPI, Start X pour enregistrement de chemin')
print('')
client.publish('xboxEtat','C' , qos=1, retain=False)# C pour attente  de start et A ou B ou X
while not (joy.Start() and (joy.A() or joy.B() or joy.X())):
    time.sleep( 0.5 )# attente 1s 
    
xboxBouton={ "A" : 0 , "B" : 0 , "X" : 0 , "Y" : 0 ,"BL" : 0, "BR" : 0  }
showIf(xboxBouton, joy.A(), 'A')
showIf(xboxBouton, joy.B(), 'B')
showIf(xboxBouton, joy.X(), 'X')
if   xboxBouton['A']== 1 :
     client.publish('xboxEtat', 'A' , qos=1, retain=False)# def du type de fonctionnement A pour "pilotage"  (mode de fonctionnement transmis par mqtt ds Etat aussi et traite spécifiquement ds fction Node-red)
elif xboxBouton['B']== 1 :
     client.publish('xboxEtat', 'B' , qos=1, retain=False)# def du type de fonctionnement B pour "souris" de la RPI
     joy.Back()
else :
    client.publish('xboxEtat', 'X' , qos=1, retain=False)# def du mode de fonctionnement X pour "def de chemins" / deplacement du Robot via notamment le pad en croix

client.publish('xboxEtat','I' , qos=1, retain=False)# I pour ok en Info, on utilise pas de bton pour déclencher, on garde Y non défini pour déf d'état potentiellement utilisable
print('Joystick ok Y mode aussi, appuyer sur Select pour arrêter ')
print('')
time.sleep( 5 )
nv='N' # drapeau fction si / publish mqtt detail data utile ????

#codage des infos box a trois niveaux : 
    #info provenant du controleur xbox implementation fichier xbox.py sous racine
    #traitement dans le fichier courant dans la chaine de conditionnel pour messages mqtt transmis choix sujet particulier et payload associer
    #traitement dans node-red en fonction d'une var de contexte (objectif :  deplacement robot, enregistrement de chemin, deplacement ds menu smartphone, ds tbleau de bord node,...) 

#construction des variables de traitement Mqtt et ou reinitialisation y compris pour Bouton
#on pourrait faire dans la boucle des if après envoi info à node-red pour traiter en direct le résultat des boutons, choix actuel tout le traitement ds node-red 

xboxJoyG={"JLX" : 0.00 , "JLY" : 0.00 , "CL" : 0 }# pilotage joystick gauche et dependance CL centre joy / var incluant infos des cpteurs US (aide pilotage)
xboxOldJoyG={"JLX" : 0.00 , "JLY" : 0.00 , "CL" : 0 }#used to recup old var
xboxJoyD={"JRX" : 0.00 , "JRY" : 0.00 , "CR" : 0 }# pilotage joystick droit
xboxOldJoyD={"JLR" : 0.00 , "JLR" : 0.00 , "CR" : 0 }#used to recup old var
xboxCroix={"U" : 0 , "D" : 0 , "L" : 0 , "R" : 0   }#voir si reservation pour enregistrement de chemin - Bton start  en bouton bascule chgement etape
#Attention a l'utilisation des  boutons et gachette num du bas. doivent etre implementer en fonction var global contexte de node-red
xboxBouton={ "A" : 0 , "B" : 0 , "X" : 0 , "Y" : 0 ,"BL" : 0, "BR" : 0 ,"S" : 0 }#Bouton 1 qd pressed 0 sinon - BL et Br bton de reaffectation des boutons ABXY si pressed in same time, S changement d'option pilotage (retour choix d'options de fonctionnement
xboxTrigger={ "TL" : 0.00 , "TR" : 0.00 } #used en level or slider vers + ou - type scalaire 0 255 sur 2 octets ?
# BL et Br jamais seul  utiliser uniquement en couple avec autres commande pour modifier leurs usages

#client.connect(Serveur, 1883, 60) 
while not joy.Back(): # voir les changement sur les axes et boutons tant que le bouton Select=Back pas pressed
    usleep(50)#ini 5000 / reglage des doublons because tps sur bton manette ou delai pour traitement zz Av revoir
    client.connect(Serveur, 1883, 60)
    #print(nv) 
    #print(fmtFloat(joy.leftY()))
    
    # Left analog stick (joystick gauche)
    show('  Left X/Y:', fmtFloat(joy.leftX()), '/', fmtFloat(joy.leftY()))
    xboxJoyG['JLX'] = fmtFloat(joy.leftX())
    xboxJoyG['JLY'] = fmtFloat(joy.leftY())
    showIf(xboxJoyG, joy.leftThumbstick(),'CL')
    #if ( (fmtFloat(joy.leftX()) != fmtFloat(0.000)) or (fmtFloat(joy.leftY()) != fmtFloat(0.000) ) or (xboxJoyG['CL']== 1 ) ) : #declenche la transmission a mqtt par acti du if
    if((xboxJoyG['JLX'] !=xboxOldJoyG['JLX'] ) or  (xboxJoyG['JLY'] !=xboxOldJoyG['JLY'] ) or (xboxJoyG['CL'] !=xboxOldJoyG['CL'] )) :
    #xboxJoyG,xboxOldJoyG = json.dumps(xboxJoyG, sort_keys=True), json.dumps(xboxOldJoyG, sort_keys=True)
    #if ( xboxJoyG !=xboxOldJoyG) : #declenche la transmission a mqtt par acti du if
        xboxOldJoyG['JLX'] =xboxJoyG['JLX'] #actu
        xboxOldJoyG['JLY'] =xboxJoyG['JLY']
        xboxOldJoyG['CL']  =xboxJoyG['CL']
        client.publish('xboxJoyG', json.dumps(xboxJoyG),0)
        
     # Right analog stick -joystick de droite    # CL et CR Centre du joystick de gauche et de droite 0 ou 1 pressed - zz mettre cde alerte?
    show('  right X/Y:', fmtFloat(joy.rightX()), '/', fmtFloat(joy.rightY()))
    xboxJoyD['JRX'] = fmtFloat(joy.rightX())
    xboxJoyD['JRY'] = fmtFloat(joy.rightY())
    showIf(xboxJoyD, joy.rightThumbstick(),'CR') 
    if ((fmtFloat(joy.rightX()) != fmtFloat(0.000)) or (fmtFloat(joy.rightY()) != fmtFloat(0.000)) or (xboxJoyD['CR']== 1  )) : #declenche la transmission a mqtt par acti du if
        client.publish('xboxJoyD', json.dumps(xboxJoyD),0)

    # Croix directionnelle : Dpad U/D/L/R  - returns 1 (pressed) or 0 (not pressed) 
    showIf(xboxCroix, joy.dpadUp(),    "U")
    showIf(xboxCroix, joy.dpadDown(),  "D")
    showIf(xboxCroix, joy.dpadLeft(),  "L")
    showIf(xboxCroix, joy.dpadRight(), "R")
    if( xboxCroix['U']== 1 or xboxCroix['D']== 1 or xboxCroix['L']== 1 or xboxCroix['R']== 1 ):
        client.publish('xboxCroix', json.dumps(xboxCroix),0)
    
       
    # A/B/X/Y buttons- returns 1 (pressed) or 0 (not pressed) +
    #Gachetttes sup du fond bumperLR- val 0 ou 1 comme bton   à associer à un autre bouton pour changer de message
    show ('Buttons:')
    showIf(xboxBouton, joy.A(), 'A')
    showIf(xboxBouton, joy.B(), 'B')
    showIf(xboxBouton, joy.X(), 'X')
    showIf(xboxBouton, joy.Y(), 'Y')
    showIf(xboxBouton, joy.leftBumper(),'BL')
    showIf(xboxBouton, joy.rightBumper(),'BR')
    showIf(xboxBouton, joy.Start(), "S")# S+... changement de mode divers reprend option S+A pilote  réini, S+B bascule vers souris clos le python S+X croix ? à voir si utile
   
    if( xboxBouton['A']== 1 or xboxBouton['B']== 1 or xboxBouton['X']== 1 or xboxBouton['Y']== 1  or xboxBouton['BL']== 1 or xboxBouton['BR']== 1 or xboxBouton['S']== 1 ) :
        client.publish('xboxBouton', json.dumps(xboxBouton),0)
        #on pourrait faire des if après envoi info pour traiter en direct le résultat des boutons, choix actuel tout le traitement ds node-red    
    
    # Left trigger value scaled between 0.0 to 1.0 - gachette du dessous val 0 à 255
    show("  TL:", fmtFloat(joy.leftTrigger()))
    xboxTrigger["TL"]=fmtFloat(joy.leftTrigger())
    # Right trigger value scaled between 0.0 to 1.0 - gachette du dessous val 0 à 255
    show("  TR:", fmtFloat(joy.rightTrigger()))
    xboxTrigger["TR"]=fmtFloat(joy.rightTrigger())    
    if (fmtFloat(joy.leftTrigger()) != fmtFloat(0.000)) or (fmtFloat(joy.rightTrigger()) != fmtFloat(0.000)) : 
         client.publish('xboxTrigger', json.dumps(xboxTrigger),0)
       
    #Envoi MQTT      # publication du json sur mqtt  si chge- ici val_xbox est le topic et json.dumps(sensor_data) le payload,  0 correspond à qos
    # Move cursor back to start of line
    show(chr(13)) # ajoute le caractère retour à la ligne
    client.loop_start() #start the loop
    client.subscribe ("ctrlPy");#la fonction  "on message" defini plus ht est executée
    time.sleep(1) # wait
    client.loop_stop() #stop the loop
    

#Sortie de la boucle de traitement des cas ordinaires (traitement differents pour :
# G guide bton central pour ouvrir com xbox manette 0 ou 1
# S bton Start 0 ou 1 (non used or used for stop action courante ou reactiver action courante light bascule)
# C close bton select sur Xbox qui ferme la boucle While de traitement des datas de la xbox
   
# Close out when done
joy.close()
print("Fermeture réussit")
#Prevenir RZ de la fermeture de la manette & fermeture du publish
client.publish('xboxEtat', 'N')
client.disconnect()
