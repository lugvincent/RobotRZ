/*
 * ============================================================================
 * NOM       : fn_init_traitements
 * CHEMIN    : Node-RED SP → onglet Gtion_Init → nœud function
 * VERSION   : 1.0 — 18/10/2026 — Vincent Philippe / Claude
 *
 * OBJECTIF
 * --------
 *   Charger en mémoire le REGISTRE DES TRAITEMENTS : le « carnet de recettes »
 *   qui dit, pour chaque id_traitement reçu dans une capsule vocale, quelle
 *   transformation appliquer à la valeur brute captée par le micro.
 *
 *   Exemple concret : vous dites « tourne à droite de 90 degrés ». SE reconnaît
 *   la commande, voit dans la Table D qu'elle est de type CALCUL avec
 *   id_traitement = CALC_MTR_ANGLE, et transmet « 90 » brut à SP. C'est ici
 *   que SP apprend que CALC_MTR_ANGLE signifie « prendre ce nombre, vérifier
 *   qu'il est entre -360 et 360, et le poser dans le gabarit vitesse,angle,acc ».
 *
 * POURQUOI UNE VARIABLE GLOBALE PLUTÔT QU'UNE LISTE DANS LE ROUTEUR
 * ------------------------------------------------------------------
 *   Le registre est une DONNÉE, pas du code. En le sortant du nœud routeur :
 *     - une page de tableau de bord pourra l'éditer plus tard (ui_Traitement)
 *       sans qu'on ait à rouvrir un nœud function ;
 *     - ajouter une recette ne demande plus de modifier le routeur ;
 *     - le routeur reste court et ne change plus.
 *   Tant que ui_Traitement n'existe pas, ce fichier EST le registre.
 *
 * ARTICULATION
 * ------------
 *   Entrée  : déclenché au démarrage, en même temps que SF_Init_1
 *             (ou par un inject manuel pour recharger après modification).
 *   Écrit   : global.RZ_Traitements
 *   Lu par  : fn_stt_intent_router (onglet RZ_GestionFlux_V3)
 *   Source  : Table D v3, colonnes ID_traitement, Prepa_Encodage,
 *             VALEUR et Note_technique.
 *   Sortie  : un message de compte-rendu, à brancher sur un debug.
 *
 * PRÉCAUTIONS
 * -----------
 *   - Ce registre ne contient AUCUNE fonction JavaScript, uniquement des
 *     descriptions. C'est volontaire : une donnée se sérialise, s'édite
 *     depuis une page web et se sauvegarde ; du code, non.
 *   - Les trois types d'interprétation reconnus sont « table », « nombre »
 *     et « table_ou_nombre ». Le routeur ne sait rien faire d'autre : si
 *     vous inventez un quatrième type ici, il faudra l'ajouter là-bas.
 *   - Une recette avec actif:false répond proprement « non implémentée »
 *     au lieu de produire une valeur fausse. C'est le cas de
 *     CALC_MTR_DISTANCE, dont le format de valeur n'est pas défini.
 *   - Recharger ce nœud écrase le registre : une édition faite ailleurs
 *     en mémoire serait perdue.
 * ============================================================================
 */

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 1 — CONSTANTES PARTAGÉES
// Valeurs par défaut reprises des notes de la Table D. Les sortir ici évite
// de les répéter dans chaque recette et de les voir diverger avec le temps.
// ─────────────────────────────────────────────────────────────────────────────
const ACC_DEFAUT = 2;      // accélération moteur par défaut — Table D, MTR_SPEED_SET
const VITESSE_ROTATION = 50; // ⚠ non spécifié en Table D, valeur à confirmer en essai réel

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 2 — LE REGISTRE
//
// Chaque entrée décrit UNE recette. Champs communs :
//   libelle    : à quoi elle sert, en français, pour la future interface
//   type       : "table" | "nombre" | "table_ou_nombre"
//   actif      : false = répond « non implémentée » sans rien calculer
//   erreur     : raison renvoyée dans l'ACK d'échec, en minuscules sans espace
//
// Champs selon le type :
//   table      : { mot capté : valeur VPIV }
//   nombre     : { min, max, defaut, entier }
//   gabarit    : modèle de la valeur finale, {valeur} est remplacé par le
//                nombre lu, les autres {jetons} par les constantes ci-dessous
//   constantes : valeurs fixes injectées dans le gabarit
// ─────────────────────────────────────────────────────────────────────────────
const REGISTRE = {

    // ── TRADUCTION : un mot capté devient un code ────────────────────────────

    CONV_COULEUR_BASE: {
        libelle: "Nom de couleur parlé vers triplet RVB",
        type: "table",
        actif: true,
        erreur: "couleur_inconnue",
        table: {
            "rouge":  "255,0,0",
            "vert":   "0,255,0",
            "bleu":   "0,0,255",
            "blanc":  "255,255,255",
            "jaune":  "255,255,0",
            "violet": "128,0,128",
            "orange": "255,165,0",
            "noir":   "0,0,0"
        }
    },

    CONV_TYPEPTGE: {
        libelle: "Mot de pilotage parlé vers code CfgS.typePtge",
        type: "table",
        actif: true,
        erreur: "pilotage_inconnu",
        // Codes repris de la variable CfgS.typePtge :
        // 0 Ecran · 1 Vocal · 2 Manette · 3 Laisse · 4 Laisse+Vocal
        table: {
            "ecran":        "0",
            "écran":        "0",
            "vocal":        "1",
            "vocale":       "1",
            "voix":         "1",
            "manette":      "2",
            "joystick":     "2",
            "laisse":       "3",
            "laisse vocal": "4"
        }
    },

    // ── CALCUL : un nombre capté est validé puis mis en forme ────────────────

    CALC_MTR_ANGLE: {
        libelle: "Angle de rotation vers commande moteur",
        type: "nombre",
        actif: true,
        erreur: "angle_invalide",
        // Le SIGNE vient de SE : négatif à gauche, positif à droite.
        // Décision du 18/10/2026 — SP ne reçoit pas l'identifiant de commande
        // et ne peut donc pas déduire le sens tout seul.
        nombre: { min: -360, max: 360, defaut: 45, entier: true },
        gabarit: "{vitesse},{valeur},{acc}",
        constantes: { vitesse: VITESSE_ROTATION, acc: ACC_DEFAUT }
    },

    CALC_MTR_VITESSE: {
        libelle: "Vitesse parlée vers commande moteur",
        type: "nombre",
        actif: true,
        erreur: "vitesse_invalide",
        nombre: { min: 0, max: 100, defaut: 50, entier: true },
        // Vitesse réglée sans rotation : l'angle du triplet reste à zéro.
        gabarit: "{valeur},0,{acc}",
        constantes: { acc: ACC_DEFAUT }
    },

    CALC_SRV_ANGLE: {
        libelle: "Angle de tête vers position du servomoteur",
        type: "nombre",
        actif: true,
        erreur: "angle_tete_invalide",
        nombre: { min: 0, max: 180, defaut: 90, entier: true }
        // Pas de gabarit : la valeur part telle quelle.
    },

    CALC_LRING_LUMIN: {
        libelle: "Intensité parlée vers luminosité de l'anneau",
        type: "nombre",
        actif: true,
        erreur: "luminosite_invalide",
        nombre: { min: 0, max: 255, defaut: 128, entier: true }
    },

    // ── CALCUL mixte : un adverbe OU un nombre ───────────────────────────────

    CALC_VOICE_VOL: {
        libelle: "Volume parlé — adverbe ou pourcentage",
        type: "table_ou_nombre",
        actif: true,
        erreur: "volume_invalide",
        // L'adverbe est cherché en premier ; à défaut, on lit un nombre.
        table: {
            "fort":        "80",
            "très fort":   "95",
            "doux":        "30",
            "bas":         "20",
            "faible":      "20",
            "moyen":       "50",
            "maximum":     "100",
            "minimum":     "10"
        },
        nombre: { min: 0, max: 100, defaut: 50, entier: true }
    },

    // ── EN ATTENTE DE SPÉCIFICATION ──────────────────────────────────────────

    CALC_MTR_DISTANCE: {
        libelle: "Distance parlée vers commande moteur",
        type: "nombre",
        actif: false,   // ⚠ volontairement inactif — voir ci-dessous
        erreur: "distance_non_specifiee",
        nombre: { min: 0, max: 1000, defaut: 100, entier: true },
        // ⚠ À TRANCHER AVANT ACTIVATION :
        //   Mtr.cmd attend un triplet vitesse,angle,accélération. Une distance
        //   n'entre pas dans ce format. La note de la Table D précise que cette
        //   commande « requiert Odom pour contrôle de distance » — il faut donc
        //   décider si SP émet une consigne Odom en plus, ou si la distance
        //   passe par une autre propriété. Tant que ce n'est pas tranché, la
        //   recette répond un échec honnête plutôt qu'une valeur inventée.
        note: "Format de valeur non defini — voir Odom avant activation"
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 3 — CONTRÔLE DE COHÉRENCE
// Mieux vaut découvrir une recette mal écrite au démarrage, dans un debug,
// qu'au moment où l'on parle au robot et qu'il répond « commande refusée ».
// ─────────────────────────────────────────────────────────────────────────────
const TYPES_CONNUS = ["table", "nombre", "table_ou_nombre"];
const anomalies = [];

for (const [cle, r] of Object.entries(REGISTRE)) {
    if (!TYPES_CONNUS.includes(r.type)) {
        anomalies.push(cle + " : type inconnu « " + r.type + " »");
    }
    if ((r.type === "table" || r.type === "table_ou_nombre") && !r.table) {
        anomalies.push(cle + " : type " + r.type + " sans table");
    }
    if ((r.type === "nombre" || r.type === "table_ou_nombre") && !r.nombre) {
        anomalies.push(cle + " : type " + r.type + " sans bornes numériques");
    }
    if (!r.erreur) {
        anomalies.push(cle + " : pas de raison d'erreur definie");
    }
    // Un gabarit ne doit référencer que {valeur} et des constantes déclarées.
    if (r.gabarit) {
        const jetons = (r.gabarit.match(/\{(\w+)\}/g) || [])
            .map(j => j.slice(1, -1))
            .filter(j => j !== "valeur");
        for (const j of jetons) {
            if (!r.constantes || r.constantes[j] === undefined) {
                anomalies.push(cle + " : gabarit utilise {" + j + "} non declare dans constantes");
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 4 — PUBLICATION
// ─────────────────────────────────────────────────────────────────────────────
global.set("RZ_Traitements", REGISTRE);

const actifs = Object.values(REGISTRE).filter(r => r.actif !== false).length;
const total  = Object.keys(REGISTRE).length;

if (anomalies.length > 0) {
    node.error("[Init Traitements] " + anomalies.length + " anomalie(s) : " + anomalies.join(" | "));
    node.status({ fill: "red", shape: "dot", text: anomalies.length + " anomalie(s)" });
} else {
    node.log("[Init Traitements] registre charge : " + actifs + " recettes actives sur " + total);
    node.status({ fill: "green", shape: "dot", text: actifs + "/" + total + " recettes" });
}

msg.payload = {
    charge: true,
    total: total,
    actifs: actifs,
    inactifs: Object.entries(REGISTRE)
        .filter(([, r]) => r.actif === false)
        .map(([c]) => c),
    anomalies: anomalies
};

return msg;
