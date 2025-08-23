// moteurs.h
#ifndef MOTEURS_H
#define MOTEURS_H

#include "config.h"
#include "communication.h"

// Constantes pour les moteurs
#define MAX_MOTEUR_PROPERTIES 3  // Vitesse, Accélération, Direction

// Variables globales
extern char transMoteurP[8];  // Commandes moteurs (format <V,A,D>)
extern char oldTransMoteurP[8];
extern char transMoteurAuto[20];

// Prototypes des fonctions
void initMoteurs();
void envoiUno();
void processMoteurMessage(const MessageVPIV &msg);

#endif
