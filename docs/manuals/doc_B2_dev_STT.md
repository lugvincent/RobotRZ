# Guide Développeur — Module STT/TTS du projet RZ

**Projet :** RZ — Robot compagnon autonome  
**Auteur :** Vincent Philippe  
**Version :** 1.0 — Mars 2026  
**Public :** Développeurs et mainteneurs du module STT, intégrateurs de nouvelles commandes vocales

---

## 1. Architecture complète du module STT

### Vue d'ensemble

Le module STT (Speech-To-Text, reconnaissance vocale) de RZ est un pipeline en plusieurs étapes qui transforme une phrase prononcée par l'utilisateur en une trame VPIV envoyée au robot. Il s'appuie sur PocketSphinx pour la reconnaissance locale hors-ligne et sur l'API Gemini (Google) pour le repli conversationnel.

```
[Microphone Android]
        │
        ▼
rz_stt_manager.sh / rz_stt_manager.py
  PocketSphinx KWS (Keyword Spotting)
  Écoute permanente du wake word "rz"
  Capture la phrase complète après le wake word
        │
        ▼ (texte brut reconnu)
rz_stt_handler.sh "texte reconnu" [valeur_numérique]
  ├─ Lecture état (global.json)
  ├─ Filtres sécurité (CPU, modeRZ, laisse)
  ├─ Recherche dans stt_catalog.json
  ├─ Traitement selon type (SIMPLE / COMPLEXE / PLAN / LOCAL)
  ├─ Construction trame VPIV capsulée
  ├─ Envoi FIFO → rz_state-manager → MQTT → SP
  └─ TTS réponse (termux-tts-speak)
        │
        │ [Si exit 1 : commande inconnue]
        ▼
rz_ai_conversation.py "phrase complète"
  Appel API Gemini 1.5 Flash
  Réponse TTS directe
```

### Fichiers du module

| Fichier | Rôle | Modifié lors d'une MàJ catalogue ? |
|---------|------|-------------------------------------|
| `original_init.sh` | Source du catalogue CSV (section balisée) | **OUI — point d'entrée** |
| `table_d_catalogue.csv` | CSV généré par original_init.sh | Non (généré) |
| `rz_build_system.py` | CSV → stt_catalog.json + rz_words.txt | Non |
| `rz_build_dict.py` | rz_words.txt + lexique_referent.dict → fr.dict | Non |
| `stt_catalog.json` | Catalogue compilé utilisé au runtime | Non (généré) |
| `rz_words.txt` | Liste de mots pour PocketSphinx | Non (généré) |
| `lib_pocketsphinx/fr.dict` | Dictionnaire phonétique réduit | Non (généré) |
| `lib_pocketsphinx/model-fr/` | Modèle acoustique (~50-200 Mo, hors Git) | Jamais |
| `lib_pocketsphinx/lexique_referent.dict` | Dict source FR (hors Git) | Jamais |
| `rz_stt_manager.sh` | Boucle PocketSphinx wake word | Non |
| `rz_stt_handler.sh` | Cœur décisionnel catalogue → VPIV | Non |
| `rz_ai_conversation.py` | Repli IA Gemini | Non |
| `config/sensors/stt_config.json` | Paramètres runtime PocketSphinx | Non (généré) |

---

## 2. La Table D — Catalogue STT

### Localisation et édition

Le catalogue est défini **uniquement** dans `original_init.sh`, dans la section balisée :

```
# ## === SECTION CATALOGUE STT — TABLE D ===
...lignes CSV...
# ## === FIN SECTION CATALOGUE STT ===
```

Le fichier `table_d_catalogue.csv` est un artefact **généré** — ne jamais l'éditer directement.

### Structure CSV — 13 colonnes

```
ID ; Traitement ; CAT ; Destinataire ; moteurActif ; Alias ; ID_fils ;
VAR ; PROP ; INSTANCE ; VALEUR ; Prepa_Encodage ; Alias-TTS
```

### Description détaillée de chaque colonne

**ID** — Identifiant unique de la commande  
- Format : `MODULE_ACTION` en majuscules, tiret bas
- Exemples : `MTR_FORWARD`, `CFG_VEILLE`, `MACRO_HELLO`, `URGENCY_STOP`
- Règle : unique dans tout le catalogue, stable dans le temps (référencé par ID_fils)

**Traitement** — Type de traitement dans le handler

