/*
 * ============================================================================
 * NOM       : fn_stt_intent_router
 * FLOW      : M_Pil_Vocale — branché sur la sortie "STT" du switch
 *             "Séparation STT Voice"
 * CHEMIN    : Node-RED SP → onglet RZ_GestionFlux_V3
 *
 * OBJECTIF
 * --------
 *   Décoder les trames STT.intent (commandes vocales encapsulées par SE) et
 *   produire un msg.vpiv exploitable par Gtion_EtatGlobal_V2, avec un marqueur
 *   d'origine explicite pour que SF_ValidationIntent_V1 sache qu'il s'agit
 *   d'une intention utilisateur vocale et non d'une donnée de capteur.
 *
 * SORTIES : 3 (inchangées depuis la v1.3)
 *   1 — Intent décodé (msg.vpiv) → link out → EVT_RAW_IN → Gtion_EtatGlobal_V2
 *   2 — ACK STT (modeSTT, context) → fn_stt_status_store
 *   3 — msg.ack → instance de SF_EmissionACK
 *
 * ENTRÉE
 *   msg.vpiv = { cat, var:"STT", prop, inst, val }, rempli par fn_parse_vpiv_v3
 *
 *   Deux formes de capsule arrivent dans vpiv.val, entre accolades :
 *     5 champs — cat:var:prop:inst:val      → déjà résolue par SE, on passe
 *     6 champs — id_traitement:cat:var:prop:inst:valBrute → à résoudre ici
 *   L'id_traitement est en PREMIER pour pouvoir trancher sur le nombre de
 *   champs sans rien réinterpréter (convention posée par rz_stt_handler.sh).
 *
 * CE QUI CHANGE EN v1.4
 * ---------------------
 *   La liste de couleurs codée en dur disparaît. Les recettes de résolution
 *   viennent désormais de global.RZ_Traitements, chargé au démarrage par
 *   fn_init_traitements (onglet Gtion_Init). Ce nœud ne contient plus que
 *   le MOTEUR d'interprétation ; les recettes sont des données éditables.
 *   Trois types sont compris : "table", "nombre", "table_ou_nombre".
 *   Le comportement de CONV_COULEUR_BASE est identique à la v1.3 — c'est le
 *   témoin qui prouve que la migration n'a rien cassé.
 *
 * ARTICULATION
 * ------------
 *   Amont   : rz_stt_handler.sh v3.1 (SE) émet la capsule et attend un ACK
 *             corrélé par id_requete ; rz_vpiv_parser.sh v2.5 écrit le
 *             fichier de corrélation à la réception de cet ACK.
 *   Registre: global.RZ_Traitements ← fn_init_traitements
 *   Aval    : Gtion_EtatGlobal_V2 par la sortie 1, SF_EmissionACK par la 3.
 *
 * PRÉCAUTIONS
 * -----------
 *   - msg.stt_id_requete doit survivre intact jusqu'au bout de la chaîne pour
 *     que l'ACK final sache à qui répondre. Aucun nœud ne le supprime
 *     aujourd'hui ; vérifier avant d'en ajouter un.
 *   - Si le registre est absent (init non joué), toute capsule à 6 champs
 *     échoue avec la raison registre_absent. C'est volontaire : mieux vaut un
 *     refus lisible qu'un comportement muet.
 *   - L'id_requete est lu sur l'enveloppe EXTERNE (vpiv.inst), pas dans le
 *     contenu interne.
 *   - Le signe de l'angle de rotation vient de SE : négatif à gauche, positif
 *     à droite. SP ne reçoit pas l'identifiant de commande et ne peut donc pas
 *     déduire le sens. Décision du 18/10/2026.
 *
 * VERSION : 1.4 — 18/10/2026 — Vincent Philippe / Claude
 *           1.4 : registre externalisé dans global.RZ_Traitements, moteur
 *                 d'interprétation générique à trois types
 *           1.3 : première résolution réelle (CONV_COULEUR_BASE en dur)
 *           1.2 : capsule à 6 champs reconnue, id_requete lu sur l'enveloppe
 *           1.1 : migration msg.command → msg.vpiv + msg.origine
 * ============================================================================
 */

const vpiv = msg.vpiv;

if (!vpiv || vpiv.var !== 'STT') {
    node.warn('[STT Router] msg.vpiv absent ou VAR incorrect');
    return [null, null, null];
}

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 1 — ACK de statut STT : sortie 2 (inchangé)
// ─────────────────────────────────────────────────────────────────────────────
if (vpiv.prop === 'modeSTT' || vpiv.prop === 'context') {
    msg.stt_status = { prop: vpiv.prop, val: vpiv.val, timestamp: Date.now() };
    node.log('[STT Router] ACK ' + vpiv.prop + '=' + vpiv.val);
    return [null, msg, null];
}

if (vpiv.prop !== 'intent') {
    node.warn('[STT Router] PROP inconnue ignorée : ' + vpiv.prop);
    return [null, null, null];
}

// id_requete : porté par INST de l'enveloppe externe, pas par le contenu.
const id_requete = vpiv.inst;

