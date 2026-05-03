[
    {
        "id": "ef2b489e1af0dd26",
        "type": "tab",
        "label": "ui_VarG",
        "disabled": false,
        "info": "",
        "env": []
    },
    {
        "id": "bd221513f816eb9f",
        "type": "ui-text",
        "z": "ef2b489e1af0dd26",
        "group": "group-vars",
        "order": 1,
        "width": "4",
        "height": "1",
        "name": "info2TitreVarModif",
        "label": "🛠Variable slectionnée",
        "format": "{{msg.info}}",
        "layout": "row-left",
        "style": true,
        "font": "Arial Black,Arial Black,Gadget,sans-serif",
        "fontSize": "18",
        "color": "#0000ff",
        "wrapText": true,
        "className": "",
        "x": 2210,
        "y": 740,
        "wires": []
    },
    {
        "id": "e22335b8755633c9",
        "type": "link in",
        "z": "ef2b489e1af0dd26",
        "name": "gestion-VarG",
        "links": [
            "16644df1dfd047f1"
        ],
        "x": 905,
        "y": 980,
        "wires": [
            [
                "e259eee6063b5e00",
                "6f3592fb1e99f59f",
                "2b177fb64723af1b",
                "472f3fa2e3efce54",
                "782f432e1bbdebaf",
                "9f3b485d9a074a49"
            ]
        ]
    },
    {
        "id": "fbedac1463e35d42",
        "type": "ui-control",
        "z": "ef2b489e1af0dd26",
        "name": "afficheUiVarG",
        "ui": "930a8a5e70986346",
        "events": "change",
        "x": 2340,
        "y": 880,
        "wires": [
            [
                "9f3b485d9a074a49"
            ]
        ]
    },
    {
        "id": "6bfefdf9652669dc",
        "type": "ui-text",
        "z": "ef2b489e1af0dd26",
        "group": "ac208ef77b5e8aea",
        "order": 9,
        "width": "3",
        "height": "1",
        "name": "Info-1",
        "label": "Info: ",
        "format": "{{msg.payload}}",
        "layout": "row-left",
        "style": false,
        "font": "",
        "fontSize": 16,
        "color": "#717171",
        "wrapText": true,
        "className": "",
        "x": 2330,
        "y": 1120,
        "wires": []
    },
    {
        "id": "e259eee6063b5e00",
        "type": "change",
        "z": "ef2b489e1af0dd26",
        "name": "",
        "rules": [
            {
                "t": "set",
                "p": "info",
                "pt": "msg",
                "to": "Les vars globales sont stockées dans enrichedVars dans un fichier et dans defaut (mémoire vive) après synchronisation",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "topic",
                "pt": "msg",
                "to": "introAdmin",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload",
                "pt": "msg",
                "to": "null",
                "tot": "json"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 1320,
        "y": 980,
        "wires": [
            [
                "52ba0a2c10bc76ea"
            ]
        ]
    },
    {
        "id": "3dfd303bd3d5a912",
        "type": "change",
        "z": "ef2b489e1af0dd26",
        "name": "",
        "rules": [
            {
                "t": "move",
                "p": "info",
                "pt": "msg",
                "to": "payload",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2090,
        "y": 1120,
        "wires": [
            [
                "6bfefdf9652669dc"
            ]
        ]
    },
    {
        "id": "52ba0a2c10bc76ea",
        "type": "link out",
        "z": "ef2b489e1af0dd26",
        "name": "link out msg Info",
        "mode": "link",
        "links": [
            "957befb99599fd76"
        ],
        "x": 1825,
        "y": 980,
        "wires": []
    },
    {
        "id": "957befb99599fd76",
        "type": "link in",
        "z": "ef2b489e1af0dd26",
        "name": "link in msg Info",
        "links": [
            "52ba0a2c10bc76ea",
            "c61335cd1a74793b",
            "cb30f289efae39d0"
        ],
        "x": 1925,
        "y": 1120,
        "wires": [
            [
                "3dfd303bd3d5a912"
            ]
        ]
    },
    {
        "id": "6f3592fb1e99f59f",
        "type": "ui-button",
        "z": "ef2b489e1af0dd26",
        "group": "ac208ef77b5e8aea",
        "name": "Retour",
        "label": "Retour  ",
        "order": 7,
        "width": "1",
        "height": "1",
        "emulateClick": false,
        "tooltip": "",
        "color": "",
        "bgcolor": "",
        "className": "",
        "icon": "",
        "iconPosition": "left",
        "payload": "{\"tab\":\"P_Admin\"}",
        "payloadType": "json",
        "topic": "topic",
        "topicType": "msg",
        "buttonColor": "grey",
        "textColor": "",
        "iconColor": "",
        "enableClick": true,
        "enablePointerdown": false,
        "pointerdownPayload": "",
        "pointerdownPayloadType": "str",
        "enablePointerup": false,
        "pointerupPayload": "",
        "pointerupPayloadType": "str",
        "x": 1270,
        "y": 880,
        "wires": [
            [
                "fbedac1463e35d42"
            ]
        ]
    },
    {
        "id": "b352d63fa4025be9",
        "type": "comment",
        "z": "ef2b489e1af0dd26",
        "name": "Retour ",
        "info": "",
        "x": 1270,
        "y": 840,
        "wires": []
    },
    {
        "id": "69b8f58459a3fa9a",
        "type": "comment",
        "z": "ef2b489e1af0dd26",
        "name": "Info",
        "info": "",
        "x": 1270,
        "y": 920,
        "wires": []
    },
    {
        "id": "035b3b83b31e22f1",
        "type": "comment",
        "z": "ef2b489e1af0dd26",
        "name": "charge var G defaut depuis enrichedVars",
        "info": "",
        "x": 1380,
        "y": 1060,
        "wires": []
    },
    {
        "id": "60c7d7ead4bd21ba",
        "type": "comment",
        "z": "ef2b489e1af0dd26",
        "name": "Gestion Memoire",
        "info": "",
        "x": 1300,
        "y": 1100,
        "wires": []
    },
    {
        "id": "2b177fb64723af1b",
        "type": "ui-button",
        "z": "ef2b489e1af0dd26",
        "group": "ac208ef77b5e8aea",
        "name": "iniVarGlobalInMemory",
        "label": "Actualiser",
        "order": 8,
        "width": "1",
        "height": "1",
        "emulateClick": false,
        "tooltip": "",
        "color": "",
        "bgcolor": "",
        "className": "",
        "icon": "",
        "iconPosition": "left",
        "payload": "",
        "payloadType": "str",
        "topic": "iniVarGlobalInMemory",
        "topicType": "msg",
        "buttonColor": "",
        "textColor": "",
        "iconColor": "",
        "enableClick": true,
        "enablePointerdown": false,
        "pointerdownPayload": "",
        "pointerdownPayloadType": "str",
        "enablePointerup": false,
        "pointerupPayload": "",
        "pointerupPayloadType": "str",
        "x": 1330,
        "y": 1140,
        "wires": [
            [
                "d3d1f28d4ef5b057"
            ]
        ]
    },
    {
        "id": "c61335cd1a74793b",
        "type": "link out",
        "z": "ef2b489e1af0dd26",
        "name": "link out 10",
        "mode": "link",
        "links": [
            "957befb99599fd76"
        ],
        "x": 1775,
        "y": 1120,
        "wires": []
    },
    {
        "id": "a663d1f0def2f3c5",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "prepareTableauVars",
        "func": "// function prepareTableauVars (corrigée pour nouvelle structure enrichedVars)\n// On ne lit QUE la RAM (default). \n// La RAM est peuplée au démarrage par SF_Init_1 et mise à jour par les formulaires.\nlet enrichedVars = global.get(\"enrichedVars\") || {}; \n\nlet tableauPourUI = [];\nflow.set(\"varSelected\", \"\"); // Reset de la sélection à l'affichage du tableau\n\nObject.entries(enrichedVars).forEach(([nomVarG, e]) => {\n    if (!nomVarG || !e || typeof e !== \"object\") return;\n\n    let props = e.proprietes || [];\n    let proprieteResume = props.length > 0\n        ? props.map(p => `${p.nomP} (${p.typeP})`).join(\", \")\n        : \"(Aucune propriété)\";\n\n    tableauPourUI.push({\n        nomVarG: nomVarG,\n        nomCpletVarG: e.nomCpletVarG || \"\",\n        instances: (e.instances || [\"*\"]).join(\", \"),\n        proprietes: proprieteResume,\n        description: e.descriptionVarG || \"\"\n    });\n});\n\nmsg.payload = tableauPourUI;\nreturn msg;",
        "outputs": 1,
        "timeout": "",
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1920,
        "y": 1220,
        "wires": [
            [
                "5d63c40d5f610447"
            ]
        ]
    },
    {
        "id": "5d63c40d5f610447",
        "type": "ui-table",
        "z": "ef2b489e1af0dd26",
        "group": "2413c080e8f28938",
        "name": "Table enrichedVars",
        "label": "",
        "order": 1,
        "width": 0,
        "height": 0,
        "maxrows": "",
        "autocols": true,
        "showSearch": true,
        "deselect": false,
        "selectionType": "click",
        "columns": [
            {
                "title": "Nom",
                "key": "nomVarG",
                "keyType": "key",
                "type": "text",
                "width": ""
            },
            {
                "title": "Type",
                "key": "typeVarG",
                "keyType": "key",
                "type": "text",
                "width": ""
            },
            {
                "title": "Instances",
                "key": "instances",
                "keyType": "key",
                "type": "text",
                "width": ""
            },
            {
                "title": "Propriétés",
                "key": "proprietes",
                "keyType": "key",
                "type": "text",
                "width": ""
            },
            {
                "title": "Description",
                "key": "description",
                "keyType": "key",
                "type": "text",
                "width": ""
            },
            {
                "title": "Détails",
                "key": "action",
                "keyType": "key",
                "type": "button",
                "width": ""
            }
        ],
        "mobileBreakpoint": "sm",
        "mobileBreakpointType": "defaults",
        "action": "replace",
        "x": 2370,
        "y": 1220,
        "wires": [
            [
                "10f35bc01001cf58"
            ]
        ]
    },
    {
        "id": "472f3fa2e3efce54",
        "type": "ui-button",
        "z": "ef2b489e1af0dd26",
        "group": "group-vars",
        "name": "ajouterUneVarG",
        "label": "➕ New varG ",
        "order": 8,
        "width": "1",
        "height": "1",
        "emulateClick": false,
        "tooltip": "",
        "color": "",
        "bgcolor": "",
        "className": "",
        "icon": "",
        "iconPosition": "left",
        "payload": "",
        "payloadType": "str",
        "topic": "newVarG",
        "topicType": "msg",
        "buttonColor": "blue",
        "textColor": "",
        "iconColor": "",
        "enableClick": true,
        "enablePointerdown": false,
        "pointerdownPayload": "",
        "pointerdownPayloadType": "str",
        "enablePointerup": false,
        "pointerupPayload": "",
        "pointerupPayloadType": "str",
        "x": 1300,
        "y": 1400,
        "wires": [
            [
                "10f35bc01001cf58",
                "3551f2f21ae9192e"
            ]
        ]
    },
    {
        "id": "b3d7294e565394e5",
        "type": "ui-text",
        "z": "ef2b489e1af0dd26",
        "group": "ac208ef77b5e8aea",
        "order": 1,
        "width": "5",
        "height": "1",
        "name": "TitreEntete",
        "label": "Tableau des variables - Choisir une action, ou selectionner une variable",
        "format": "{{msg.payload}}",
        "layout": "row-left",
        "style": true,
        "font": "Arial Black,Arial Black,Gadget,sans-serif",
        "fontSize": "18",
        "color": "#0000ff",
        "wrapText": true,
        "className": "",
        "x": 2330,
        "y": 1020,
        "wires": []
    },
    {
        "id": "782f432e1bbdebaf",
        "type": "change",
        "z": "ef2b489e1af0dd26",
        "name": "",
        "rules": [
            {
                "t": "set",
                "p": "payload",
                "pt": "msg",
                "to": "\"\"",
                "tot": "str"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 1310,
        "y": 1020,
        "wires": [
            [
                "b3d7294e565394e5"
            ]
        ]
    },
    {
        "id": "dd2b7c16f4b733b5",
        "type": "ui-text",
        "z": "ef2b489e1af0dd26",
        "group": "group-vars",
        "order": 9,
        "width": "4",
        "height": "1",
        "name": "info2TitreVarModif",
        "label": "🛠Variable slectionnée",
        "format": "{{msg.info}}",
        "layout": "row-left",
        "style": true,
        "font": "Arial Black,Arial Black,Gadget,sans-serif",
        "fontSize": "18",
        "color": "#0000ff",
        "wrapText": true,
        "className": "",
        "x": 2970,
        "y": 1540,
        "wires": []
    },
    {
        "id": "3551f2f21ae9192e",
        "type": "change",
        "z": "ef2b489e1af0dd26",
        "name": "valVarSelectInfo",
        "rules": [
            {
                "t": "set",
                "p": "payload",
                "pt": "msg",
                "to": "\"➕ Création d’une nouvelle variable.\"",
                "tot": "str"
            },
            {
                "t": "move",
                "p": "info",
                "pt": "msg",
                "to": "payload",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2740,
        "y": 1540,
        "wires": [
            [
                "dd2b7c16f4b733b5"
            ]
        ]
    },
    {
        "id": "2442a3544a9f6387",
        "type": "ui-form",
        "z": "ef2b489e1af0dd26",
        "name": "Form_Var_Modif",
        "group": "group-vars",
        "label": "",
        "order": 6,
        "width": 0,
        "height": 0,
        "options": [
            {
                "label": "Nom",
                "key": "nomVarG",
                "type": "text",
                "required": true,
                "rows": null
            },
            {
                "label": "Nom complet",
                "key": "nomCpletVarG",
                "type": "text",
                "required": false,
                "rows": null
            },
            {
                "label": "Instances (csv), toujours une instance par défaut *",
                "key": "instances",
                "type": "text",
                "required": false,
                "rows": null
            },
            {
                "label": "Description",
                "key": "descriptionVarG",
                "type": "multiline",
                "required": false,
                "rows": 2
            },
            {
                "label": "Date MAJ",
                "key": "dateUpdateVarG",
                "type": "text",
                "required": false,
                "rows": null
            }
        ],
        "formValue": {
            "nomVarG": "",
            "nomCpletVarG": "",
            "instances": "",
            "descriptionVarG": "",
            "dateUpdateVarG": ""
        },
        "submit": "Enregistrer",
        "cancel": "Annuler",
        "resetOnSubmit": true,
        "topic": "",
        "topicType": "str",
        "splitLayout": true,
        "className": "",
        "dropdownOptions": [],
        "x": 2360,
        "y": 1380,
        "wires": [
            [
                "1faa680740ccd411"
            ]
        ]
    },
    {
        "id": "1faa680740ccd411",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "InfosSauvegardeVars",
        "func": "const nodeFs = global.get('nodeFs');\nconst filePath = '/data/data/com.termux/files/home/.node-red/globalSave.json';\n\nlet enrichedVars = global.get(\"enrichedVars\") || {};\nlet varSelected = flow.get(\"varSelected\") || null;\nlet formData = msg.payload;\n\nif (!formData || !formData.nomVarG) {\n    msg.info = \"❌ Formulaire incomplet.\";\n    return null;\n}\n\nlet newName = formData.nomVarG.trim();\nlet oldName = varSelected;\n\n// --- LOGIQUE DE TRAITEMENT (Identique à la vôtre) ---\nif (!enrichedVars[oldName]) {\n    // Création...\n    if (enrichedVars[newName]) { msg.info = \"❌ Existe déjà\"; return null; }\n    enrichedVars[newName] = {\n        nomVarG: newName,\n        nomCpletVarG: formData.nomCpletVarG || \"\",\n        instances: formData.instances ? formData.instances.split(\",\").map(e => e.trim()) : [\"*\"],\n        descriptionVarG: formData.descriptionVarG || \"\",\n        dateUpdateVarG: new Date().toISOString(),\n        proprietes: [{ nomP: \"defautProp\", nomCpletP: \"def\", typeP: \"string\", valeurs: { \"*\": \"\" } }]\n    };\n    flow.set(\"varSelected\", newName);\n} else {\n    // Modification...\n    let current = enrichedVars[oldName];\n    if (oldName !== newName) {\n        if (enrichedVars[newName]) { msg.info = \"❌ Nom déjà pris\"; return null; }\n        enrichedVars[newName] = current;\n        delete enrichedVars[oldName];\n        flow.set(\"varSelected\", newName);\n    }\n    \n    let newInstances = formData.instances ? formData.instances.split(\",\").map(e => e.trim()) : current.instances;\n    let oldInstances = current.instances || [];\n\n    (enrichedVars[newName].proprietes || []).forEach(prop => {\n        prop.valeurs = prop.valeurs || {};\n        newInstances.forEach(inst => {\n            if (!(inst in prop.valeurs)) {\n                // Correction : Utilisation de typeP pour la cohérence\n                switch (prop.typeP) {\n                    case \"boolean\": prop.valeurs[inst] = false; break;\n                    case \"int\": case \"number\": prop.valeurs[inst] = 0; break;\n                    default: prop.valeurs[inst] = \"\";\n                }\n            }\n        });\n        oldInstances.forEach(inst => { if (!newInstances.includes(inst)) delete prop.valeurs[inst]; });\n    });\n\n    enrichedVars[newName] = {\n        ...enrichedVars[newName],\n        nomVarG: newName,\n        nomCpletVarG: formData.nomCpletVarG || enrichedVars[newName].nomCpletVarG,\n        instances: newInstances,\n        descriptionVarG: formData.descriptionVarG || enrichedVars[newName].descriptionVarG,\n        dateUpdateVarG: new Date().toISOString()\n    };\n}\n\n// --- PROTECTION & PERSISTANCE ---\nif (!enrichedVars || Object.keys(enrichedVars).length === 0) {\n    node.error(\"🚨 SAUVEGARDE ANNULÉE : L'objet enrichedVars est vide !\");\n    return null;\n}\n\nglobal.set(\"enrichedVars\", enrichedVars); // Mise à jour RAM\n\ntry {\n    let dataToSave = { enrichedVars: enrichedVars, lastUpdate: new Date().toISOString() };\n    nodeFs.writeFileSync(filePath, JSON.stringify(dataToSave, null, 4), 'utf8');\n    node.warn(\"💾 Disque : globalSave.json mis à jour (Infos Générales)\");\n} catch (err) {\n    node.error(\"❌ Erreur Disque : \" + err.message);\n}\n\nmsg.payload = newName || oldName;\nmsg.info = `✅ Variable ${newName} synchronisée RAM/Disque`;\nreturn msg;",
        "outputs": 1,
        "timeout": "",
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2420,
        "y": 1440,
        "wires": [
            [
                "3551f2f21ae9192e",
                "c7ab3fbe7dfc40bd"
            ]
        ]
    },
    {
        "id": "10f35bc01001cf58",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "recupVarSelected",
        "func": "// 🔹 recupVarSelected\n// - Détermine si on sélectionne une variable existante ou si on crée une nouvelle\n// - Met à jour flow.varSelected\n// - Fournit msg.info et msg.payload adaptés\n\nlet enrichedVars = global.get(\"enrichedVars\") || {};\nlet listeVars = Object.keys(enrichedVars);\nlet varSelected = null;\n\n// 1) Cas bouton \"Ajouter une VarG\"\nif (msg.topic === \"newVarG\") {\n    varSelected = \"new\";\n}\n// 2) Cas sélection dans la table\nelse {\n    varSelected = msg.payload?.row?.nomVarG || msg.payload?.nomVarG || null;\n}\n\n// 3) Vérification : aucune sélection\nif (!varSelected) {\n    msg.info = (listeVars.length === 0)\n        ? \"⚠️ Aucune variable existante dans enrichedVars.\"\n        : \"⚠️ Aucune variable sélectionnée pour édition.\";\n    return msg;  // on sort proprement\n}\n\n// 4) Stockage de la sélection\nflow.set(\"varSelected\", varSelected);\n\n// 5) Message d'info\nmsg.info = (varSelected === \"new\")\n    ? \"➕ Création d'une nouvelle variable.\"\n    : `✅ Vous êtes sur la variable : ${varSelected}`;\n\n// 6) Retour\nmsg.payload = varSelected;\nreturn msg;\n",
        "outputs": 1,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1770,
        "y": 1400,
        "wires": [
            [
                "3551f2f21ae9192e",
                "0a28b382b946e407"
            ]
        ]
    },
    {
        "id": "36cdacc4ec326fd5",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "prepareFormVarSelected",
        "func": "// 🔹 prepareFormVarSelected\n// - Prépare le formulaire Form_Var_Modif selon varSelected\n// - Gère : création nouvelle variable, édition variable existante, cas vide\n\nlet enrichedVars = global.get(\"enrichedVars\", \"file\") || {};\nlet varSelected = flow.get(\"varSelected\") || null;\n\n// 1) Cas création d'une nouvelle variable\nif (varSelected === \"new\") {\n    msg.payload = {\n        nomVarG: \"\",\n        nomCpletVarG: \"\",\n        instances: \"*\",  // instance par défaut obligatoire\n        descriptionVarG: \"- Objectif(s): - Origine: - Autres : \",\n        dateUpdateVarG: new Date().toISOString()\n    };\n    msg.topic = \"newVarG\";\n    msg.info = \"➕ Création d'une nouvelle variable.\";\n    return msg;\n}\n\n// 2) Cas édition d'une variable existante\nif (varSelected && enrichedVars[varSelected]) {\n    const selected = enrichedVars[varSelected];\n    msg.payload = {\n        nomVarG: selected.nomVarG || varSelected,\n        nomCpletVarG: selected.nomCpletVarG || \"\",\n        instances: selected.instances ? selected.instances.join(\", \") : \"*\",\n        descriptionVarG: selected.descriptionVarG || \"\",\n        dateUpdateVarG: selected.dateUpdateVarG || new Date().toISOString()\n    };\n    msg.info = `✏️ Édition de la variable '${varSelected}'.`;\n    return msg;\n}\n\n// 3) Cas aucun varSelected valide\nif (Object.keys(enrichedVars).length === 0) {\n    msg.info = \"⚠️ Aucune variable définie. Utilisez 'Ajouter une VarG' pour commencer.\";\n} else {\n    msg.info = \"⚠️ Aucune variable sélectionnée pour édition.\";\n}\n\n// Payload neutre pour éviter un crash du formulaire\nmsg.payload = {\n    nomVarG: \"\",\n    nomCpletVarG: \"\",\n    instances: \"\",\n    descriptionVarG: \"\",\n    dateUpdateVarG: new Date().toISOString()\n};\nreturn msg;\n\n",
        "outputs": 1,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2090,
        "y": 1380,
        "wires": [
            [
                "2442a3544a9f6387"
            ]
        ]
    },
    {
        "id": "c7ab3fbe7dfc40bd",
        "type": "change",
        "z": "ef2b489e1af0dd26",
        "name": "",
        "rules": [
            {
                "t": "move",
                "p": "info",
                "pt": "msg",
                "to": "payload",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2770,
        "y": 1620,
        "wires": [
            [
                "872e3000fa69583b"
            ]
        ]
    },
    {
        "id": "872e3000fa69583b",
        "type": "ui-text",
        "z": "ef2b489e1af0dd26",
        "group": "grp1",
        "order": 19,
        "width": "4",
        "height": 1,
        "name": "Info message",
        "label": "🛠Propriété  - ",
        "format": "{{msg.payload}}",
        "layout": "row-left",
        "style": true,
        "font": "Arial Black,Arial Black,Gadget,sans-serif",
        "fontSize": "14",
        "color": "#0000ff",
        "wrapText": false,
        "className": "",
        "x": 2980,
        "y": 1620,
        "wires": []
    },
    {
        "id": "6d978071061139e1",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "supprimerVarG",
        "func": "const nodeFs = global.get('nodeFs');\nconst filePath = '/data/data/com.termux/files/home/.node-red/globalSave.json';\n\nif (msg.payload === \"confirm_clicked\") {\n    const varToDelete = flow.get(\"pendingDeletion\");\n    let enrichedVars = global.get(\"enrichedVars\") || {};\n\n    if (varToDelete && enrichedVars[varToDelete]) {\n        // 1. Suppression RAM\n        delete enrichedVars[varToDelete];\n        global.set(\"enrichedVars\", enrichedVars);\n\n        // 2. Persistance Disque immédiate\n        try {\n            if (Object.keys(enrichedVars).length === 0) {\n                // Optionnel : protéger si on veut interdire de supprimer la DERNIÈRE variable\n                // ou simplement autoriser un fichier vide :\n                nodeFs.writeFileSync(filePath, JSON.stringify({ enrichedVars: {}, lastUpdate: new Date().toISOString() }, null, 4));\n            } else {\n                nodeFs.writeFileSync(filePath, JSON.stringify({ enrichedVars: enrichedVars, lastUpdate: new Date().toISOString() }, null, 4));\n            }\n            node.warn(`🗑️ Variable ${varToDelete} supprimée (RAM & Disque)`);\n        } catch (err) {\n            node.error(\"❌ Erreur lors de la suppression disque : \" + err.message);\n        }\n    }\n\n    flow.set(\"varSelected\", null);\n    msg.payload = \"refresh\";\n    return msg;\n}\nreturn null;",
        "outputs": 1,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2000,
        "y": 1280,
        "wires": [
            [
                "cb30f289efae39d0"
            ]
        ]
    },
    {
        "id": "ccd4033d5d0a98d4",
        "type": "ui-button",
        "z": "ef2b489e1af0dd26",
        "group": "group-vars",
        "name": "supprimerVarG",
        "label": "❌ Supprimer Var",
        "order": 7,
        "width": "1",
        "height": "1",
        "emulateClick": false,
        "tooltip": "Cliquez pour supprimer la variable sélectionnée",
        "color": "",
        "bgcolor": "",
        "className": "",
        "icon": "",
        "iconPosition": "left",
        "payload": "supprimer",
        "payloadType": "str",
        "topic": "demande-confirmation",
        "topicType": "msg",
        "buttonColor": "",
        "textColor": "red",
        "iconColor": "",
        "enableClick": true,
        "enablePointerdown": false,
        "pointerdownPayload": "",
        "pointerdownPayloadType": "str",
        "enablePointerup": false,
        "pointerupPayload": "",
        "pointerupPayloadType": "str",
        "x": 1300,
        "y": 1280,
        "wires": [
            [
                "608893f29afc3b8d"
            ]
        ]
    },
    {
        "id": "608893f29afc3b8d",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "prepaBoiteDialogue",
        "func": "// open-dialog -> Preparation d'une Boite de Dialogue Suppression\n// les actions dans ui-notification doivent être configuré selon un schéma précis\nconst varSelected = flow.get(\"varSelected\") || null;\n\nif (!varSelected || varSelected === \"none\" || varSelected === \"new\") {\n    msg.payload = {\n        type: \"warning\", \n        title: \"Aucune variable\",\n        text: \"Veuillez sélectionner une variable à supprimer.\",\n        duration: 3000\n    };\n    return msg;\n}\n\nflow.set(\"pendingDeletion\", varSelected);\n// gestion dynamique du ui-notification, les mots clefs de ui-update sont spécifique au noeud\nmsg.payload = {\n    type: \"warning\",\n    title: \"Confirmation suppression\",\n    text: `Voulez-vous vraiment supprimer la variable \"<strong>${varSelected}</strong>\" ?`,\n    displayTime: 0, // Ne pas fermer automatiquement\n    ui_update: {\n        allowConfirm: true,\n        allowDismiss: true,\n        confirmText: \"✅ Supprimer\",\n        dismissText: \"❌ Annuler\"\n    }\n};\n\nreturn msg;",
        "outputs": 1,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1530,
        "y": 1280,
        "wires": [
            [
                "335285a988544d9a"
            ]
        ]
    },
    {
        "id": "cb30f289efae39d0",
        "type": "link out",
        "z": "ef2b489e1af0dd26",
        "name": "link out 2",
        "mode": "link",
        "links": [
            "957befb99599fd76"
        ],
        "x": 2145,
        "y": 1280,
        "wires": []
    },
    {
        "id": "335285a988544d9a",
        "type": "ui-notification",
        "z": "ef2b489e1af0dd26",
        "ui": "930a8a5e70986346",
        "position": "bottom center",
        "colorDefault": true,
        "color": "#000000",
        "displayTime": "3",
        "showCountdown": true,
        "outputs": 1,
        "allowDismiss": true,
        "dismissText": "❌ Annuler",
        "allowConfirm": true,
        "confirmText": "✅ Supprimer",
        "raw": true,
        "className": "",
        "name": "notificationSuppression",
        "x": 1770,
        "y": 1280,
        "wires": [
            [
                "6d978071061139e1"
            ]
        ]
    },
    {
        "id": "9e1a3ca0e1d179d7",
        "type": "comment",
        "z": "ef2b489e1af0dd26",
        "name": "IMP exemple de notification",
        "info": "",
        "x": 1340,
        "y": 1240,
        "wires": []
    },
    {
        "id": "d3d1f28d4ef5b057",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "chargeEnrichedVarsToMemory",
        "func": "// 🔄 Fonctions utilitaires existantes\nconst storeDefault = \"default\";\nconst storeFile = \"file\";\n\n// 🔄 Valeur par défaut selon le type\nfunction defaultForType(type) {\n    switch (type) {\n        case \"boolean\": return false;\n        case \"int\": case \"number\": return 0;\n        case \"array\": return [];\n        case \"object\": return {};\n        case \"date\": return new Date().toISOString().slice(0, 10);\n        default: return \"\";\n    }\n}\n\n// 🔄 Conversion string → typé (avec gestion des valeurs par défaut)\nfunction convertFromString(value, type, defaultVal) {\n    if (value === null || value === undefined) {\n        return defaultVal !== undefined ? convertFromString(defaultVal, type) : defaultForType(type);\n    }\n    try {\n        switch (type) {\n            case \"int\": return parseInt(value, 10);\n            case \"number\": return parseFloat(value);\n            case \"boolean\": return (value === true || value === \"true\");\n            case \"string\": return String(value);\n            case \"array\": return Array.isArray(value) ? value : JSON.parse(value);\n            case \"object\": return typeof value === \"object\" ? value : JSON.parse(value);\n            case \"date\": return new Date(value);\n            default: return value;\n        }\n    } catch (err) {\n        node.error(`Erreur de conversion (${type}): ${err.message} (valeur: ${value})`, msg);\n        return defaultForType(type);\n    }\n}\n\n// 🔄 Fonction principale : chargeEnrichedVarsToMemory\nfunction chargeEnrichedVarsToMemory() {\n    // Supprimer enrichedVars du stockage par défaut\n    global.set(\"enrichedVars\", undefined, storeDefault);\n\n    // Charger enrichedVars depuis le fichier\n    let enrichedVars = global.get(\"enrichedVars\", storeFile) || {};\n    if (Object.keys(enrichedVars).length === 0) {\n        node.error(\"⚠️ Aucune donnée enrichedVars trouvée dans le stockage fichier.\", msg);\n        node.status({ fill: \"red\", shape: \"ring\", text: \"Échec : données manquantes\" });\n        return false;\n    }\n\n    // Parcourir chaque variable dans enrichedVars\n    Object.entries(enrichedVars).forEach(([key, e]) => {\n        const instances = e.instances || [\"*\"];\n        const props = e.proprietes || [];\n        let variable = {};\n\n        // Parcourir les instances\n        instances.forEach(instance => {\n            variable[instance] = {};\n            props.forEach(prop => {\n                const name = prop.nomP;\n                const type = prop.typeP;\n                const rawValue = prop.valeurs?.[instance];\n                const defaultVal = prop.valeurs?.[\"*\"];\n                variable[instance][name] = convertFromString(rawValue, type, defaultVal);\n            });\n        });\n\n        // Ajouter les métadonnées (noms complets, etc.)\n        if (e.nomCpletVarG) variable._nomCpletVarG = e.nomCpletVarG;\n        if (e.indexInstances) variable._indexInstances = e.indexInstances;\n\n        // Stocker dans les variables globales\n        if (instances.length === 1 && instances[0] === \"*\") {\n            global.set(key, variable[\"*\"], storeDefault);\n        } else {\n            global.set(key, variable, storeDefault);\n        }\n    });\n\n    node.status({ fill: \"green\", shape: \"dot\", text: \"✅ enrichedVars chargé en mémoire\" });\n    node.send({ payload: \"Chargement terminé\" });\n    return true;\n}\n\n// 🔄 Appel de la fonction\nchargeEnrichedVarsToMemory();\nmsg.info = \"✅ Initialisation enrichie depuis enrichedVars (file) terminée.\";\nreturn msg;\n",
        "outputs": 1,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1590,
        "y": 1140,
        "wires": [
            [
                "c61335cd1a74793b",
                "a663d1f0def2f3c5"
            ]
        ],
        "info": "// Les variables sont stockées dans deux types de stockage : \"default\" et \"file\".\r\n// Ce nœud synchronise les variables globales avec enrichedVars.\r\n// Il met à jour enrichedVars avec les variables globales actuelles\r\n// et s'assure que chaque variable suit la nouvelle structure définie.\r\n\r\n// Les variables sont structurées avec des propriétés comme nomVarG, instances, propriétés, etc.\r\n// La fonction gère également la restauration des variables globales manquantes depuis enrichedVars.\r\n// Elle prend en compte les instances et les sous-variables pour une synchronisation cohérente."
    },
    {
        "id": "551b4d23c8a911e9",
        "type": "ui-template",
        "z": "ef2b489e1af0dd26",
        "group": "grp1",
        "page": "",
        "ui": "",
        "name": "Éditeur Base (avec Schéma provisoireV2)",
        "order": 14,
        "width": 12,
        "height": "auto",
        "format": "<template>\n    <div v-if=\"msg && msg.topic==='openEditor' && msg.editor==='base'\"\n        style=\"border:1px solid #ddd;padding:15px;background:#fff;margin-top:12px;border-radius:6px;\">\n        <h4 style=\"margin-bottom:15px;\">{{originalName?'✏️ Édition':'➕ Nouvelle propriété'}}</h4>\n\n        <!-- Formulaire principal sur 2 colonnes -->\n        <div style=\"display:grid;grid-template-columns:1fr 1fr;gap:15px;\">\n            <!-- Colonne gauche -->\n            <div>\n                <div style=\"margin-bottom:12px;\">\n                    <label style=\"display:block;font-weight:600;margin-bottom:4px;color:#333;\">Nom</label>\n                    <input v-model=\"local.nomP\" style=\"width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:14px;\" />\n                </div>\n\n                <div style=\"margin-bottom:12px;\">\n                    <label style=\"display:block;font-weight:600;margin-bottom:4px;color:#333;\">Type</label>\n                    <select v-model=\"local.typeP\" style=\"width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:14px;background:white;\">\n            <option>string</option>\n            <option>integer</option>\n            <option>number</option>\n            <option>boolean</option>\n            <option>array</option>\n            <option>object</option>\n          </select>\n                </div>\n\n                <!-- Option pour les listes de valeurs -->\n                <div style=\"margin-bottom:12px;\">\n                    <label style=\"display:block;font-weight:600;margin-bottom:4px;color:#333;\">\n                        <input type=\"checkbox\" v-model=\"local.useValueList\" @change=\"toggleValueList\" />\n                        Utiliser une liste de valeurs prédéfinies\n                    </label>\n                </div>\n            </div>\n\n            <!-- Colonne droite -->\n            <div>\n                <div style=\"margin-bottom:12px;\">\n                    <label style=\"display:block;font-weight:600;margin-bottom:4px;color:#333;\">Nom complet</label>\n                    <input v-model=\"local.nomCpletP\" style=\"width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:14px;\" />\n                </div>\n\n                <!-- Champ valeur par défaut optionnel -->\n                <div style=\"margin-bottom:12px;\" v-if=\"local.defaut !== undefined\">\n                    <label style=\"display:block;font-weight:600;margin-bottom:4px;color:#333;\">Valeur par défaut</label>\n                    <input v-model=\"local.defaut\" style=\"width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:14px;\" />\n                </div>\n            </div>\n        </div>\n\n        <!-- Éditeur de liste de valeurs -->\n        <div v-if=\"local.useValueList\" style=\"margin-top:15px;border:1px solid #e0e0e0;padding:12px;border-radius:4px;background:#fafafa;\">\n            <h5 style=\"margin-bottom:10px;\">📝 Liste de valeurs</h5>\n            <div style=\"margin-bottom:10px;\">\n                <small style=\"color:#666;\">Les valeurs seront partagées entre toutes les instances</small>\n            </div>\n            <div v-for=\"(item, index) in local.valueList\" :key=\"index\" style=\"display:flex;gap:8px;margin-bottom:8px;align-items:center;\">\n                <input v-model=\"item.value\" \n                       placeholder=\"Valeur\"\n                       style=\"flex:1;padding:6px;border:1px solid #ddd;border-radius:3px;font-size:13px;\" />\n                <input v-model=\"item.label\" \n                       placeholder=\"Libellé (optionnel)\"\n                       style=\"flex:1;padding:6px;border:1px solid #ddd;border-radius:3px;font-size:13px;\" />\n                <button @click=\"removeValueListItem(index)\" \n                        style=\"background:#ff4444;color:white;border:none;padding:4px 8px;border-radius:3px;cursor:pointer;font-size:12px;\">\n                    🗑\n                </button>\n            </div>\n            <button @click=\"addValueListItem\" \n                    style=\"background:#4CAF50;color:white;border:none;padding:6px 12px;border-radius:4px;cursor:pointer;font-size:13px;\">\n                ➕ Ajouter une valeur\n            </button>\n        </div>\n\n        <!-- Éditeur de schéma amélioré -->\n        <div v-if=\"showSchemaEditor\"\n            style=\"margin-top:20px;border:1px solid #e0e0e0;padding:12px;border-radius:4px;background:#fafafa;\">\n            <h5 style=\"margin-bottom:10px;\">📑 Schéma (sous-propriétés)</h5>\n            <div style=\"display:grid;grid-template-columns:1fr 1fr;gap:10px;\">\n                <div v-for=\"(sp,i) in local.schemaArr\" :key=\"i\"\n                    style=\"border:1px solid #e8e8e8;padding:10px;border-radius:4px;background:white;\">\n                    <div style=\"margin-bottom:8px;\">\n                        <label style=\"display:block;font-size:0.85em;font-weight:600;margin-bottom:2px;color:#666;\">Nom</label>\n                        <input v-model=\"sp.nomSsP\" style=\"width:100%;padding:6px;border:1px solid #ddd;border-radius:3px;font-size:13px;\" />\n                    </div>\n                    <div style=\"margin-bottom:8px;\">\n                        <label style=\"display:block;font-size:0.85em;font-weight:600;margin-bottom:2px;color:#666;\">Type</label>\n                        <select v-model=\"sp.typeSsP\" style=\"width:100%;padding:6px;border:1px solid #ddd;border-radius:3px;font-size:13px;background:white;\">\n              <option>string</option>\n              <option>number</option>\n              <option>boolean</option>\n            </select>\n                    </div>\n                    <button @click=\"removeSchema(i)\" style=\"background:#ff4444;color:white;border:none;padding:4px 8px;border-radius:3px;cursor:pointer;font-size:12px;width:100%;\">\n            🗑 Supprimer\n          </button>\n                </div>\n            </div>\n            <button @click=\"addSchema\" style=\"margin-top:10px;background:#4CAF50;color:white;border:none;padding:8px 12px;border-radius:4px;cursor:pointer;\">\n        ➕ Ajouter sous-propriété\n      </button>\n        </div>\n\n        <!-- Boutons d'action -->\n        <div style=\"margin-top:20px;display:flex;gap:10px;\">\n            <button :disabled=\"!canSave\" @click=\"onSave\"\n              style=\"background:#2196F3;color:white;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;font-weight:600;\">\n        💾 Enregistrer\n      </button>\n            <button @click=\"onCancel\"\n              style=\"background:#f0f0f0;color:#333;border:1px solid #ccc;padding:10px 20px;border-radius:4px;cursor:pointer;\">\n        ↩ Annuler\n      </button>\n        </div>\n\n        <div v-if=\"lockedWarning\"\n            style=\"margin-top:12px;padding:8px;background:#ffeaa7;border:1px solid #fdcb6e;border-radius:4px;color:#b33;\">\n            ⚠️ Propriété verrouillée\n        </div>\n    </div>\n</template>\n\n<script>\n  export default {\n  data(){ return { local:null, schemaArr:[], originalName:null, lockedWarning:false }; },\n  computed:{\n    showSchemaEditor(){ return this.local&&(this.local.typeP==='object'||this.local.typeP==='array'); },\n    canSave(){\n      if(!this.local||!this.local.nomP||!this.local.nomCpletP||!this.local.typeP) return false;\n      if(this.showSchemaEditor) return this.local.schemaArr&&this.local.schemaArr.length>0&&this.local.schemaArr.every(s=>s.nomSsP);\n      if(this.local.useValueList) return this.local.valueList && this.local.valueList.length > 0 && this.local.valueList.every(item => item.value);\n      return true;\n    }\n  },\n  mounted(){\n    this.$watch('msg',(m)=>{\n      if(!m) return;\n      if(m.topic==='openEditor'&&m.editor==='base'){\n        this.originalName = m.originalName||null;\n        let p = m.prop||{};\n        \n        // INITIALISATION CORRIGÉE - assure que useValueList et valueList existent\n        let defaultProps = {\n          nomP: '', \n          nomCpletP: '', \n          typeP: null, \n          schema: [], \n          useValueList: false, \n          valueList: []\n        };\n        \n        this.local = JSON.parse(JSON.stringify({...defaultProps, ...p}));\n        this.local.schemaArr = Array.isArray(this.local.schema)?JSON.parse(JSON.stringify(this.local.schema)):[];\n        \n        // S'assurer que useValueList est un booléen\n        this.local.useValueList = !!this.local.useValueList;\n        \n        // S'assurer que valueList est un tableau\n        if(!Array.isArray(this.local.valueList)) {\n            this.local.valueList = [];\n        }\n        \n        this.lockedWarning = !!(p && p._locked);\n      }\n    },{immediate:true});\n  },\n  methods:{\n    toggleValueList() {\n        if (!this.local.useValueList) {\n            this.local.valueList = [];\n        }\n    },\n    addValueListItem() {\n        if(!this.local.valueList) this.local.valueList = [];\n        this.local.valueList.push({ value: '', label: '' });\n    },\n    removeValueListItem(index) {\n        this.local.valueList.splice(index, 1);\n    },\n    addSchema() {\n      if(!this.local.schemaArr) this.local.schemaArr = [];\n      let newName = 'newSSP';\n      if(this.local.schemaArr.some(s => s.nomSsP === newName)){\n        newName += '_1';\n      }\n      this.local.schemaArr.push({ nomSsP: newName, typeSsP: 'string' });\n    },\n    removeSchema(i){ this.local.schemaArr.splice(i,1); },\n    onSave(){\n      let propOut={\n        nomP:this.local.nomP,\n        nomCpletP:this.local.nomCpletP,\n        typeP:this.local.typeP,\n        useValueList: this.local.useValueList,\n        valueList: this.local.useValueList ? this.local.valueList : []\n      };\n      if(this.showSchemaEditor){ \n        propOut.schema=this.local.schemaArr; \n        this.send({topic:'saveBaseWithSchema',prop:propOut,originalName:this.originalName}); \n      }\n      else{ \n        this.send({topic:'saveBase',prop:propOut,originalName:this.originalName}); \n      }\n    },\n    onCancel(){ this.send({topic:'cancel'}); }\n  }\n}\n</script>",
        "storeOutMessages": true,
        "passthru": true,
        "resendOnRefresh": true,
        "templateScope": "local",
        "className": "",
        "x": 2560,
        "y": 2340,
        "wires": [
            [
                "bde6d1bdadbff192"
            ]
        ]
    },
    {
        "id": "1ff49a0479abc9e6",
        "type": "ui-template",
        "z": "ef2b489e1af0dd26",
        "group": "grp1",
        "page": "",
        "ui": "",
        "name": "Éditeur Valeurs (instances)",
        "order": 16,
        "width": 12,
        "height": "auto",
        "format": "<template>\n  <div v-if=\"msg && msg.topic==='openEditor' && msg.editor==='valeurs'\"\n    style=\"border:1px solid #ddd;padding:15px;background:#fff;margin-top:12px;border-radius:6px;\">\n    <h4 style=\"margin-bottom:15px;\">💾 Valeurs pour : {{propData.nomP}}</h4>\n\n    <!-- Indicateur de liste de valeurs -->\n    <div v-if=\"hasValueList\" style=\"margin-bottom:15px;padding:8px;background:#e3f2fd;border-radius:4px;\">\n      <small>📝 Utilise une liste de valeurs prédéfinies</small>\n    </div>\n\n    <!-- Valeur par défaut (*) -->\n    <div style=\"margin-bottom:20px;border:1px solid #e0e0e0;padding:12px;border-radius:4px;background:#f8f9fa;\">\n      <h5 style=\"margin-bottom:10px;\">⭐ Valeur par défaut (*)</h5>\n\n      <div v-if=\"isComplex\" style=\"display:grid;grid-template-columns:1fr 1fr;gap:10px;\">\n        <div v-for=\"(ssp,i) in schema\" :key=\"i\" style=\"margin-bottom:8px;\">\n          <label style=\"display:block;font-weight:600;margin-bottom:4px;color:#333;font-size:0.9em;\">\n            {{ssp.nomSsP}} ({{ssp.typeSsP}})\n          </label>\n\n          <select v-if=\"hasValueList && ssp.typeSsP==='string'\"\n                  v-model=\"local.valeurs['*'][ssp.nomSsP]\"\n                  style=\"width:100%;padding:6px;border:1px solid #ccc;border-radius:3px;font-size:13px;background:white;\">\n            <option value=\"\">-- Sélectionner --</option>\n            <option v-for=\"item in valueList\" :key=\"item.value\" :value=\"item.value\">\n              {{ item.label || item.value }}\n            </option>\n          </select>\n\n          <input v-else\n                 v-model=\"local.valeurs['*'][ssp.nomSsP]\"\n                 :type=\"ssp.typeSsP==='boolean' ? 'checkbox' : 'text'\"\n                 style=\"width:100%;padding:6px;border:1px solid #ccc;border-radius:3px;font-size:13px;\" />\n        </div>\n      </div>\n\n      <div v-else>\n        <select v-if=\"hasValueList && propData.typeP==='string'\"\n                v-model=\"local.valeurs['*']\"\n                style=\"width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:14px;background:white;\">\n          <option value=\"\">-- Sélectionner --</option>\n          <option v-for=\"item in valueList\" :key=\"item.value\" :value=\"item.value\">\n            {{ item.label || item.value }}\n          </option>\n        </select>\n\n        <input v-else\n               v-model=\"local.valeurs['*']\"\n               :type=\"propData.typeP==='boolean' ? 'checkbox' : 'text'\"\n               style=\"width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:14px;\" />\n      </div>\n    </div>\n\n    <!-- Valeurs par instance -->\n    <div v-for=\"inst in instances\" :key=\"inst\"\n      style=\"margin-bottom:15px;border:1px solid #e8e8e8;padding:12px;border-radius:4px;background:white;\">\n      <strong style=\"display:block;margin-bottom:10px;padding-bottom:5px;border-bottom:1px solid #eee;color:#2c3e50;\">{{inst}}</strong>\n\n      <div v-if=\"isComplex\" style=\"display:grid;grid-template-columns:1fr 1fr;gap:10px;\">\n        <div v-for=\"(ssp,i) in schema\" :key=\"i\" style=\"margin-bottom:8px;\">\n          <label style=\"display:block;font-weight:600;margin-bottom:4px;color:#333;font-size:0.9em;\">\n            {{ssp.nomSsP}} ({{ssp.typeSsP}})\n          </label>\n\n          <select v-if=\"hasValueList && ssp.typeSsP==='string'\"\n                  v-model=\"local.valeurs[inst][ssp.nomSsP]\"\n                  style=\"width:100%;padding:6px;border:1px solid #ccc;border-radius:3px;font-size:13px;background:white;\">\n            <option value=\"\">-- Sélectionner --</option>\n            <option v-for=\"item in valueList\" :key=\"item.value\" :value=\"item.value\">\n              {{ item.label || item.value }}\n            </option>\n          </select>\n\n          <input v-else\n                 v-model=\"local.valeurs[inst][ssp.nomSsP]\"\n                 :type=\"ssp.typeSsP==='boolean' ? 'checkbox' : 'text'\"\n                 style=\"width:100%;padding:6px;border:1px solid #ccc;border-radius:3px;font-size:13px;\" />\n        </div>\n      </div>\n\n      <div v-else>\n        <select v-if=\"hasValueList && propData.typeP==='string'\"\n                v-model=\"local.valeurs[inst]\"\n                style=\"width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:14px;background:white;\">\n          <option value=\"\">-- Sélectionner --</option>\n          <option v-for=\"item in valueList\" :key=\"item.value\" :value=\"item.value\">\n            {{ item.label || item.value }}\n          </option>\n        </select>\n\n        <input v-else\n               v-model=\"local.valeurs[inst]\"\n               :type=\"propData.typeP==='boolean' ? 'checkbox' : 'text'\"\n               style=\"width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;font-size:14px;\" />\n      </div>\n    </div>\n\n    <!-- Boutons d'action -->\n    <div style=\"margin-top:20px;display:flex;gap:10px;\">\n      <button @click=\"onSave\"\n              style=\"background:#2196F3;color:white;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;font-weight:600;\">\n        💾 Enregistrer\n      </button>\n      <button @click=\"onCancel\"\n              style=\"background:#f0f0f0;color:#333;border:1px solid #ccc;padding:10px 20px;border-radius:4px;cursor:pointer;\">\n        ↩ Annuler\n      </button>\n    </div>\n  </div>\n</template>\n\n<script>\n  export default {\n  data(){ \n    return { \n      local: {valeurs: {}}, \n      propData: {},\n      schema: [], \n      isComplex: false, \n      instances: [], \n      hasValueList: false,\n      valueList: []\n    }; \n  },\n  mounted(){ \n    this.$watch('msg',(m)=>{ \n      if(!m) return; \n      if(m.topic==='openEditor' && m.editor==='valeurs'){\n        console.log('Éditeur Valeurs - Message reçu:', m);\n        \n        this.propData = m.prop || {};\n        this.schema = (m.prop && m.prop.schema) ? JSON.parse(JSON.stringify(m.prop.schema)) : [];\n        this.isComplex = (m.prop && (m.prop.typeP==='object' || m.prop.typeP==='array')) || false;\n        this.instances = m.instances || [];\n        \n        // Initialisation des listes de valeurs\n        this.hasValueList = !!(m.prop && m.prop.useValueList && m.prop.valueList);\n        this.valueList = (m.prop && m.prop.valueList) || [];\n        \n        console.log('Données analysées:', {\n          hasValueList: this.hasValueList,\n          valueList: this.valueList,\n          propData: this.propData,\n          isComplex: this.isComplex\n        });\n        \n        // Initialisation des valeurs\n        this.local = {valeurs: {}};\n        \n        if(this.isComplex && this.schema.length){\n          // Propriété complexe (objet/array)\n          let def = {}; \n          this.schema.forEach(s => {\n            def[s.nomSsP] = (m.prop && m.prop.valeurs && m.prop.valeurs['*'] && m.prop.valeurs['*'][s.nomSsP] !== undefined)\n              ? m.prop.valeurs['*'][s.nomSsP]\n              : null;\n          });\n          this.local.valeurs['*'] = JSON.parse(JSON.stringify(def));\n          \n          this.instances.forEach(i => { \n            let instVal = {}; \n            this.schema.forEach(s => {\n              instVal[s.nomSsP] = (m.prop && m.prop.valeurs && m.prop.valeurs[i] && m.prop.valeurs[i][s.nomSsP] !== undefined)\n                ? m.prop.valeurs[i][s.nomSsP]\n                : null;\n            }); \n            this.local.valeurs[i] = instVal; \n          });\n        } else {\n          // Propriété simple\n          this.local.valeurs['*'] = (m.prop && m.prop.valeurs && m.prop.valeurs['*'] !== undefined)\n            ? m.prop.valeurs['*']\n            : null;\n          this.instances.forEach(i => { \n            this.local.valeurs[i] = (m.prop && m.prop.valeurs && m.prop.valeurs[i] !== undefined)\n              ? m.prop.valeurs[i]\n              : null; \n          });\n        }\n        \n        console.log('Valeurs initialisées:', this.local.valeurs);\n      } \n    }, {immediate: true}); \n  },\n  methods: {\n    onSave(){ \n      this.send({\n        topic: 'saveValues',\n        prop: {\n          nomP: this.propData.nomP,\n          valeurs: this.local.valeurs\n        }\n      }); \n    },\n    onCancel(){ this.send({topic: 'cancel'}); }\n  }\n}\n</script>",
        "storeOutMessages": true,
        "passthru": true,
        "templateScope": "local",
        "className": "",
        "x": 2520,
        "y": 2440,
        "wires": [
            [
                "bde6d1bdadbff192"
            ]
        ]
    },
    {
        "id": "a26935de4b0a7af2",
        "type": "ui-template",
        "z": "ef2b489e1af0dd26",
        "group": "grp1",
        "page": "",
        "ui": "",
        "name": "tableauProprietes (template dashboard2 , vue3)",
        "order": 18,
        "width": 12,
        "height": "auto",
        "format": "<template>\n  <div>\n    <h3>{{ msg.payload.variable.nomVarG || '' }}</h3>\n    <table style=\"width:100%;border-collapse:collapse;border:1px solid #cccccc;\">\n      <thead>\n        <tr style=\"background:#f8f9fa;\">\n          <th style=\"border:1px solid #cccccc;padding:8px;\">Nom</th>\n          <th style=\"border:1px solid #cccccc;padding:8px;\">Action</th>\n          <th style=\"border:1px solid #cccccc;padding:8px;\">Type</th>\n          <th style=\"border:1px solid #cccccc;padding:8px;\">Défaut (*)</th>\n          <th v-for=\"inst in msg.payload.instances\" :key=\"inst\" style=\"border:1px solid #cccccc;padding:8px;\">{{inst}}\n          </th>\n         </tr>\n      </thead>\n      <tbody>\n        <tr v-for=\"(p,idx) in msg.payload.proprietes\" :key=\"idx\" :style=\"p._isNew ? 'background:#fff8e1':''\">\n          <td style=\"border:1px solid #dddddd;padding:6px 8px;\">{{p.nomP}}</td>\n          <td style=\"border:1px solid #dddddd;padding:6px 8px;\">\n            <button v-if=\"p._isNew\" @click=\"openBase(p)\">➕ Définir</button>\n            <template v-else>\n              <button @click=\"openBase(p)\">📝 Base</button>\n              <button @click=\"openValues(p)\">✏️ Valeurs</button>\n              <button @click=\"askDelete(p)\">🗑️</button>\n            </template>\n          </td>\n          <td style=\"border:1px solid #dddddd;padding:6px 8px;\">{{p.typeP || '-'}}</td>\n          <td style=\"border:1px solid #dddddd;padding:6px 8px;\">{{ formatVal(p.defaut) }}</td>\n          <td v-for=\"inst in msg.payload.instances\" :key=\"inst\" style=\"border:1px solid #dddddd;padding:6px 8px;\">{{\n            formatVal(p.valeurs && p.valeurs[inst]) }}</td>\n        </tr>\n      </tbody>\n    </table>\n    <div style=\"margin-top:8px;color:#333;font-size:0.95em\">{{ msg.info }}</div>\n  </div>\n</template>\n<script>\n  export default {\n  methods: {\n    formatVal(v){ return v===null||v===undefined?'null':(typeof v==='object'?JSON.stringify(v):v); },\n    openBase(prop){ this.send({action:'base',prop}); },\n    openValues(prop){ this.send({action:'valeurs',prop}); },\n    askDelete(prop){ if(confirm('Supprimer '+prop.nomP+' ?')) this.send({action:'delete',prop}); }\n  }\n}\n</script>",
        "storeOutMessages": true,
        "passthru": true,
        "resendOnRefresh": true,
        "templateScope": "local",
        "className": "",
        "x": 2580,
        "y": 2240,
        "wires": [
            [
                "c7ab3fbe7dfc40bd",
                "54b1e67381ceed7f"
            ]
        ]
    },
    {
        "id": "54b1e67381ceed7f",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "routerAction",
        "func": "// Dans le noeud \"routerAction\" - version corrigée\nlet action = msg.action;\nlet prop = msg.prop || {};\nlet varSelected = flow.get('varSelected');\nlet enriched = global.get('enrichedVars', 'file') || {};\nlet variable = enriched[varSelected] || {};\nlet instances = (variable.instances || []).filter(i => i !== '*');\n\nif (action) flow.set('propSelected', prop && prop.nomP ? prop.nomP : null);\n\n// CORRECTION : Récupérer la propriété COMPLÈTE depuis la variable\nlet completeProp = prop;\nif (prop.nomP && !prop._isNew) {\n    // Chercher la propriété complète dans la variable\n    let foundProp = (variable.proprietes || []).find(p => p.nomP === prop.nomP);\n    if (foundProp) {\n        completeProp = foundProp;\n    }\n}\n\n// Gestion de l'éditeur Base (nouveau ou modification)\nif (action === 'base') {\n    // Si c'est une nouvelle propriété (_isNew = true), on envoie un objet vide\n    // Sinon, on envoie la propriété complète\n    let propData = prop._isNew ? {} : completeProp;\n    \n    return [{\n        topic: 'openEditor',\n        editor: 'base',\n        prop: propData,\n        instances: instances,\n        originalName: prop._isNew ? null : prop.nomP\n    }, null, null];\n}\n// Gestion de l'éditeur Valeurs\nelse if (action === 'valeurs') {\n    return [null, {\n        topic: 'openEditor',\n        editor: 'valeurs',\n        prop: completeProp,  // ← Propriété COMPLÈTE avec useValueList et valueList\n        instances: instances,\n        originalName: prop.nomP\n    }, null];\n}\nelse if (action === 'delete') { \n    return [null, null, { topic: 'delete', prop }]; \n}\nreturn null;",
        "outputs": 3,
        "timeout": "",
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1770,
        "y": 2340,
        "wires": [
            [
                "551b4d23c8a911e9"
            ],
            [
                "1ff49a0479abc9e6"
            ],
            [
                "bde6d1bdadbff192"
            ]
        ]
    },
    {
        "id": "1161314d400802c5",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "prepareTableauProprietes",
        "func": "// Prépare l'affichage du tableau des propriétés\nlet varSelected = flow.get('varSelected') || null;\nlet enrichedVars = global.get('enrichedVars', 'file') || {};\nlet variable = enrichedVars[varSelected] || {};\nlet propSelected = flow.get('propSelected') || null;\nlet infoAction = flow.get('infoAction') || null;\n\nconsole.log(\"prepareTableauProprietes - varSelected:\", varSelected);\n\n// Si aucune variable sélectionnée ou nouvelle variable\nif (!varSelected || varSelected === \"new\") {\n    msg.payload = {\n        proprietes: [],  // ← Tableau VIDE (pas de ligne \"NouvelleP\")\n        propSelected: null,\n        instances: [],\n        variable: { nomVarG: varSelected === \"new\" ? \"Nouvelle variable\" : \"Aucune variable sélectionnée\" }\n    };\n    msg.info = varSelected === \"new\"\n        ? \"Nouvelle variable - définissez d'abord la variable\"\n        : \"Sélectionnez une variable dans le tableau\";\n    return msg;\n}\n\n// Traitement normal pour variable existante\nif (msg.topic == \"initialisePropriete\") {\n    flow.set(\"propSelected\", null);\n}\n\nif (msg.topic == \"refresh\") {\n    flow.set(\"propSelected\", null);\n}\n\nlet proprietes = (variable.proprietes || []).map(p => ({\n    nomP: p.nomP,\n    nomCpletP: p.nomCpletP,\n    typeP: p.typeP,\n    schema: p.schema || null,\n    valeurs: p.valeurs || {},\n    defaut: (p.valeurs && p.valeurs['*'] !== undefined) ? p.valeurs['*'] : null,\n    useValueList: p.useValueList || false,\n    valueList: p.valueList || [],\n    _locked: p._locked || false\n}));\n\n// Ajouter une ligne pour nouvelle propriété UNIQUEMENT si variable existe\nif (varSelected && varSelected !== \"new\") {\n    proprietes.push({\n        nomP: 'NouvelleP',\n        nomCpletP: 'New',\n        typeP: null,\n        schema: null,\n        valeurs: { '*': null },\n        _isNew: true\n    });\n}\n\nmsg.payload = {\n    proprietes: proprietes,\n    propSelected: propSelected,\n    instances: (variable.instances || []).filter(i => i !== '*'),\n    variable: { nomVarG: variable.nomVarG || varSelected }\n};\n\nmsg.info = 'Variable:' + varSelected + ' - ' + infoAction + (propSelected ? propSelected : \"choisir une propriété\");\nreturn msg;",
        "outputs": 1,
        "timeout": "",
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1730,
        "y": 2180,
        "wires": [
            [
                "a26935de4b0a7af2"
            ]
        ]
    },
    {
        "id": "bde6d1bdadbff192",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "SauvResultatActions (save / delete handlers)",
        "func": "const nodeFs = global.get('nodeFs');\nconst filePath = '/data/data/com.termux/files/home/.node-red/globalSave.json';\n\nlet enriched = global.get('enrichedVars') || {};\nlet varSelected = flow.get('varSelected');\nif (!varSelected) return null;\n\nlet variable = enriched[varSelected] || {};\n\nswitch (msg.topic) {\n  case 'saveBase':\n  case 'saveBaseWithSchema': {\n    let p = msg.prop;\n    let idx = (variable.proprietes || []).findIndex(pr => pr.nomP === msg.originalName);\n    \n    let propToSave = {\n      nomP: p.nomP,\n      nomCpletP: p.nomCpletP,\n      typeP: p.typeP,\n      useValueList: !!p.useValueList,\n      valueList: Array.isArray(p.valueList) ? p.valueList : [],\n      valeurs: (idx >= 0) ? variable.proprietes[idx].valeurs : {} \n    };\n\n    // Initialisation des instances si nouvelle propriété\n    if (idx < 0 && variable.instances) {\n        variable.instances.forEach(inst => { propToSave.valeurs[inst] = null; });\n    }\n\n    if (p.schema) { propToSave.schema = p.schema; propToSave._locked = true; }\n\n    if (idx >= 0) { variable.proprietes[idx] = propToSave; } \n    else { (variable.proprietes = variable.proprietes || []).push(propToSave); }\n    break;\n  }\n  case 'saveValues': {\n    let pName = msg.prop.nomP;\n    let pIdx = (variable.proprietes || []).findIndex(pr => pr.nomP === pName);\n    if (pIdx >= 0) { variable.proprietes[pIdx].valeurs = msg.prop.valeurs; }\n    break;\n  }\n  case 'delete': {\n    variable.proprietes = (variable.proprietes || []).filter(pr => pr.nomP !== msg.prop.nomP);\n    break;\n  }\n}\n\n// --- PROTECTION & PERSISTANCE ---\nif (!enriched || Object.keys(enriched).length === 0) {\n    node.error(\"🚨 SAUVEGARDE ANNULÉE : Tentative d'écriture à vide dans SauvResultatActions\");\n    return null;\n}\n\nglobal.set('enrichedVars', enriched); // Sync RAM\n\ntry {\n    let dataToSave = { enrichedVars: enriched, lastUpdate: new Date().toISOString() };\n    nodeFs.writeFileSync(filePath, JSON.stringify(dataToSave, null, 4), 'utf8');\n    node.status({fill:\"green\", shape:\"dot\", text:\"Sync Disque OK\"});\n} catch (err) {\n    node.error(\"❌ Erreur Disque : \" + err.message);\n    node.status({fill:\"red\", shape:\"ring\", text:\"Erreur Disque\"});\n}\n\nmsg.topic = 'refresh';\nmsg.view = 'tableau';\nreturn msg;",
        "outputs": 1,
        "timeout": "",
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2990,
        "y": 2560,
        "wires": [
            [
                "1161314d400802c5"
            ]
        ]
    },
    {
        "id": "0a28b382b946e407",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "gestionSelectAutomatique",
        "func": "// Noeud \"Gestion Sélection Automatique\" - VERSION CORRIGÉE\nlet varSelected = msg.payload;\n\nconsole.log(\"Gestion Sélection Automatique - varSelected:\", varSelected);\n\nif (varSelected && varSelected !== \"new\") {\n    // Variable existante sélectionnée\n    flow.set('varSelected', varSelected);\n    console.log(\"Variable sélectionnée automatiquement:\", varSelected);\n    \n    // Préparer les messages pour les deux composants\n    let formMsg = {\n        topic: \"refresh\",\n        payload: varSelected,\n        info: `✅ Vous êtes sur la variable : ${varSelected}`\n    };\n    \n    let tableauMsg = {\n        topic: 'refresh',\n        view: 'tableau',\n        payload: varSelected\n    };\n    \n    return [formMsg, tableauMsg];\n    \n} else if (varSelected === \"new\") {\n    // Nouvelle variable\n    flow.set('varSelected', \"new\");\n    \n    // Préparer les messages pour les deux composants\n    let formMsg = {\n        topic: \"newVarG\",\n        info: \"➕ Création d'une nouvelle variable.\",\n        payload: {\n            nomVarG: \"\",\n            nomCpletVarG: \"\",\n            instances: \"*\",\n            descriptionVarG: \"- Objectif(s): - Origine: - Autres : \",\n            dateUpdateVarG: new Date().toISOString()\n        }\n    };\n    \n    let tableauMsg = {\n        topic: 'refresh',\n        view: 'tableau',\n        payload: \"new\"\n    };\n    \n    return [formMsg, tableauMsg];\n    \n} else {\n    // Aucune sélection\n    return [null, null];\n}",
        "outputs": 2,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2070,
        "y": 1440,
        "wires": [
            [
                "36cdacc4ec326fd5"
            ],
            [
                "1161314d400802c5"
            ]
        ]
    },
    {
        "id": "9f3b485d9a074a49",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "initialisePage",
        "func": "// Noeud \"Initialisation Page\"\n// S'exécute au chargement de la page pour reset l'état\n\n// Reset les sélections au chargement\nflow.set('varSelected', null);\nflow.set('propSelected', null);\nflow.set('infoAction', \"Sélectionnez une variable\");\n\n// Préparer un tableau vide\nmsg.payload = {\n    proprietes: [{\n        nomP: 'NouvelleP',\n        nomCpletP: 'New',\n        typeP: null,\n        schema: null,\n        valeurs: { '*': null },\n        _isNew: true\n    }],\n    propSelected: null,\n    instances: [],\n    variable: { nomVarG: \"Aucune variable sélectionnée\" }\n};\nmsg.info = \"Sélectionnez une variable dans le tableau\";\n\nreturn msg;",
        "outputs": 1,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 930,
        "y": 1100,
        "wires": [
            [
                "1161314d400802c5"
            ]
        ]
    },
    {
        "id": "bcaeb490b26f91e9",
        "type": "inject",
        "z": "ef2b489e1af0dd26",
        "name": "",
        "props": [
            {
                "p": "payload"
            },
            {
                "p": "topic",
                "vt": "str"
            }
        ],
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "topic": "",
        "payload": "",
        "payloadType": "date",
        "x": 1430,
        "y": 1600,
        "wires": [
            [
                "75d10583f4c3508e"
            ]
        ]
    },
    {
        "id": "75d10583f4c3508e",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "name": "function 2",
        "func": "// Voir ce qui existe dans le stockage fichier\nlet fileData = global.get(\"enrichedVars\", \"file\");\nnode.warn(\"Données dans le fichier: \" + JSON.stringify(fileData));\n\n// Voir tous les keys disponibles (si supporté)\ntry {\n    let keys = global.keys(\"file\");\n    node.warn(\"Clés disponibles dans le fichier: \" + JSON.stringify(keys));\n} catch(e) {\n    node.warn(\"keys() non supporté: \" + e.message);\n}",
        "outputs": 1,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1600,
        "y": 1580,
        "wires": [
            [
                "d17721af4c1497f3"
            ]
        ]
    },
    {
        "id": "d17721af4c1497f3",
        "type": "debug",
        "z": "ef2b489e1af0dd26",
        "name": "debug 7",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "statusVal": "",
        "statusType": "auto",
        "x": 1760,
        "y": 1580,
        "wires": []
    },
    {
        "id": "9e14fb82b120ac80",
        "type": "function",
        "z": "ef2b489e1af0dd26",
        "d": true,
        "name": "oldsupprimerVarG",
        "func": "/* Le nœud ui-notification envoie un message avec msg.payload contenant :\n   - \"confirm_clicked\" si l'utilisateur clique sur \"✅ Supprimer\".\n   - \"dismiss_clicked\" si l'utilisateur clique sur \"❌ Annuler\".\n*/\nif (msg.payload === \"confirm_clicked\") {\n    const varToDelete = flow.get(\"pendingDeletion\");\n\n    if (!varToDelete) {\n        msg.info = \"⚠️ Aucune variable en attente de suppression.\";\n        return msg;\n    }\n\n    // Suppression\n    let enrichedVars = global.get(\"enrichedVars\", \"file\") || {};\n    delete enrichedVars[varToDelete];\n    delete enrichedVars[\"\"];\n    global.set(\"enrichedVars\", enrichedVars, \"file\");\n    global.set(varToDelete, undefined, \"default\");\n    flow.set(\"varSelected\", \"none\");\n    flow.set(\"pendingDeletion\", null);\n\n    msg.payload = \"none\";\n    msg.info = `✅ Variable \"${varToDelete}\" supprimée.`;\n    msg.notification = {\n        type: \"success\",\n        title: \"✅ Supprimée\",\n        text: `Variable \"${varToDelete}\" supprimée avec succès.`,\n        duration: 3000\n    };\n\n} else if (msg.payload === \"dismiss_clicked\") {\n    flow.set(\"pendingDeletion\", null);\n    msg.info = \"❌ Suppression annulée.\";\n    msg.notification = {\n        type: \"info\",\n        title: \"❌ Annulée\",\n        text: \"Suppression annulée.\",\n        duration: 3000\n    };\n}\n\nreturn msg;\n",
        "outputs": 1,
        "timeout": 0,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2530,
        "y": 1300,
        "wires": [
            []
        ]
    },
    {
        "id": "4a7c400fd4b9e842",
        "type": "comment",
        "z": "ef2b489e1af0dd26",
        "name": "UI-varG cohérence-sauv",
        "info": "Synthèse de la cohérence finale de ton flow :\n\n    Source de Vérité : La variable globale enrichedVars (dans le store par défaut, donc la RAM).\n\n    Lectures UI : Tous les nœuds de préparation (prepareTableauVars, prepareForm, etc.) font un global.get(\"enrichedVars\").\n\n    Écritures : Les nœuds de sauvegarde (InfosSauvegardeVars, SauvResultatActions, supprimerVarG) font systématiquement deux actions :\n\n        global.set(\"enrichedVars\", ...) pour mettre à jour l'application instantanément.\n\n        nodeFs.writeFileSync(...) pour garantir que le robot n'oublie rien après un reboot.",
        "x": 1170,
        "y": 720,
        "wires": []
    },
    {
        "id": "201fbe0b3a912dec",
        "type": "inject",
        "z": "ef2b489e1af0dd26",
        "name": "",
        "props": [
            {
                "p": "payload"
            },
            {
                "p": "topic",
                "vt": "str"
            }
        ],
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "topic": "",
        "payload": "",
        "payloadType": "date",
        "x": 610,
        "y": 880,
        "wires": [
            [
                "2b177fb64723af1b"
            ]
        ]
    },
    {
        "id": "group-vars",
        "type": "ui-group",
        "name": "Variable selectionnée",
        "page": "page-1",
        "width": 6,
        "height": 4,
        "order": 3,
        "showTitle": false,
        "className": "",
        "visible": "true",
        "disabled": "false",
        "groupType": "default"
    },
    {
        "id": "930a8a5e70986346",
        "type": "ui-base",
        "name": "My Dashboard",
        "path": "/dashboard",
        "appIcon": "",
        "includeClientData": true,
        "acceptsClientConfig": [],
        "showPathInSidebar": false,
        "headerContent": "page",
        "navigationStyle": "default",
        "titleBarStyle": "default",
        "showReconnectNotification": true,
        "notificationDisplayTime": 1,
        "showDisconnectNotification": true,
        "allowInstall": true
    },
    {
        "id": "ac208ef77b5e8aea",
        "type": "ui-group",
        "name": "Accueil-AdminVarG",
        "page": "page-1",
        "width": 6,
        "height": 1,
        "order": 1,
        "showTitle": false,
        "className": "",
        "visible": "true",
        "disabled": "false",
        "groupType": "default"
    },
    {
        "id": "2413c080e8f28938",
        "type": "ui-group",
        "name": "GrpeTableVar",
        "page": "page-1",
        "width": 6,
        "height": 1,
        "order": 2,
        "showTitle": false,
        "className": "/* Bordures fines pour toutes les tables */ .ui-table {     border: 1px solid #e0e0e0 !important;     border-collapse: collapse !important; }  .ui-table th {     border: 1px solid #d0d0d0 !important;     background-color: #f8f9fa !important;     padding: 8px 12px !important;     font-weight: 600 !important; }  .ui-table td {     border: 1px solid #e8e8e8 !important;     padding: 6px 12px !important; }",
        "visible": "true",
        "disabled": "false",
        "groupType": "default"
    },
    {
        "id": "grp1",
        "type": "ui-group",
        "name": "Grpe_Propriete",
        "page": "page-1",
        "width": 6,
        "height": 1,
        "order": 4,
        "showTitle": false,
        "className": "",
        "visible": "true",
        "disabled": "false",
        "groupType": "default"
    },
    {
        "id": "page-1",
        "type": "ui-page",
        "name": "P_admin-VarG",
        "ui": "930a8a5e70986346",
        "path": "/admin_varG",
        "icon": "dashboard",
        "layout": "notebook",
        "theme": "1050ebb74976e9e1",
        "breakpoints": [
            {
                "name": "Default",
                "px": "0",
                "cols": "3"
            },
            {
                "name": "Tablet",
                "px": "576",
                "cols": "6"
            },
            {
                "name": "Small Desktop",
                "px": "768",
                "cols": "9"
            },
            {
                "name": "Desktop",
                "px": "1024",
                "cols": "12"
            }
        ],
        "order": 6,
        "className": "",
        "visible": true,
        "disabled": false
    },
    {
        "id": "1050ebb74976e9e1",
        "type": "ui-theme",
        "name": "Default Theme",
        "colors": {
            "surface": "#ffffff",
            "primary": "#0094ce",
            "bgPage": "#eeeeee",
            "groupBg": "#ffffff",
            "groupOutline": "#cccccc"
        },
        "sizes": {
            "density": "default",
            "pagePadding": "12px",
            "groupGap": "4px",
            "groupBorderRadius": "4px",
            "widgetGap": "10px"
        }
    }
]