| Valeur | Description | Particularité |
|--------|-------------|---------------|
| SIMPLE | Une trame VPIV directe | Cas le plus fréquent |
| COMPLEXE | Enchaîne plusieurs commandes enfants | ID_fils obligatoire |
| PLAN | Commande avec extraction numérique | Prepa_Encodage obligatoire |
| LOCAL | Traitement interne SE, pas de trame VPIV externe | Pour SYS_STATUS, SYS_BATTERY |

**CAT** — Catégorie de la trame VPIV produite  
`V` (Consigne) | `I` (Information) | `E` (Événement) | `F` (Flux) | `A` (Application)

**Destinataire** — Composant cible  
`A` (Arduino Mega) | `SE` (Smartphone Embarqué) | `SP` (Node-RED)

**moteurActif** — Indicateur sécurité laisse  
`0` = commande sans moteur | `1` = commande impliquant les moteurs  
⚠️ Si `moteurActif=1` et `typePtge=4` (laisse+vocal) : commande bloquée sauf STOP.

**Alias** — Synonymes prononcés, séparés par virgule  
- Écrits en minuscules, sans accents majuscules
- Chaque alias doit être prononçable par PocketSphinx (doit figurer dans `fr.dict`)
- Tous les mots de tous les alias sont extraits dans `rz_words.txt` puis dans `fr.dict`
- Exemple : `avance,tout droit,vas-y,en avant`

**ID_fils** — Pour COMPLEXE uniquement  
Liste des IDs enfants séparés par virgule, dans l'ordre d'exécution.  
Délai de 200ms entre chaque commande enfant.  
Exemple : `MTR_STOP,LRING_OFF,CFG_VEILLE`

**VAR** — Variable VPIV ciblée  
Doit correspondre exactement à un module de la Table A : `Mtr`, `CfgS`, `CamSE`, `LRing`, `Srv`, `Appli`, `MicroSE`, `Urg`, `COM`...

**PROP** — Propriété de la variable  
Doit correspondre à une propriété définie dans la Table A pour cette VAR.  
Commandes ponctuelles : entre parenthèses `(reset)`, `(clear)`, `(snap)`.

**INSTANCE** — Instance cible  
`*` pour module unique ou toutes instances | `rear`/`front` | `TGD`/`THB`/`BASE` | index numérique

**VALEUR** — Valeur de la trame VPIV  
Laissée vide pour PLAN (remplacée par Prepa_Encodage).  
Pour SIMPLE/COMPLEXE : valeur directe.

**Prepa_Encodage** — Pour PLAN uniquement

| Code | Description | Exemple d'usage |
|------|-------------|-----------------|
| `NUM` | Injecte le nombre extrait de la phrase | "luminosité 80" → val=80 |
| `NUM_DIR` | ±nombre selon gauche/droite/haut/bas | "tourne à droite de 45" → val=+45 |
| `angle+speed` | Couple angle+vitesse croisière | "avance de 50" → val=50,speed_cruise,2 |

**Alias-TTS** — Réponse vocale du robot  
Variantes séparées par `|`. L'handler choisit une variante aléatoirement.  
Exemple : `Je fonce.|C'est parti.|En avant !`  
Vide = pas de réponse vocale.

### Lignes de commentaires dans le CSV

Les lignes qui commencent par `;;;;` sont des séparateurs de section, ignorés par `rz_build_system.py` :
```
;;;;;;;;;;;; MOTEUR — déplacements simples ;;;;
```

---

## 3. Types de traitement détaillés

### SIMPLE — Une commande directe

Le cas le plus fréquent. Produit exactement une trame VPIV.

```
# Catalogue : MTR_FORWARD
# CAT=V, VAR=Mtr, PROP=cmd, INST=*, VALEUR=50,0,2
# Traitement :
inner_vpiv = "V:Mtr:cmd:*:50,0,2"
capsule = "$A:STT:intent:*:{V:Mtr:cmd:*:50,0,2}#"  → FIFO → SP valide → Arduino
```

L'encapsulation dans `$A:STT:intent:*:{...}#` permet à SP de valider l'intent avant exécution.

### COMPLEXE — Enchaînement de commandes

Exécute les commandes ID_fils dans l'ordre, avec 200ms de délai entre chaque.

