#!/usr/bin/env python3
# =============================================================================
# SCRIPT  : mqtt_bridge.py
# CHEMIN  : ~/robotRZ-smartSE/termux/scripts/core/mqtt_bridge.py
#
# OBJECTIF
# --------
# Pont bidirectionnel entre le port Série (Arduino Mega) et MQTT.
# Relaie les trames VPIV Arduino → SP (via SE/arduino/data)
# et les commandes SP → Arduino (via SE/arduino).
#
# DESCRIPTION FONCTIONNELLE
# -------------------------
# 1. Lit les credentials MQTT depuis keys.json (même pattern que les scripts .sh)
# 2. Ouvre le port Série vers l'Arduino Mega
# 3. Écoute MQTT sur SE/arduino → transmet à Arduino via Série
# 4. Lit la Série Arduino → publie sur SE/arduino/data vers SP
#
# FORMAT VPIV SUR LE FIL SÉRIE
# ----------------------------
# L'Arduino envoie des trames VPIV complètes : $CAT:VAR:PROP:INST:VAL#
# Ce bridge les transmet telles quelles sur MQTT (pas de transformation).
# Note v1 : si la ligne Série n'est pas au format VPIV, elle est encapsulée
# dans $I:COM:debug:A:<ligne># pour ne pas perdre l'information.
#
# TOPICS MQTT
# -----------
#   SE/arduino/data    : Arduino → SP (publication)
#   SE/arduino         : SP → Arduino (souscription)
#
# ARTICULATION
# ------------
#   Arduino Mega (Série USB-C)
#     ↕ (VPIV texte)
#   mqtt_bridge.py
#     ↕ (MQTT)
#   Broker MQTT → SP Node-RED
#
# PRÉCAUTIONS
# -----------
# - keys.json doit contenir MQTT_USER, MQTT_PASS, MQTT_HOST
# - SERIAL_PORT doit correspondre au port USB-C Arduino (/dev/ttyACM0 ou /dev/ttyUSB0)
# - Reconnexion automatique MQTT en cas de coupure réseau
# - Les erreurs Série non critiques sont loguées sans arrêt du bridge
#
# DÉPENDANCES
# -----------
#   pyserial  : pip install pyserial
#   paho-mqtt : pip install paho-mqtt
#
# AUTEUR  : Vincent Philippe
# VERSION : 2.0  (credentials keys.json — transmission VPIV native)
# DATE    : 2026-04-27
# =============================================================================

import serial
import paho.mqtt.client as mqtt
import json
import logging
import os
import re
from time import sleep

# =============================================================================
# CONFIGURATION CHEMINS
# =============================================================================

BASE_DIR = "/data/data/com.termux/files/home/robotRZ-smartSE/termux"
KEYS_FILE = os.path.join(BASE_DIR, "config", "keys.json")
LOG_FILE  = os.path.join(BASE_DIR, "logs", "mqtt_bridge.log")

# Port Série Arduino (USB-C)
SERIAL_PORT = "/dev/ttyACM0"   # Vérifier avec : ls /dev/ttyACM*
BAUD_RATE   = 115200           # Doit correspondre à config.h Arduino

# Topics MQTT (doc_A1 / Table A)
MQTT_TOPIC_OUT = "SE/arduino/data"    # Arduino → SP
MQTT_TOPIC_IN  = "SE/arduino"         # SP → Arduino

# Regex validation trame VPIV
VPIV_REGEX = re.compile(r'^\$[VIFEA]:[^:]+:[^:]+:[^:]+:.*#$')

# =============================================================================
# LOGGING
# =============================================================================

os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [BRIDGE] %(levelname)s : %(message)s',
    handlers=[
        logging.FileHandler(LOG_FILE),
        logging.StreamHandler()
    ]
)

# =============================================================================
# LECTURE CREDENTIALS MQTT (keys.json — même pattern que les scripts .sh)
# =============================================================================

def load_mqtt_credentials():
    """
    Lit MQTT_USER, MQTT_PASS, MQTT_HOST depuis keys.json.
    Retourne (host, user, password) avec fallbacks si absent.
    """
    try:
        with open(KEYS_FILE, 'r') as f:
            keys = json.load(f)
        host = keys.get('MQTT_HOST', 'robotz-vincent.duckdns.org')
        user = keys.get('MQTT_USER', '')
        pwd  = keys.get('MQTT_PASS', '')
        logging.info(f"Credentials MQTT chargés depuis {KEYS_FILE}")
        return host, user, pwd
    except FileNotFoundError:
        logging.warning(f"keys.json absent ({KEYS_FILE}) — fallback sans credentials")
        return 'robotz-vincent.duckdns.org', '', ''
    except json.JSONDecodeError as e:
        logging.error(f"keys.json invalide : {e} — fallback sans credentials")
        return 'robotz-vincent.duckdns.org', '', ''

# =============================================================================
# VARIABLE GLOBALE — port Série (partagé entre callbacks)
# =============================================================================

ser = None   # Initialisé dans main()

