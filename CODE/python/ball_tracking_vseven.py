# USAGE - 13.11.2022 09:11:31
# python ball_tracking.py --video ball_tracking_example.mp4
# python ball_tracking.py

# import the necessary packages /video
from collections import deque
from imutils.video import VideoStream
import numpy as np
import argparse
import cv2
import imutils
import time #gestion du temps, sleep,...
# ajouter sleep
from time import sleep

#Echange mqtt
import os
import sys #info systeme
import paho.mqtt.client as mqtt # import du client 1,  avec def d'un generique svent mqtt ou paho si on en choisis on le garde ds le reste du code
import paho.mqtt.publish as publish
import json

#Preparation MQTT - source testmqtt5.py ds dossier suiviobjet
#variante import socket pour recup IPlocal
#Serveur = socket.gethostbyname(socket.gethostname())#recup val serveur local
serveur = 'localhost'
port = 1883

print ("creating new instance\n")
client1 = mqtt.Client("P1")#creation d'une nouvelle instance client object (attention si plusieurs il faudra autant de boucle que de client

print ("connection au broker\n")
client1.connect(serveur, 1883, 60) #connection au broker ini 60 /3 jamais 0

#Creer des messages de retour (callback messages (voir one note comprendre les rappels) -
#Pour cela deux étapes : 
#	créer des fonctions  de rappel pour traiter tous les messages,
#	 démarrez une boucle pour vérifier les messages reçus 
def on_message(client, userdata, message): # si nom fction truc
    print("message received" ,str(message.payload.decode("utf-8")))
    print("message topic=",message.topic)
    print("message qos=",message.qos)
    print("message retain flag=",message.retain)
    
client1.on_message=on_message     #attach function to callback / si truc : client1.on_message=truc  

def on_publish(client,userdata,result):             #create function for callback
    print("data published \n")
    pass

client1.on_publish = on_publish                          #assign function to callback
client1.connect(serveur,port)                                 #establish connection / toujours avant client.loop_start
ret= client1.publish("suivi","on") #Comprendre que l'éval de la variable ret entraine l'exécution de la methode publish et donc la publication topic ici suivi avec val on - peut servir à demander verif camera fermée

def on_disconnect(client, userdata, rc):
   print("client disconnected ok")
   loggig.debug("Code de déconnexion "+str(rc))
   client.loop_stop()

client1.on_disconnect = on_disconnect
#client1.disconnect()

#VarD stock le delta de vitesse du moteur du robot
# exemple la boule se décale à gauche le moteur droit doit accelerer de x% et le gauche ralentir de -x% varD= x 
varD =  0.00 
varOldD= 0.00
varDeltaD=5.00 # Reglage arbitraire de la limite de variation nécessitant un envoi d'info
varLimiteD=50 # Réglage des limites max d'angles
varH = 0.00 # stock delta position verticale exploité par servomoteur verticale de la tete
varOldH= 0.00 
varDeltaH=2.00 # Reglage arbitraire de la limite de variation nécessitant un envoi d'info
varV = 0 # calculer à partir de la variation de l'aire du cercle qui change avec la distance
varOldV= 0
varDeltaV=20.00 # Reglage arbitraire de la limite de variation nécessitant un envoi d'info
#Var surface du cercle : la balle a une surface différente en fonction de la distance 
# le changement de surface signifie un rapprocement ou un éloignement qui nécessite une correction de vitesse
# pour calucler le delta on doit entre chaque boucle faire le différentiel avec la taille précédente
aire = 0 # A REGLER empiriquement pour balle de tennis à 1 m du robot autour de 340 pour l'aire avec limite proche 380 et limite loin 300 
oldAire = aire
transfert=0 # variable boolen pour enregistrer si modif des vars D H V suffisantes pour justifier transfert info
varMoteur={"varD" : 0.00 , "varH" : 0.00 , "varV" : 0 }#xboxJoyG={"JLX" : 0.00 , "JLY" : 0.00 , "CL" : 0 }
varOldMoteur={"varOldD" : 0.00 , "varOldH" : 0.00 , "varOldV" : 0 }#varD delta vitesse moteur D / direction - Var H delta servo-Moteur Haut-Bas et varV variation de vitesse des deux moteurs / changement diametre boule

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-v", "--video",
	help="path to the (optional) video file")
ap.add_argument("-b", "--buffer", type=int, default=64,
	help="max buffer size")
args = vars(ap.parse_args())

#REGLAGE "EMPIRIQUE" PEUT ETRE NECESSAIRE Définir les limites de la couleur suivi au format HSV ici pour balle de tennis
# define the lower and upper boundaries of the "green"
# ball in the HSV color space, then initialize the
# list of tracked points - 
greenLower = (29, 86, 6)
greenUpper = (64, 255, 255)
pts = deque(maxlen=args["buffer"])

# if a video path was not supplied, grab the reference
# to the webcam
if not args.get("video", False):
	vs = VideoStream(src=0).start()

# otherwise, grab a reference to the video file
else:
	vs = cv2.VideoCapture(args["video"])