```
# Catalogue : MACRO_SLEEP  ID_fils=MTR_STOP,LRING_OFF,CFG_VEILLE
# Traitement :
for f in MTR_STOP LRING_OFF CFG_VEILLE:
    process_simple(f)  # envoie chaque trame individuellement
    sleep 0.2
```

### PLAN — Commande avec extraction numérique

Pour les commandes où l'utilisateur fournit un nombre dans sa phrase.  
Le handler extrait le premier nombre trouvé dans la phrase reconnue.

```
# Utilisateur dit : "rz vitesse 40"
# Catalogue : MTR_SPEED_SET  Prepa_Encodage=NUM
# Traitement :
CLEAN_TEXT = "vitesse 40"
VAL_NUMERIQUE = "40"  ← extrait par le manager STT avant d'appeler le handler
# La valeur 40 remplace le champ VALEUR vide du catalogue
```

Pour `NUM_DIR` : le signe est déterminé par la présence de "droite"/"haut" (positif) ou "gauche"/"bas" (négatif).

### LOCAL — Traitement interne SE

Pas de trame VPIV vers SP ou Arduino. Le handler traite directement.

```
# Catalogue : SYS_STATUS  CAT=I
# Traitement :
# Lit global.json
# Compose un message de statut vocal
# Appelle termux-tts-speak directement
```

---

## 4. Workflow de build complet

### Séquence de régénération après modification du catalogue

```
1. Éditer la section catalogue dans original_init.sh
   (section === SECTION CATALOGUE STT ===)
        │
        ▼
2. Exécuter original_init.sh
   → génère table_d_catalogue.csv
        │
        ▼
3. rz_build_system.py lit table_d_catalogue.csv
   → génère stt_catalog.json (catalogue compilé JSON)
   → génère rz_words.txt (tous les mots des alias + "rz")
        │
        ▼
4. rz_build_dict.py lit rz_words.txt + lexique_referent.dict
   → génère fr.dict (dictionnaire phonétique réduit)
   → signale les mots non trouvés dans le lexique (→ "T B D" à corriger)
        │
        ▼
5. check_config.sh valide stt_config.json
   + vérifie présence de lib_pocketsphinx/model-fr/
        │
        ▼
6. Redémarrer rz_stt_manager.sh (reload fr.dict)
```

### Gestion des mots non trouvés dans le lexique

Quand `rz_build_dict.py` ne trouve pas un mot dans `lexique_referent.dict`, il écrit :
```
bonjour    # trouvé → phonétique réelle
avance     # non trouvé → T B D  ← à corriger manuellement
```

Les entrées `T B D` dans `fr.dict` empêchent PocketSphinx de reconnaître ces mots correctement. Pour les corriger, trouver la phonétique française et éditer `fr.dict` directement.

---

## 5. Ajouter ou modifier une commande vocale — Procédure pas à pas

### Ajouter une nouvelle commande SIMPLE

**Exemple :** Ajouter la commande "allume la lumière" pour activer un éclairage via Arduino.

**Étape 1 :** Vérifier dans la Table A que la propriété existe.  
→ `$V:LRing:act:*:1#` — propriété `act` de `LRing` ✓

**Étape 2 :** Ouvrir `original_init.sh`, localiser la section catalogue.  
Trouver le groupe approprié (`;;;; LEDS — anneau ;;;;`).

**Étape 3 :** Ajouter la ligne CSV :
```
LRING_AMBIENT;SIMPLE;V;A;0;allume la lumière,lumière ambiante,éclaire;;LRing;act;*;1;;Voilà, j'éclaire.|Lumière allumée.
```

**Étape 4 :** Vérifier les alias (pas de doublons avec les commandes existantes).  
Ici "allume" existe déjà pour `LRING_ON` ("allume les leds") — les deux coexistent si les phrases complètes sont distinctes.

**Étape 5 :** Relancer `original_init.sh` ou à minima :
```bash
cd ~/scripts_RZ_SE/termux/scripts/sensors/stt
python3 rz_build_system.py
python3 rz_build_dict.py
# Vérifier les T B D dans fr.dict
# Redémarrer rz_stt_manager
```

### Ajouter une commande PLAN

**Exemple :** "RZ tourne de X degrés" avec X variable.

```
MTR_TURN_ANGLE;PLAN;V;A;1;tourne de,tourne à;;Mtr;cmd;*;;NUM_DIR;Je tourne.
```
- VALEUR est vide (remplacé par le nombre extrait)
- Prepa_Encodage = `NUM_DIR` → le signe dépend de "droite"/"gauche" dans la phrase

