# Fiche Rapide A1 — Architecture et Acteurs du projet RZ

**Projet :** RZ — Robot compagnon autonome  
**Auteur :** Vincent Philippe | **Version :** 1.0 — Mars 2026

---

## Le projet RZ en un coup d'œil

RZ est un robot compagnon mobile piloté à distance ou de façon autonome. Il est composé de 4 composants matériels qui communiquent via le protocole VPIV transporté par MQTT.

```
[SP — Ordinateur]          [SE — Smartphone embarqué]
  Node-RED                   Termux + Scripts Bash/Python
  Superviseur maître  ──────► Cerveau local mobile
       │                           │
       │   MQTT / VPIV             │ USB-C
       │   (Internet)              ▼
       └──────────────────  [A — Arduino Mega]
                               Capteurs + Actionneurs
                                    │
                                    │ Série/I2C
                                    ▼
                            [Arduino UNO]
                            Moteurs uniquement
```

---

## Les 4 composants matériels

### SP — Système Pilote (ordinateur fixe)
- **Technologie :** Node-RED (JavaScript)
- **Rôle :** Superviseur maître absolu. Décide de tous les modes, envoie la configuration, fournit l'interface utilisateur, traite les alertes urgentes.
- **Responsabilités exclusives :** configuration initiale (courant_init.json), validation des intents STT vocaux, historisation des données.

### SE — Smartphone embarqué (sur la plateforme)
- **Technologie :** Android + Termux (Linux), Tasker, PocketSphinx, IP Webcam
- **Rôle :** Cerveau local. Traite les commandes de SP, gère les capteurs smartphone, exécute la reconnaissance vocale, relaie vers Arduino.
- **Contrainte :** Pas de SIM propre — connexion via Wi-Fi (box ou partage 4G).

### A — Arduino Mega (sur la plateforme)
- **Technologie :** C++ / firmware PlatformIO
- **Rôle :** Contrôleur temps-réel des capteurs et actionneurs physiques.
- **Capteurs gérés :** gyroscope, magnétomètre, 9 capteurs ultrasons, 3 microphones, détecteur IR, capteur de force.
- **Actionneurs gérés :** 2 LED (anneau Lring + ruban Lrub), 3 servomoteurs, sécurité mouvement.

### Arduino UNO (esclave moteur)
- **Rôle :** Exclusivement dédié au pilotage PWM des moteurs. Reçoit ses ordres de l'Arduino Mega. Invisible depuis SE et SP.

---

## Les applications sur SE

| Application | Type | Rôle |
|-------------|------|------|
| **Termux** | Linux sur Android | Exécute tous les scripts Bash et Python |
| **Tasker** | Automatisation Android | Gère les apps Android (Zoom, GPS, Baby...) |
| **IP Webcam** | Streaming | Flux vidéo HTTP depuis la caméra smartphone |
| **PocketSphinx** | Reconnaissance vocale | Wake word "rz" + commandes hors-ligne |
| **Gemini API** | IA cloud | Conversation libre (repli si commande inconnue) |
| **Termux:API** | Passerelle Android | TTS, torche, capteurs, Tasker depuis Bash |

---

## Les 3 couches logiques

| Couche | Sur SP | Sur SE | Sur Arduino |
|--------|--------|--------|-------------|
| **Système** | Node-RED, MQTT | Android, Termux | Firmware C++ |
| **Applicative** | Flux JavaScript | Managers Bash/Python, STT | Modules C++ |
| **Protocolaire** | VPIV/MQTT | VPIV/MQTT + FIFO | VPIV/Série |

---

## Tableau des responsabilités

| Qui | Configure | Décide | Exécute | Publie |
|-----|-----------|--------|---------|--------|
| SP | Tout (envoie courant_init.json) | Modes, urgences, validation | Interface | Commandes MQTT |
| SE | *_runtime.json depuis SP | Réponses locales, TTS | Managers, STT | Statuts MQTT |
| Arduino Mega | Depuis SP via SE | Sécurité hardware | Capteurs, actionneurs | Données série |
| Tasker | Tâches Android | Aucune | Apps Android | — |

---

## Réseau et connectivité

| Contexte | Connexion SE | Broker MQTT |
|----------|-------------|-------------|
| Domicile | Box Wi-Fi | robotz-vincent.duckdns.org:1883 |
| Déplacement | Partage connexion smartphone tiers | idem via 4G/5G |
| Développement | Wi-Fi local | idem ou broker local |

Topics MQTT principaux :
- `SE/commande` — SP vers SE (commandes entrantes)
- `SE/statut` — SE vers SP (états, ACKs)
- `SE/arduino/data` — Arduino vers SP (données capteurs via bridge)

---

*Fiche A1 — Architecture et Acteurs — Projet RZ v1.0 — Mars 2026 — Vincent Philippe*