const innerRaw = String(vpiv.val).replace(/^\{|\}$/g, '').trim();
const parts = innerRaw.split(':');

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 2 — Capsule à 5 champs : SE a déjà résolu, on ne touche à rien
// ─────────────────────────────────────────────────────────────────────────────
if (parts.length === 5) {
    const [iCat, iVar, iProp, iInst, iVal] = parts;

    if (!['V', 'I', 'A'].includes(iCat)) {
        node.warn('[STT Router] CAT interne refusée : ' + iCat);
        return [null, null, null];
    }

    msg.vpiv = { cat: iCat, var: iVar, prop: iProp, inst: iInst, val: iVal };
    msg.origine = 'STT';
    msg.topic = 'stt_intent';

    node.log('[STT Router] Intent (résolu SE) → ' + iCat + ':' + iVar + ':'
        + iProp + ':' + iInst + ':' + iVal);

    return [msg, null, null];
}

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 3 — MOTEUR D'INTERPRÉTATION
//
// Prend une recette du registre et la valeur brute captée au micro, rend soit
// la valeur VPIV finale, soit une raison d'échec. Aucune recette n'est écrite
// ici : ce sont des données, elles vivent dans global.RZ_Traitements.
// ─────────────────────────────────────────────────────────────────────────────
function appliquerGabarit(recette, valeur) {
    // Sans gabarit, la valeur part telle quelle (cas Srv.angle, Lring.lumin).
    if (!recette.gabarit) { return String(valeur); }

    // Avec gabarit, {valeur} reçoit le nombre lu et les autres jetons les
    // constantes déclarées à côté. Exemple : "{vitesse},{valeur},{acc}".
    return recette.gabarit.replace(/\{(\w+)\}/g, (_, jeton) => {
        if (jeton === 'valeur') { return String(valeur); }
        const c = (recette.constantes || {})[jeton];
        return c === undefined ? '' : String(c);
    });
}

function lireDansTable(recette, valBrute) {
    const cle = String(valBrute).trim().toLowerCase();
    const v = recette.table ? recette.table[cle] : undefined;
    return v === undefined ? null : v;
}

function lireNombre(recette, valBrute) {
    const n = recette.nombre || {};
    const brut = String(valBrute).trim();

    // Rien de capté : on retombe sur le défaut de la recette s'il existe.
    if (brut === '' && n.defaut !== undefined) { return n.defaut; }

    // parseFloat et non Number : tolère « 90 degrés » en ne lisant que 90.
    // Le signe éventuel vient de SE (négatif = gauche) et doit être conservé.
    const x = parseFloat(brut.replace(',', '.'));
    if (!isFinite(x)) { return null; }

    const y = (n.entier === false) ? x : Math.round(x);
    if (n.min !== undefined && y < n.min) { return null; }
    if (n.max !== undefined && y > n.max) { return null; }
    return y;
}

function resoudre(recette, valBrute) {
    if (recette.actif === false) {
        return { ok: false, raison: recette.erreur || 'recette_inactive' };
    }

    let valeur = null;

    if (recette.type === 'table') {
        valeur = lireDansTable(recette, valBrute);

    } else if (recette.type === 'nombre') {
        valeur = lireNombre(recette, valBrute);

    } else if (recette.type === 'table_ou_nombre') {
        // L'adverbe d'abord (« fort », « doux »), le nombre sinon.
        valeur = lireDansTable(recette, valBrute);
        if (valeur === null) { valeur = lireNombre(recette, valBrute); }

    } else {
        return { ok: false, raison: 'type_de_recette_inconnu' };
    }

    if (valeur === null) {
        return { ok: false, raison: recette.erreur || 'valeur_invalide' };
    }
    return { ok: true, valeur: appliquerGabarit(recette, valeur) };
}

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 4 — Capsule à 6 champs : résolution côté SP
// ─────────────────────────────────────────────────────────────────────────────
if (parts.length === 6) {
    const [id_traitement, iCat, iVar, iProp, iInst, valBrute] = parts;

    const registre = global.get('RZ_Traitements');

    // Le registre manque : l'init n'a pas été joué. On le dit clairement
    // plutôt que de laisser croire à une commande mal prononcée.
    if (!registre) {
        node.error('[STT Router] global.RZ_Traitements absent — fn_init_traitements '
            + 'n\'a pas ete joue. Capsule refusee (id_requete=' + id_requete + ')');
        msg.ack = { var: 'STT', prop: 'intent', id_requete,
                    statut: 'FAIL', raison: 'registre_absent' };
        return [null, null, msg];
    }

    const recette = registre[id_traitement];

    if (!recette) {
        node.warn('[STT Router] id_traitement inconnu du registre : ' + id_traitement
            + ' (id_requete=' + id_requete + ')');
        msg.ack = { var: 'STT', prop: 'intent', id_requete,
                    statut: 'FAIL', raison: 'resolution_non_implementee' };
        return [null, null, msg];
    }

    const res = resoudre(recette, valBrute);

    if (!res.ok) {
        node.warn('[STT Router] ' + id_traitement + ' : echec « ' + res.raison
            + ' » sur la valeur captee « ' + valBrute + ' »');
        msg.ack = { var: 'STT', prop: 'intent', id_requete,
                    statut: 'FAIL', raison: res.raison };
        return [null, null, msg];
    }

    msg.vpiv = { cat: iCat, var: iVar, prop: iProp, inst: iInst, val: res.valeur };
    msg.origine = 'STT';
    msg.topic = 'stt_intent';
    msg.stt_id_requete = id_requete;

    node.log('[STT Router] Intent résolu (' + id_traitement + ') → '
        + iVar + '.' + iProp + '=' + res.valeur + ' (mot capté: ' + valBrute + ')');

    return [msg, null, null];
}

// ─────────────────────────────────────────────────────────────────────────────
// SECTION 5 — Capsule mal formée
// ─────────────────────────────────────────────────────────────────────────────
node.warn('[STT Router] Intent mal formé — 5 ou 6 champs attendus, reçu '
    + parts.length + ' : ' + innerRaw);
return [null, null, null];
