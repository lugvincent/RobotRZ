// vpiv_utils.h
// Fonctions utilitaires pour parser des listes et valeurs simples.
// Conçu pour l'environnement Arduino (pas de heap dynamique lourd).
#ifndef VPIV_UTILS_H
#define VPIV_UTILS_H

#include <Arduino.h>

// Parse une chaîne d'indices au format "[0,1,2]" -> écrit les indices dans outIndices (taille max) et retourne le nombre.
// Retourne -1 en cas d'erreur.
int parseIndexList(const char* s, int* outIndices, int maxCount);

// Parse un triplet RGB "R,G,B" -> remplit out[3], retourne true si ok.
bool parseRGB(const char* s, int out[3]);

// Vérifie si inst correspond à "*" (toutes instances)
inline bool instIsAll(const char* inst) {
  return inst && inst[0] == '*' && inst[1] == '\0';
}

// Parse un entier simple, fallback si non numérique
inline long parseIntOrDefault(const char* s, long dflt=0) {
  if(!s) return dflt;
  return atol(s);
}

#endif // VPIV_UTILS_H

// Implementation (put in vpiv_utils.cpp or below if single-file)
#ifdef VPIV_UTILS_IMPLEMENTATION

int parseIndexList(const char* s, int* outIndices, int maxCount) {
  // attend "[0,1,2]" ou "0" ou "5"
  if(!s) return -1;
  while(*s == ' ') s++;
  if(*s == '[') {
    s++; // skip '['
    int count = 0;
    char numbuf[8];
    int ni = 0;
    while(*s && *s != ']') {
      // read number
      ni = 0;
      while(*s == ' ') s++;
      if(!isdigit(*s) && *s != '-') return -1;
      while(*s && (isdigit(*s) || *s == '-')) {
        if(ni < (int)sizeof(numbuf)-1) numbuf[ni++] = *s;
        s++;
      }
      numbuf[ni] = '\0';
      if(count < maxCount) outIndices[count] = atoi(numbuf);
      count++;
      while(*s == ' ') s++;
      if(*s == ',') s++;
    }
    return count;
  } else {
    // simple number
    while(*s == ' ') s++;
    if(!*s) return -1;
    int val = atoi(s);
    if(maxCount>0) outIndices[0] = val;
    return 1;
  }
}

bool parseRGB(const char* s, int out[3]) {
  if(!s) return false;
  // format "r,g,b" with commas
  int a=0,b=0,c=0;
  char *p1, *p2;
  // copy to temp
  char tmp[32];
  strncpy(tmp, s, sizeof(tmp)-1); tmp[sizeof(tmp)-1] = '\0';
  p1 = strtok(tmp, ",");
  if(!p1) return false;
  p2 = strtok(NULL, ",");
  if(!p2) return false;
  char* p3 = strtok(NULL, ",");
  if(!p3) return false;
  a = atoi(p1); b = atoi(p2); c = atoi(p3);
  out[0] = a; out[1] = b; out[2] = c;
  return true;
}

#endif // VPIV_UTILS_IMPLEMENTATION
