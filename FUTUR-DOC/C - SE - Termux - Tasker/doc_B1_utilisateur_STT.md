# Guide Utilisateur — Parler à RZ

**Projet :** RZ — Robot compagnon autonome  
**Auteur :** Vincent Philippe  
**Version :** 1.0 — Mars 2026  
**Public :** Utilisateurs du robot RZ, toute personne souhaitant le piloter par la voix

---

## Comment parler à RZ ?

RZ comprend les commandes vocales en français. Il écoute en permanence le mot de réveil **"RZ"**, puis la commande que vous lui donnez.

**Exemple :**
> Vous : **"RZ avance"**  
> RZ répond : *"Je fonce !"* et se met en mouvement.

Pas besoin de parler fort — une voix normale à 1-2 mètres suffit.

---

## Le mot de réveil : "RZ"

Chaque commande commence toujours par dire **"RZ"**. C'est le signal qui dit au robot que vous vous adressez à lui. Sans ce mot, RZ n'écoute pas.

✅ **"RZ avance"**  
✅ **"RZ arrête"**  
✅ **"RZ comment tu vas ?"**  
❌ ~~"Avance"~~ (sans RZ, ignoré)

---

## Les commandes disponibles

### Déplacements

| Ce que vous dites | Ce que fait RZ |
|-------------------|----------------|
| RZ avance | Avance tout droit |
| RZ recule | Recule |
| RZ stop / arrête / freine | S'arrête |
| RZ lentement / doucement | Ralentit |
| RZ vite / accélère | Accélère |
| RZ tourne à droite | Tourne vers la droite |
| RZ tourne à gauche | Tourne vers la gauche |
| RZ urgence / arrête tout | Arrêt d'urgence immédiat |

> **Note :** Les commandes de déplacement ne fonctionnent que si RZ est en mode "déplacement" (voir section Modes).

---

### Modes de fonctionnement

| Ce que vous dites | Effet |
|-------------------|-------|
| RZ mode veille | Met RZ en veille (économie énergie) |
| RZ mode déplacement | Autorise les déplacements |
| RZ mode fixe | RZ reste en place |
| RZ mode autonome | RZ agit de façon autonome |
| RZ pilotage vocal | Active le pilotage vocal |
| RZ mode laisse | Active le suivi automatique |

---

### Tête et expressions

| Ce que vous dites | Effet |
|-------------------|-------|
| RZ lève la tête / regarde en haut | La tête se lève |
| RZ baisse la tête / regarde en bas | La tête s'abaisse |
| RZ regarde à droite | La tête tourne à droite |
| RZ regarde à gauche | La tête tourne à gauche |
| RZ regarde devant / tout droit | La tête revient au centre |
| RZ souris / fais un sourire | Expression sourire (LEDs) |
| RZ neutre / expression neutre | Expression neutre |
| RZ triste | Expression triste |
| RZ étonné / surpris | Expression étonnement |
| RZ en colère / fâché | Expression colère |

---

### Caméra

| Ce que vous dites | Effet |
|-------------------|-------|
| RZ démarre la caméra / caméra on | Active la caméra arrière |
| RZ arrête la caméra / caméra off | Coupe la caméra |
| RZ caméra avant | Bascule caméra frontale |
| RZ caméra arrière | Bascule caméra dos |
| RZ prends une photo / snapshot | Prend un cliché |

---

### LEDs

| Ce que vous dites | Effet |
|-------------------|-------|
| RZ allume les leds | Allume l'anneau LED |
| RZ éteins les leds | Éteint les LEDs |

---

### Applications

| Ce que vous dites | Effet |
|-------------------|-------|
| RZ lance zoom / démarre zoom | Ouvre Zoom (réunion vidéo) |
| RZ arrête zoom | Ferme Zoom |
| RZ surveillance bébé | Lance la surveillance bébé |
| RZ arrête baby | Arrête la surveillance bébé |
| RZ lance la navigation / gps | Lance la navigation GPS |
| RZ arrête la navigation | Ferme la navigation |

---

### Microphone

| Ce que vous dites | Effet |
|-------------------|-------|
| RZ active le micro | Active l'écoute sonore |
| RZ coupe le micro | Désactive le microphone |
| RZ écoute d'où vient le son | Localise la direction du son |
| RZ surveillance sonore | Active la surveillance sonore |

---

### Système

| Ce que vous dites | Effet |
|-------------------|-------|
| RZ quel est ton état / comment tu vas | RZ donne son état |
| RZ batterie / niveau de batterie | RZ indique sa charge batterie |
| RZ réinitialise / reset | Réinitialise le système |
| RZ efface l'urgence | Annule une alerte active |

---

### Enchaînements automatiques (macros)

| Ce que vous dites | Ce que fait RZ |
|-------------------|----------------|
| RZ bonjour / salut / coucou | Sourire LED + expression joyeuse |
| RZ pause / fais une pause | S'arrête + expression neutre |
| RZ dors / va dormir / mode nuit | S'arrête, éteint les LEDs, passe en veille |
| RZ réveille-toi / debout / active-toi | Passe en déplacement, allume les LEDs |

---

## Si RZ ne comprend pas

Si RZ ne reconnaît pas votre commande, il bascule automatiquement vers son intelligence artificielle. Vous pouvez alors lui parler librement :

> **"RZ, qu'est-ce que tu penses de la météo aujourd'hui ?"**  
> RZ répond avec une phrase courte.

> **"RZ, raconte-moi une blague."**  
> RZ improvise une réponse.

Dans ce mode conversation, RZ ne pilote plus ses moteurs — il discute simplement.

---

## Conseils pour être bien compris

- **Parlez clairement**, à vitesse normale — ni trop vite ni trop lentement
- **Attendez** que RZ finisse de répondre avant de donner la commande suivante
- **Évitez le bruit de fond** excessif (musique forte, vent)
- Si RZ ne réagit pas, **répétez** : "RZ avance" (il peut avoir raté le mot de réveil)
- Les commandes marchent mieux **sans "s'il te plaît"** ou autres mots supplémentaires

---

## Quand les commandes de déplacement ne marchent pas

Il y a des situations normales où RZ refuse de bouger :

- **En mode veille** : dites d'abord "RZ mode déplacement"
- **En mode laisse** : RZ suit la laisse, les commandes de déplacement vocal sont bloquées pour la sécurité
- **Urgence active** : dites "RZ efface l'urgence" pour débloquer

---

## Les réponses vocales de RZ

RZ répond toujours pour confirmer qu'il a bien reçu une commande. Si vous n'entendez pas de réponse, c'est peut-être que :
- Le mode de pilotage est réglé sur "écran" (sans retour vocal)
- Le volume du téléphone est coupé
- RZ est trop chargé (CPU) et a mis en pause le traitement vocal

---

*Guide utilisateur RZ — Version 1.0 — Mars 2026 — Vincent Philippe*
