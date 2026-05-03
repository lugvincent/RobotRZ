// === prepareTemplate ===
// Objectif : transformer la variable enrichie stockée en contexte
// en une structure cohérente et nettoyée pour le template d’édition.

let varName = flow.get("varSelected") || msg.payload || null;
if (!varName) {
    node.warn("prepareTemplate: aucune variable sélectionnée !");
    msg.payload = null;
    return msg;
}

let enrichedVars = global.get("enrichedVars", "file") || {};
let variable = enrichedVars[varName];

// Cas variable non trouvée
if (!variable) {
    msg.payload = null;
    msg.info = "❌ Variable non trouvée";
    return msg;
}

// Helpers
function defaultForType(t) {
    switch (t) {
        case "boolean": return false;
        case "int":
        case "number": return 0;
        case "array": return [];
        case "object": return {};
        case "date": return new Date().toISOString().slice(0, 10);
        default: return "";
    }
}

// 1) Construire la liste des instances
let instances = Array.isArray(variable.instances) && variable.instances.length > 0
    ? variable.instances.filter(i => (typeof i === "string") && i && !i.startsWith("_"))
    : ["*"];

// 2) Construire les propriétés
let proprietes = [];

// Vérifie si la variable a des propriétés
if (variable.proprietes) {
    proprietes = variable.proprietes.map(p => {
        const type = p.type || "string";
        const valeurs = {};
        instances.forEach(inst => {
            valeurs[inst] = p.valeurs && p.valeurs[inst] !== undefined ? p.valeurs[inst] : defaultForType(type);
        });
        return {
            nom: p.nom || "",
            abbr: p.abbr || "",
            type: type,
            valeurs: valeurs
        };
    });
} else if (variable.proprietesByInstance && variable.proprietesByInstance.proprietes) {
    proprietes = variable.proprietesByInstance.proprietes.map(p => {
        const type = p.type || "string";
        const valeurs = {};
        instances.forEach(inst => {
            valeurs[inst] = p.valeurs && p.valeurs[inst] !== undefined ? p.valeurs[inst] : defaultForType(type);
        });
        return {
            nom: p.nom || "",
            abbr: p.abbr || "",
            type: type,
            valeurs: valeurs
        };
    });
} else {
    node.warn(`Aucune propriété définie pour la variable ${varName}, tableau vide initialisé.`);
}

// 3) Préparer le payload final
msg.payload = {
    nomVarG: variable.nomVarG || varName,
    abbrVarG: variable.abbrVarG || "",
    instances: instances,
    proprietes: proprietes,
    descriptionVarG: variable.descriptionVarG || "",
    dateUpdateVarG: variable.dateUpdateVarG || new Date().toISOString()
};

// Ajouter les informations pour une nouvelle propriété
msg.newProp = {
    nom: "",
    abbr: "",
    type: "",
    initialValues: {}
};
instances.forEach(i => {
    msg.newProp.initialValues[i] = "";
});

msg.info = "✅ prepareTemplate terminé";
return msg;
