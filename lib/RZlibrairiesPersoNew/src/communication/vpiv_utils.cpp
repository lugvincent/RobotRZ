/************************************************************
 * FILE : src/communication/vpiv_utils.cpp
 * ROLE : Parsing robuste des listes d'indices, RGB, etc.
 * contexte d'utilisation : VPIV (ex : "Lring:color:[1,3]:255,0,0")
 * version : 1.00
 * date : 2024-06-01
 * auteur : Vincent Philippe
 ************************************************************/
#include "vpiv_utils.h"
#include <string.h>
#include <ctype.h>

int parseIndexList(const char *s, int *out, int maxCount)
{
    if (!s)
        return -1;

    // Trim spaces
    while (*s == ' ')
        s++;

    // Cas "*"
    if (s[0] == '*' && s[1] == '\0')
    {
        return 0; // 0 = ALL instances
    }

    // Cas vide → ALL
    if (s[0] == '\0')
        return 0;

    // Cas liste "[1,2,4]"
    if (s[0] == '[')
    {
        s++; // skip '['
        int count = 0;
        char buf[16]; // taille augmentée
        int bi = 0;

        while (*s && *s != ']')
        {
            while (*s == ' ')
                s++;

            if (!isdigit(*s) && *s != '-')
                return -1;

            bi = 0;
            while (*s && (isdigit(*s) || *s == '-'))
            {
                if (bi < (int)sizeof(buf) - 1)
                    buf[bi++] = *s;
                s++;
            }
            buf[bi] = '\0';

            if (count < maxCount)
                out[count] = atoi(buf);
            count++;

            while (*s == ' ')
                s++;
            if (*s == ',')
                s++;
        }
        return count;
    }

    // Cas simple "3"
    out[0] = atoi(s);
    return 1;
}

bool parseRGB(const char *s, int out[3])
{
    if (!s)
        return false;

    char tmp[32];
    strncpy(tmp, s, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *p = strtok(tmp, ",");
    if (!p)
        return false;
    out[0] = constrain(atoi(p), 0, 255);

    p = strtok(NULL, ",");
    if (!p)
        return false;
    out[1] = constrain(atoi(p), 0, 255);

    p = strtok(NULL, ",");
    if (!p)
        return false;
    out[2] = constrain(atoi(p), 0, 255);

    return true;
}