# allow the camera or video file to warm up - ini 1 chge nbre de prise de vue non
time.sleep(3.0)

# Boucle de la reconnaissance de vue - keep looping
while True:
	# grab the current frame
	frame = vs.read()

	# handle the frame from VideoCapture or VideoStream
	frame = frame[1] if args.get("video", False) else frame

	# if we are viewing a video and we did not grab a frame,
	# then we have reached the end of the video
	if frame is None:
		break

	# resize the frame, blur it, and convert it to the HSV - pour resize soit cv2.resize ou imutils.resize() legerement différents
	# color space width original 600 (taille de la fenêtre image)
	frame = imutils.resize(frame, width=600)
	blurred = cv2.GaussianBlur(frame, (11, 11), 0)
	hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

	# construct a mask for the color "green", then perform
	# a series of dilations and erosions to remove any small
	# blobs left in the mask
	mask = cv2.inRange(hsv, greenLower, greenUpper)
	mask = cv2.erode(mask, None, iterations=2)
	mask = cv2.dilate(mask, None, iterations=2)

	# find contours in the mask and initialize the current
	# (x, y) center of the ball
	cnts = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL,
		cv2.CHAIN_APPROX_SIMPLE)
	cnts = imutils.grab_contours(cnts)
	center = None

	# Traitement uniquement si une forme a été délimitée
	#only proceed if at least one contour was found
	# Cas ou perte du contour forme
	if len(cnts) == 0:
		client1.publish("incrementPerteSuivi", 1)
	if len(cnts) > 0:
		# find the largest contour in the mask, then use
		# it to compute the minimum enclosing circle and
		# centroid
		c = max(cnts, key=cv2.contourArea)
		((x, y), radius) = cv2.minEnclosingCircle(c)
		M = cv2.moments(c)
		# x = int(Moments["m10"] / Moments["m00"]) et y = int(Moments["m01"] / Moments["m00"])
		center = (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))
		#aire = cv2.contourArea(c)
		aire=M["m00"]
		#REGLAGE PEUT ETRE NECESSAIRE / valeurs empiriques
		#zz Perso - analyse du deplacement voir OneNote Etalonnage visionCamera
		print(x, ", ", y, ",",aire) 
		if (x > 215) and (x < 375):# milieu camera x 300, y 225 avec 5% d'erreur (orientation gauche droite sens du robot avec x0 à gauche de camera) y en bas 440 en haut 0
			#continue - balle centree horizontalement - cas position au centre de la camera
			print ("même orientation" )
			
			# cas deplacement sur la gauche en restant dans le cadre- modifer d'un % vardg la vitesse des moteurs
		elif (x <= 215) and (x > 70): 
			# moteur droit diminue de 1/2 de la var exprime en % zz pour suite programme faire -varD pour moteur gauche 
			varD= round(((300-x)/6), 2) #(0.5 * ((x-300)/100)) # comme var max est 300/centre on divise par 3 et comme les deux moteurs vont être changé en vitesse on ne prend que moitié de la var pour le droit donc var div par 6                    
			print ("dérive à gauche : ", varD)
			#sleep(.5) ou sleep(.04) zz voir pour permettre reaction moteur après info à node-red
			
			# cas la boule par à droite du robot je ralenti le moteur droit  varD négatif
		elif (x >= 375) and (x < 530):
			varD= round(((300-x)/6),2) #old calcul pas bon 0.5 * ((300-x)/100)
			print ("dérive à droite : ", varD)# ici val positive signifie accelere sur moteur droit pour tourner vers gauche
			#sleep(.04)
			#sleep(.5)
			
			# cas deplacement sur la gauche potentiellement hors champ de la camera
		elif (x <= 100):
			varD= varLimiteD # delta à droite de (300-0)/6
			print ("dérive hors cadre à gauche : ", varD)
			#sleep(.05)
			print ("dérive hors cadre à gauche : ", varD)

			# cas deplacement sur la droite potentiellement hors champ
		elif (x >= 520):
			varD= -varLimiteD 
			print ("dérive hors cadre à droite : ", varD)
			#sleep(.05)
		else:
			continue
		
		# varV Déplacement dans l'axe rapprochement eloignement se traduit par changement de l'aire de la boule,
		# doit changer vitesse globla des moteurs
		# le relevé de mesure montre des variations en fonction de la lumière,  de l'angle qui rendent difficile une évaluation
		varV= aire
			#cas ou vitesse robot et balle proche
		#client1.publish("aire",aire)
		#if (aire > 330) and (aire < 350):# dans la limite approximative de 50 cm à 1m50 du robot la balle de tennis à une surface qui variie de 300 à 380
			#continue - balle centree verticalement - cas position au centre de la camera
			#print ("distance stable" )
			
			# cas ou balle s'éloigne
		#elif (aire <= 330) and (aire >= 280): # voir si suffisant pour ne pas perdre la balle
			# acceleration des moteurs 
			#varV= (340-aire)*2 # +- le % d'acceleration nécessaire pour suivre la boule                   
			#print ("distance balle augmente, augmentation vitesse: ", varV)
			
			#cas ou balle se rapproche
		#elif (aire<= 400) and (aire >= 350): 
			# ralentir le moteur la balle se rapproche trop 
			#varV= (340-aire)*2 # +- le % de val negative envoye au servomoteur pour monter                   
			#print ("distance balle diminue on diminue la vitesse : ", varV)
			
			# cas ou perte potentiel de la balle - genere une alerte
		#elif (aire> 400) or (aire < 280) :
			#varV= 0 # traitement dans node-red du cas particulier indiquant perte potentiel de la balle
			#print ("perte potentiel de la balle, varV mise à zero pour alerte")
		#else:
			#continue

		# varH Déplacement vertical de la balle et réponse servo moteur
			#cas ou le déplacement vertical est faible
		if (y > 200) and (y < 240):# milieu camera x 300, y 225 avec 5% d'erreur (orientation gauche droite sens du robot avec x0 à gauche de camera) y en bas 440 en haut 0
			#continue - balle centree verticalement - cas position au centre de la camera
			print ("même orientation verticale" )
			
			# cas deplacement balle vers le haut (y en haut 0 et 220 au milieu en bas 440
		elif (y <= 200) and (y > 10): 
			# déplacement vertical par servomoteur vertical de la tête 
			varH= round(((200-y)/4), 2) # +- le % de val positive envoye au servomoteur pour monter                   
			print ("balle va vers le haut : ", varH)
			#sleep(.5) ou sleep(.04) zz voir pour permettre reaction moteur après info à node-red
		elif (y <= 430) and (y > 240): 
			# déplacement vertical par servomoteur vertical de la tête 
			varH= round(((200-y)/4), 2) # +- le % de val positive envoye au servomoteur pour monter                   
			print ("balle va vers le bas : ", varH)
		else:
			continue
		
		#Transferer à node-red varD pour régler le moteur voir aussi pour inclure le diametre de la boule
		client1.loop_start()#start the loop - il faudra l'arrêter et lui donner le temps de traiter les rappels
		print("abonnement au sujet :","varD")
		client1.subscribe("varD")
		print("publication sur sujet :","varD")
		client1.publish("varD",varD) # comme le publish a été passé directement et non via l'actu d'une variable il s'affiche dans le schell
		#time.sleep(4)#Ini attendre le message de retour en fait du fait  des traitements ralentis tous le système
		client1.loop_stop()#stop the loop

		#Test si modif de position de la balle significatif / var Delta spécifique 
		if (varD>= 0 and varD>=(varOldMoteur['varOldD']+varDeltaD)): 
			transfert=1
		elif (varD<= 0 and varD<=(varOldMoteur['varOldD']-varDeltaD)): 
			transfert=1
		elif (varH>= 0 and varH>=(varOldMoteur['varOldH']+varDeltaH)): 
			transfert=1
		elif (varH<= 0 and varH<=(varOldMoteur['varOldH']-varDeltaH)): 
			transfert=1
		elif((varV>=(varOldMoteur['varOldV']+varDeltaV))or(varV<=(varOldMoteur['varOldV']-varDeltaV))): 
			transfert=1
		print("transfert :",transfert)
		if (transfert==1): #test - si une des variables c'est écarté de l'ancienne valeure significativement actualisation et transfert 
			varMoteur['varD']=varD # Direction GcheDroite
			varMoteur['varH']=varH # HautBas
			varMoteur['varV']=varV # aire
			#Actu des valeurs Old 
			varOldMoteur['varOldD']=varMoteur['varD']
			varOldMoteur['varOldH']=varMoteur['varH']
			varOldMoteur['varOldV']=varMoteur['varV']
			client1.publish('varsMoteurs', json.dumps(varMoteur),0)
		#/zz
		# only proceed if the radius meets a minimum size
		if radius > 10:
			# draw the circle and centroid on the frame,
			# then update the list of tracked points
			cv2.circle(frame, (int(x), int(y)), int(radius),
				(0, 255, 255), 2)
			cv2.circle(frame, center, 5, (0, 0, 255), -1)

	# update the points queue
	pts.appendleft(center)

	# loop over the set of tracked points
	for i in range(1, len(pts)):
		# if either of the tracked points are None, ignore
		# them
		if pts[i - 1] is None or pts[i] is None:
			continue

		# otherwise, compute the thickness of the line and
		# draw the connecting lines
		thickness = int(np.sqrt(args["buffer"] / float(i + 1)) * 2.5)
		cv2.line(frame, pts[i - 1], pts[i], (0, 0, 255), thickness)

	# show the frame to our screen -zz DOIT ETRE DESACTIVE POUR FONCTIONNEMENT DEPUIS NODE-RED l'appli Cam Web interface doit elle être arr^été  
	#cv2.imshow("Frame", frame)
	key = cv2.waitKey(1) & 0xFF

	# if the 'q' key is pressed, stop the loop
	if key == ord("q"):
		break

# if we are not using a video file, stop the camera video stream
if not args.get("video", False):
	vs.stop()

# otherwise, release the camera
else:
	vs.release()

# close all windows
cv2.destroyAllWindows()
