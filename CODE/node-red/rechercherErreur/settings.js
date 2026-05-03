   // CORRECTION MAX LISTENERS - AJOUTEZ CETTE LIGNE
   require('events').EventEmitter.defaultMaxListeners = 25;

module.exports = {
    // Configuration de base
    uiPort: process.env.PORT || 1880,
    flowFile: 'flows.json',
    flowFilePretty: true,
    credentialSecret: "thLEga99?",
    // Configuration du stockage des variables globales - ok  par defaut :
    // data/data/com.termux/files/home/.node-red/context/file
contextStorage: {
    default: { module: "memory" },
    file: { module: "localfilesystem" }
},

    // Configuration de l'éditeur
    editorTheme: {
        projects: {
            enabled: false
        }
    },

    // Configuration des nœuds
    functionGlobalContext: {},

    // Configuration du tableau de bord
    ui: { path: "ui" },

    // Configuration du logging (original)
    logging: {
        console: {
            level: "info",
            metrics: false,
            audit: false
        }
    },


    /* Diagnostic détaillé - À AJOUTER Provisoirement avec analyse events qui suit
    logging: {
       console: {
            level: "debug",  // ← Changer de "info" à "debug"
            metrics: true,
            audit: true
         }
     },

    // OU plus spécifique aux events :
    functionGlobalContext: {
    eventDebug: function() {
        const emitter = require('events');
        console.log("MaxListeners actuels:", emitter.defaultMaxListeners);
        console.log("Détail des écouteurs...");
       }
     },
     */

    // Configuration MQTT
    mqttReconnectTime: 15000,

    // Configuration des modules externes
    externalModules: {},

    // Configuration HTTP (si nécessaire)
    httpNodeCors: {
        origin: "*",
        methods: "GET,PUT,POST,DELETE"
    }
};