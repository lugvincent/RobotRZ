// 🔹 prepareTableauProprietes (version dynamique pour ui-template)
/// Préparation des données pour le tableau dynamique des propriétés
// Préparation des données pour le tableau dynamique des propriétés

let varSelected = flow.get("varSelected") || null;
let enrichedVars = global.get("enrichedVars", "file") || {};
let variable = enrichedVars[varSelected] || {};
let proprietes = variable.proprietes || [];
let instances = variable.instances || ["*"];

// Vérifie si une variable est sélectionnée
if (!varSelected || !enrichedVars[varSelected]) {
    msg.payload = {
        proprietes: [],
        instances: []
    };
    msg.info = "⚠️ Aucune variable sélectionnée ou variable introuvable.";
    return msg;
}


// Envoie les données au template
msg.payload = {
    proprietes,
    instances
};

msg.info = `📋 Tableau des propriétés pour "${varSelected}" chargé (${proprietes.length} propriétés).`;
return msg;