### Ajouter une MACRO

**Exemple :** Macro "mode patrouille" = activer mouvement + surveillance micro.

```
MACRO_PATROL;COMPLEXE;V;SE;1;mode patrouille,commence la patrouille;CFG_DEPLACEMENT,MIC_SURV;;;;;;;Mode patrouille activé.
```

**Vérifier :** tous les IDs fils existent dans le catalogue.

### Supprimer une commande

1. Supprimer la ligne dans la section catalogue de `original_init.sh`
2. Si la commande est référencée comme ID_fils d'une MACRO : mettre à jour la macro aussi
3. Relancer le build complet

### Modifier un alias

1. Éditer le champ Alias de la ligne concernée
2. Relancer le build (les mots supprimés disparaîtront de `fr.dict`, les nouveaux y seront ajoutés)

---

## 6. Filtres de sécurité du handler

`rz_stt_handler.sh` applique 5 filtres dans l'ordre avant d'exécuter une commande :

### Filtre 1 — CPU (Étape B)
```bash
if [ "$CPU_INT" -gt 90 ]; then
    termux-tts-speak "Je suis trop chargé."
    exit 0  # abandon silencieux
fi
```
Seuil fixe à 90%. Pas configurable depuis le catalogue — c'est une protection matérielle.

### Filtre 2 — Identification (Étape C)
```bash
CMD_DATA=$(jq '.commandes[] | select(.synonymes[] == $txt)' stt_catalog.json)
```
Si aucun synonyme ne correspond exactement : `exit 1` → déclenche Gemini.

### Filtre 3 — Mode RZ (Étape D)
```bash
if [[ "$MODE_RZ" == "0" || "$MODE_RZ" == "5" ]]; then
    if [[ "$CMD_ID" != "URGENCY_STOP" ]]; then
        termux-tts-speak "Système indisponible."
        exit 0
    fi
fi
```
Modes 0 (ARRET) et 5 (ERREUR) bloquent toutes les commandes sauf `URGENCY_STOP`.

### Filtre 4 — Laisse (Étape E)
```bash
if [[ "$TYPE_PTGE" == "4" && "$MOTEUR_ACTIF" == "1" ]]; then
    if [[ "$CMD_ID" != *"STOP"* ]]; then
        termux-tts-speak "Mouvement bloqué par la laisse."
        exit 0
    fi
fi
```
En mode laisse+vocal (typePtge=4), les commandes avec `moteurActif=1` sont bloquées sauf les STOP.

### Filtre 5 — Type de traitement (Étape F)
Aiguille vers `process_simple()` (SIMPLE/LOCAL), boucle fils (COMPLEXE), ou extraction numérique (PLAN).

---

## 7. Bascule vers l'IA Gemini

### Déclenchement

Quand `rz_stt_handler.sh` termine avec `exit 1` (commande inconnue), le manager STT appelle :
```bash
python3 rz_ai_conversation.py "phrase complète"
```

### Comportement de rz_ai_conversation.py

1. Lit la clé API dans `config/keys.json` (clé `GEMINI_API_KEY`)
2. Construit un prompt avec la personnalité RZ : *"Tu es RZ, un robot compagnon amical. Réponds en une seule phrase courte."*
3. Appelle l'API `gemini-1.5-flash` (rapide, faible latence)
4. Passe la réponse à `termux-tts-speak`

### Quand Gemini est utilisé vs catalogue

| Situation | Mécanisme | Exemple |
|-----------|-----------|---------|
| Commande connue | Catalogue → VPIV | "avance", "caméra on" |
| Question conversationnelle | Gemini | "comment tu vas", "quel âge as-tu" |
| Commande partielle | Gemini (non reconnu) | "vas vers la fenêtre" |
| CPU > 90% | Abandon TTS | (aucun Gemini appelé) |

### Précaution clé API

La clé `GEMINI_API_KEY` dans `config/keys.json` **ne doit pas être versionnée dans Git** (`.gitignore`). Elle doit être copiée manuellement sur le smartphone.

---

## 8. TTS — Sources et règles

### Les 3 sources de TTS dans le système SE