# =============================================================================
# CALLBACKS MQTT
# =============================================================================

def on_connect(client, userdata, flags, rc):
    """Connexion au broker — souscription au topic commandes Arduino."""
    if rc == 0:
        logging.info("Connecté au broker MQTT")
        client.subscribe(MQTT_TOPIC_IN, qos=1)
        logging.info(f"Souscription : {MQTT_TOPIC_IN}")
    else:
        logging.error(f"Échec connexion MQTT (code {rc})")

def on_disconnect(client, userdata, rc):
    """Déconnexion — reconnexion automatique gérée par paho loop_start."""
    if rc != 0:
        logging.warning(f"Déconnexion inattendue MQTT (code {rc}) — reconnexion auto...")

def on_message(client, userdata, msg):
    """
    Réception commande SP → Arduino.
    Transmet la trame VPIV telle quelle sur le port Série.
    Format attendu : $CAT:VAR:PROP:INST:VAL# + newline
    """
    global ser
    if ser is None or not ser.is_open:
        logging.warning("Port Série fermé — commande Arduino ignorée")
        return
    try:
        trame = msg.payload.decode('utf-8').strip()
        logging.info(f"SP→Arduino : {trame}")
        # Transmission Série avec newline (Arduino attend \n comme fin de trame)
        ser.write((trame + '\n').encode('utf-8'))
    except Exception as e:
        logging.error(f"Erreur transmission Série : {e}")

# =============================================================================
# TRAITEMENT LIGNE SÉRIE
# =============================================================================

def process_serial_line(line, client):
    """
    Traite une ligne reçue de l'Arduino.

    Si la ligne est une trame VPIV valide → publie telle quelle sur SE/arduino/data.
    Sinon → encapsule dans $I:COM:debug:A:<ligne># pour ne pas perdre l'info.

    Note : en production, toutes les lignes Arduino devraient être des VPIV.
    L'encapsulation sert de filet de sécurité pour le debug.
    """
    if not line:
        return

    if VPIV_REGEX.match(line):
        # Trame VPIV native Arduino → transmission directe
        logging.info(f"Arduino→SP : {line}")
        client.publish(MQTT_TOPIC_OUT, line, qos=1)
    else:
        # Ligne non-VPIV (debug Arduino, printf...) → encapsulation
        encapsule = f"$I:COM:debug:A:{line}#"
        logging.debug(f"Arduino (non-VPIV) encapsulé : {encapsule}")
        client.publish(MQTT_TOPIC_OUT, encapsule, qos=0)

# =============================================================================
# BOUCLE PRINCIPALE
# =============================================================================

def main():
    global ser

    logging.info("=" * 50)
    logging.info("Démarrage mqtt_bridge.py v2.0")
    logging.info("=" * 50)

    # --- Credentials MQTT ---
    mqtt_host, mqtt_user, mqtt_pass = load_mqtt_credentials()

    # --- Connexion Série Arduino ---
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        logging.info(f"Port Série ouvert : {SERIAL_PORT} @ {BAUD_RATE} baud")
    except serial.SerialException as e:
        logging.error(f"Impossible d'ouvrir le port Série {SERIAL_PORT} : {e}")
        logging.error("Vérifier connexion USB-C Arduino. Arrêt.")
        return

    # --- Client MQTT ---
    client = mqtt.Client(client_id="rz-mqtt-bridge")
    client.on_connect    = on_connect
    client.on_disconnect = on_disconnect
    client.on_message    = on_message

    # Credentials si définis
    if mqtt_user:
        client.username_pw_set(mqtt_user, mqtt_pass)

    # Reconnexion automatique
    client.reconnect_delay_set(min_delay=1, max_delay=30)

    try:
        client.connect(mqtt_host, 1883, keepalive=60)
        client.loop_start()
        logging.info(f"Connexion MQTT : {mqtt_host}:1883")
    except Exception as e:
        logging.error(f"Erreur connexion MQTT : {e}")
        ser.close()
        return

    # --- Boucle lecture Série ---
    logging.info("Bridge actif — lecture Série en cours...")
    try:
        while True:
            try:
                raw = ser.readline()
                if raw:
                    line = raw.decode('utf-8', errors='replace').strip()
                    if line:
                        process_serial_line(line, client)
            except serial.SerialException as e:
                logging.error(f"Erreur Série : {e} — tentative de réouverture...")
                sleep(2)
                try:
                    ser.close()
                    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
                    logging.info("Port Série réouvert")
                except Exception as e2:
                    logging.error(f"Réouverture Série échouée : {e2}")
                    sleep(5)
            except Exception as e:
                logging.error(f"Erreur inattendue : {e}")
                sleep(1)

    except KeyboardInterrupt:
        logging.info("Arrêt demandé (Ctrl+C)")

    finally:
        logging.info("Nettoyage et fermeture...")
        if ser and ser.is_open:
            ser.close()
        client.loop_stop()
        client.disconnect()
        logging.info("mqtt_bridge.py arrêté proprement.")

if __name__ == "__main__":
    main()