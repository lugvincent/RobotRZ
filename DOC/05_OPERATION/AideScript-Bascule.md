bascule-nodeRed.sh  : 
⚙️ Installation et utilisation sur ton smartphone
Vérifie les prérequis
Tu dois avoir installé :
Termux
Termux:API (pour les notifications, ouverture de navigateur, etc.)
Termux:Widget (pour créer le bouton sur ton écran d’accueil)

Assure-toi que Node-RED est déjà installé dans Termux et fonctionne avec node-red
le script doit être installé dans les raccourcis : mkdir -p ~/.shortcuts passer au chmod 
+x ~/.shortcuts/bascule-nodeRed.sh

Créer le bouton sur l’écran d’accueil
Fais un appui long sur ton écran d’accueil Android.
Choisis Widgets.
Cherche Termux:Widget.
Sélectionne script direct et choisis bascule-nodeRed.sh.
Un bouton apparaît sur ton écran d’accueil (tu peux le renommer “Node-RED ON/OFF”).

Utilisation
Clique sur le bouton → Node-RED démarre + ouverture automatique de la page dashboard.

Une notification persistante apparaît avec :

STOP pour l’arrêter

Ouvrir pour relancer le dashboard.

Reclique sur le bouton → Node-RED s’arrête proprement.


📝 Résumé du fonctionnement
Ce script sert de bouton bascule ON/OFF pour Node-RED installé dans Termux sur ton smartphone.

Si Node-RED est arrêté → il démarre en arrière-plan avec nohup, garde le téléphone éveillé (wake-lock), affiche une notification persistante avec deux boutons :

STOP → arrête Node-RED et libère le CPU

Ouvrir → ouvre directement le dashboard Node-RED dans ton navigateur Android
→ en plus, ton navigateur Récupère automatiquement l’IP du smartphone pilote sur le WiFi.
Ex. si ton téléphone est 192.168.1.13, l’URL devient http://192.168.1.13:1880/dashboard/accueil
Depuis ton PC ou un autre smartphone connecté au WiFi → tu peux aussi taper http://192.168.1.xx:1880/dashboard/accueil et tu tombes sur le même Node-RED

Si Node-RED est déjà lancé → il est arrêté (pkill), le verrou CPU est libéré (wake-unlock), et la notification est supprimée.

En résumé : un seul bouton contrôle tout Node-RED comme une appli Android.

