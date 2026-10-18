# Code des nœuds function Node-RED

**Chemin** : `node-red/functions/`
**Objectif** : conserver, sous forme lisible et comparable, le code JavaScript des nœuds `function` de Node-RED.
**Version** : 1.0 — 18/10/2026 — Vincent Philippe / Claude

---

## Pourquoi ce dossier

Le code des nœuds `function` vit à l'intérieur de `flows.json`, où il est stocké sur une seule ligne avec les retours à la ligne échappés. Conséquence : une modification de dix caractères dans une fonction apparaît dans Git comme une ligne de 5 000 caractères entièrement réécrite. Impossible de relire ce qui a changé, impossible de revenir en arrière sur une seule fonction.

Déposer ici une copie lisible de chaque fonction importante rend les modifications relisibles.

## Qui fait autorité

⚠️ **`flows.json` fait foi. Ces fichiers en sont une copie.**

Node-RED ne lit pas ce dossier. Le robot tourne avec ce qui est dans `flows.json`, pas avec ce qui est ici. Un fichier de ce dossier qui ne serait pas reporté dans l'éditeur Node-RED ne sert à rien.

La règle est donc : **on modifie dans l'éditeur Node-RED, on déploie, on teste, puis on recopie ici.** Jamais l'inverse. Et on réexporte `flows.json` dans la foulée, pour que les deux restent cohérents.

C'est une convention nouvelle, introduite le 18/10/2026. Si elle se révèle plus coûteuse à tenir qu'utile, mieux vaut supprimer ce dossier que le laisser diverger silencieusement.

## Fichiers

| Fichier | Onglet | Rôle |
|---|---|---|
| `fn_init_traitements.js` | Gtion_Init | Charge `global.RZ_Traitements`, le registre des recettes de résolution des commandes vocales |
| `fn_stt_intent_router.js` | RZ_GestionFlux_V3 | Décode les capsules STT et applique les recettes du registre |

## Convention de nommage

Le nom du fichier reprend le nom du nœud dans Node-RED, sans numéro de version. La version se lit dans l'en-tête du fichier et dans l'historique Git — pas dans le nom, qui changerait à chaque correction et casserait le suivi.
