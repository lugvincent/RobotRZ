#!/usr/bin/env python3
# =============================================================================
# SCRIPT: mqtt_bridge.py
# DESCRIPTION: Pont entre le port Série (Arduino) et MQTT.
#              Lit les données Série, les parse, et les publie sur MQTT.
#              Python/Batch -> gère erreurs, parsing avancé/json décode, 
#                              MQTT native/reconnexion, extenssivité,
#                              vitesse non problèmatique /échange
# DEPENDENCIES: pyserial, paho-mqtt
# USAGE: python3 mqtt_bridge.py
# VERSION: 1.0 2024-06-15 -Vincent / RobotZ
# =============================================================================

import serial
import paho.mqtt.client as mqtt
import json
import logging
import os
from time import sleep

# --- Configuration ------------------------------------------------------------
SERIAL_PORT = "/dev/ttyACM0"  # Port Série Arduino
BAUD_RATE = 115200             # Doit correspondre à config_serial.h
MQTT_BROKER = "robotz-vincent.duckdns.org"
MQTT_PORT = 1883
MQTT_TOPIC_OUT = "SE/arduino/data"  # Topic pour publier les données Arduino
MQTT_TOPIC_IN = "SE/arduino/commande"  # Topic pour recevoir les commandes

# Configuration
CONFIG_DIR = os.path.dirname(os.path.realpath(__file__)) + "/../config"

# Configure le logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("/data/data/com.termux/files/home/robotRZ-smartSE/termux/logs/mqtt_bridge.log"),
        logging.StreamHandler()
    ]
)

# --- Callbacks MQTT -----------------------------------------------------------
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        logging.info("Connecté au broker MQTT")
        client.subscribe(MQTT_TOPIC_IN)
    else:
        logging.error(f"Échec de connexion MQTT (code {rc})")

def on_message(client, userdata, msg):
    logging.info(f"Commande reçue: {msg.payload.decode()}")
    # Envoie la commande à Arduino via Série
    ser.write(msg.payload)

# --- Boucle Principale --------------------------------------------------------
def main():
    # Initialise la connexion Série
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        logging.info(f"Port Série ouvert: {SERIAL_PORT}@{BAUD_RATE}")
    except serial.SerialException as e:
        logging.error(f"Erreur port Série: {e}")
        return

    # Initialise le client MQTT
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        client.loop_start()
    except Exception as e:
        logging.error(f"Erreur MQTT: {e}")
        ser.close()
        return

    # Boucle de lecture Série
    while True:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line:
                logging.info(f"Données Série: {line}")
                # Parse la ligne (ex: "1200,-450,9800" pour Gyro)
                try:
                    data = {"type": "Gyro", "values": [int(x) for x in line.split(",")]}
                    client.publish(MQTT_TOPIC_OUT, json.dumps(data))
                except ValueError as e:
                    logging.error(f"Erreur de parsing: {e}")
        except KeyboardInterrupt:
            break
        except Exception as e:
            logging.error(f"Erreur: {e}")
            sleep(1)

    # Nettoyage
    ser.close()
    client.loop_stop()
    client.disconnect()

if __name__ == "__main__":
    main()
