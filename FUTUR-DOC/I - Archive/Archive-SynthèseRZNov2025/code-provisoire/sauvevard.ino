// Sauvegarde des modifications
const storeFile = "file";
let enrichedVars = global.get("enrichedVars", storeFile) || {};
let formData = msg.payload || {};

if (!formData || !formData.nomVarG) {
    msg.info = "❌ Données invalides.";
    return null;
}

// Vérification et renommage de "valeurCourante" si nécessaire
if (formData.instances && formData.instances.length > 1 && formData.instances.includes("valeurCourante")) {
    let index = 0;
    let newInstanceName = `instance${index}`;
    while (formData.instances.includes(newInstanceName)) {
        index++;
        newInstanceName = `instance${index}`;
    }
    // Renommer "valeurCourante" en "instanceX"
    let valeursCourantes = {};
    formData.proprietesByInstance.proprietes.forEach(prop => {
        if (prop.valeurs && prop.valeurs.valeurCourante) {
            valeursCourantes[prop.nom] = prop.valeurs.valeurCourante;
        }
    });

    // Mettre à jour les instances
    let newInstances = formData.instances.map(instance => instance === "valeurCourante" ? newInstanceName : instance);
    formData.instances = newInstances;

    // Mettre à jour les valeurs des propriétés
    formData.proprietesByInstance.proprietes.forEach(prop => {
        if (prop.valeurs && prop.valeurs.valeurCourante) {
            prop.valeurs[newInstanceName] = prop.valeurs.valeurCourante;
            delete prop.valeurs.valeurCourante;
        }
    });
}

const varName = formData.nomVarG;
enrichedVars[varName] = formData;
global.set("enrichedVars", enrichedVars, storeFile);

msg.payload = enrichedVars[varName];
msg.info = `💾 Variable '${varName}' sauvegardée.`;
return msg;