**Source 1 — Catalogue (Alias-TTS)**  
Déclenchée par `rz_stt_handler.sh` après exécution d'une commande reconnue.  
Sélection aléatoire parmi les variantes `|`.  
Exemples : "Je fonce.", "C'est parti.", "Arrêt d'urgence."

**Source 2 — Messages COM de SP**  
Déclenchée par `rz_vpiv_parser.sh` si `typePtge ≠ 0`.  
```bash
# $I:COM:error:SE:"Connexion perdue"#  → TTS "Erreur : Connexion perdue"
# $I:COM:info:SE:"Caméra initialisée"# → TTS "Caméra initialisée"
```
Seuls `COM.error` et `COM.info` déclenchent le TTS. `debug` et `warn` sont silencieux.

**Source 3 — Urgences**  
Déclenchée par `rz_vpiv_parser.sh` pour certains codes d'urgence :
```bash
"URG_LOW_BAT"    → "Batterie critique."
"URG_CPU_CHARGE" → "Surcharge processeur."
*                → "Alerte sécurité."
```

### Règles TTS

- Un seul appel TTS actif à la fois (séquentiel, pas de superposition)
- Phrases courtes recommandées (TTS Android peut être lent sur > 15 mots)
- Le TTS est inactif si `typePtge = 0` (pilotage écran sans retour vocal) pour les sources 2 et 3
- La source 1 (catalogue) produit du TTS quel que soit `typePtge`

---

## 9. Paramètres PocketSphinx (stt_config.json)

| Paramètre | Description | Valeur typique |
|-----------|-------------|----------------|
| modeSTT | KWS (actif) ou OFF | KWS |
| threshold | Sensibilité détection wake word | 1e-20 |
| keyphrase | Wake word | "rz" |
| lang | Langue du modèle acoustique | fr |
| lib_path | Chemin relatif vers lib_pocketsphinx/ | lib_pocketsphinx |

**Ajustement du threshold :**
- `1e-20` : valeur recommandée, équilibre entre sensibilité et faux déclenchements
- Plus petit (ex: `1e-30`) : plus exigeant, moins de faux positifs mais peut rater des déclenchements
- Plus grand (ex: `1e-10`) : plus permissif, plus de faux positifs

---

## 10. Dépannage courant

| Symptôme | Cause probable | Solution |
|----------|---------------|----------|
| Wake word non détecté | `fr.dict` ne contient pas "rz" | Vérifier `fr.dict`, relancer build |
| Commande reconnue mais non exécutée | modeRZ=0 ou CPU>90% | Vérifier global.json |
| "T B D" dans fr.dict | Mot absent du lexique_referent.dict | Corriger manuellement fr.dict |
| Gemini appelé pour une commande connue | Alias mal écrit (accent, casse) | Vérifier les alias dans le catalogue |
| TTS muet | typePtge=0 (sources 2/3) | Normal — passer en typePtge≥1 pour TTS COM |
| rz_build_system.py échoue | CSV mal formé | Vérifier les séparateurs `;` dans le catalogue |
| model-fr/ absent | Fichiers lourds non installés | Copier manuellement sur le smartphone |
| Temps de réponse STT trop long | winSize trop grand ou CPU surchargé | Réduire winSize, vérifier charge CPU |

---

## Annexe — Exemple complet de stt_catalog.json (extrait)

```json
{
  "commandes": [
    {
      "id": "MTR_FORWARD",
      "traitement": "SIMPLE",
      "cat": "V",
      "destinataire": "A",
      "moteurActif": 1,
      "synonymes": ["avance", "tout droit", "vas-y", "en avant"],
      "id_fils": [],
      "vpiv_parts": {
        "var": "Mtr",
        "prop": "cmd",
        "inst": "*",
        "val": "50,0,2"
      },
      "alias_tts": ["Je fonce.", "C'est parti."]
    },
    {
      "id": "MACRO_HELLO",
      "traitement": "COMPLEXE",
      "cat": "V",
      "destinataire": "A",
      "moteurActif": 0,
      "synonymes": ["bonjour", "salut", "coucou"],
      "id_fils": ["LRING_SMILE", "EXPR_SMILE"],
      "vpiv_parts": { "var": "", "prop": "", "inst": "", "val": "" },
      "alias_tts": ["Bonjour ! Je suis RZ."]
    }
  ]
}
```

---

*Fin du Guide Développeur STT — Projet RZ*  
*Version 1.0 — Mars 2026 — Vincent Philippe*
