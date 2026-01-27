/************************************************************
 * FILE : src/communication/vpiv_utils.h
 * ROLE : Fonctions utilitaires VPIV (parsing nombres, listes,
 *        booléens, RGB, Instances, etc.)
 ************************************************************/
#ifndef VPIV_UTILS_H
#define VPIV_UTILS_H

#include <Arduino.h>
#include <string.h>
#include <ctype.h>

/************ Vérifie si inst = "*" ************/
inline bool instIsAll(const char* s) {
    return (s && s[0]=='*' && s[1]=='\0');
}

/************ Convertit "true"/"1" / "false"/"0" ************/
inline bool parseBool(const char* s) {
    if (!s) return false;
    if (strcmp(s, "1") == 0) return true;
    if (strcasecmp(s, "true") == 0) return true;
    return false;
}

/************ Convertit une chaîne en entier avec défaut ************/
inline long parseIntOrDefault(const char* s, long def = 0) {
    if (!s) return def;
    return atol(s);       // atol retourne 0 si chaîne invalide
}

/************ Parse "R,G,B" vers 3 entiers ************/
bool parseRGB(const char* s, int out[3]);

/************ Parse liste d'indices "[0,2,4]" ou simple "3" ************/
int parseIndexList(const char* s, int* out, int maxCount);

#endif
