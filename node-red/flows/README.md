# Sauvegardes des flux Node-RED — RobotRZ

**Chemin** : `node-red/flows/`
**Objectif** : conserver l'historique des exports `flows.json` de Node-RED (SP), seule mémoire du travail réalisé côté pilote.
**Précaution majeure** : Node-RED ne versionne rien tout seul. Un export manuel est la **seule** sauvegarde existante. Entre mars et octobre 2026, sept mois de travail n'ont été sauvegardés nulle part.
**Articulation** : complète `docs/dev_RZ_HTML/` (documentation) et `node-red/config/globalSave.json` (Table B).
**Version** : 1.0 — 18/10/2026 — Vincent Philippe / Claude

---

## Fichiers présents

### `flows-RPI-2026-10-18.json` — **référence courante**

Export du SP de référence (Raspberry Pi 3, `192.168.1.40`). C'est l'état de production.

- 298 nœuds · 7 onglets · 9 sous-flows
- Canevas nettoyé le 18/10/2026 : 13 nœuds morts et le sous-flow vide
  `OldSub-Arrets` supprimés, après vérification qu'aucun n'avait d'entrée
  ni de sortie
- Export de fin de journée (15h47). L'état du matin, avant les travaux du
  18/10, reste consultable au commit `1a46604`.
- Onglets : `Gtion_Init`, `ui_Accueil_v4`, `Gtion_EtatGlobal_V2`, `RZ_GestionFlux_V3`, `ui_VarG`, `Diag_SE_Statut`, `B2_Variables_Complexes`
- Dashboard 2 (FlowFuse) — types de nœuds en `ui-xxx` avec tiret
- Contient la correction du câblage de `SF_GestionUrgence` déployée le 17/10/2026 : sortie urgence vers `EtatGlobal_Out`, sortie normale vers `SF_ReglesMinimales_V1` seule
- Contient aussi les travaux du 18/10/2026 : `fn_init_traitements`
  (registre des recettes de résolution vocale, onglet Gtion_Init) et
  `fn_stt_intent_router` v1.4, plus le banc d'essai à injects

### `flows-XIAOMI-2026-10-18.json` — **archive à ne pas supprimer**

Export du SP de secours (smartphone Xiaomi). Génération **antérieure** au RPI, mais il porte six onglets qui **n'existent nulle part ailleurs**.

- 455 nœuds · 16 onglets · 9 sous-flows
- Dashboard 1 — types de nœuds en `ui_xxx` avec tiret bas. Un portage vers le RPI impose une conversion.

Contenu unique, par ordre de valeur :

| Onglet | Nœuds | Pourquoi le garder |
|---|---|---|
| `ui_Rules` | 31 | **Moteur d'exécution de règles complet** + modèle de données (règle = activation + conditions + actions) + 3 notes de conception. Matière de `SF_ReglesDynamiques`. |
| `travail_joystickAccelerationEncodageVPIV` | 8 | Prototype de la page pilotage : nœud joystick, curseur d'accélération, conversion vers vitesse/rotation, retour odométrie, cadre vidéo. |
| `demarrage liaison SE-SP` | 5 | Poignée de main `SE_READY` puis envoi de configuration. Matière de `SF_Init_4`. |
| `travail_Calibrage Odometrie` | 6 | Paramètres mesurés réels : diamètre 0,100 m, voie 0,300 m, 256 points par tour. |
| `ui_VarRegles` | 48 | Interface de règles concurrente, écartée par décision. Import/export JSON. Référence. |
| `ui_Ini_Pilotage` | 19 | Page d'options de pilotage, génération de texte dynamique selon mode et périphérique. |

Également présentes, sans valeur d'usage : les générations anciennes `ui_Accueil_V2`, `ui_Accueil_v3`, `RZ_GestionFlux_V1`, `RZ_GestionFlux_V2`, `RZ_VPIV_Encoder_Dispatcher_V1`, `Gtion_EtatGlobal_V1`, et `oldvxui_Admin` (désactivé, 71 nœuds).

### `flows-sauvMarsV1.json` — historique

Export de mars 2026. Conservé comme point de repère. Ne reflète plus rien de l'état courant.

### La chronologie intermédiaire — avril à août 2026

Cinq exports retrouvés le 20/10/2026 dans le sas d'échange du dossier documentaire local, et versés ici pour combler le trou entre mars et octobre.

| Fichier | Nœuds | Onglets | Nom d'origine |
|---|---|---|---|
| `flows-RPI-2026-04-27.json` | 231 | 8 | `flows-Vserveur.json` |
| `flows-RPI-2026-05-20.json` | 261 | 9 | `flowsSP-total.json` |
| `flows-RPI-2026-06-21.json` | 270 | 9 | `flowsSP-total_21juin.json` |
| `flows-RPI-2026-07-05.json` | 294 | 9 | `flows_complet_5 juillet.json` |
| `flows-RPI-2026-08-08.json` | 306 | 7 | `flows_complet_8 aout.json` |

⚠️ **Pour les exports d'avril à juillet, la machine est déduite de la structure des onglets, pas certifiée.** Ils appartiennent clairement à la lignée du SP principal — 8 ou 9 onglets, là où le Xiaomi en compte 16 — mais la répartition RPI / Xiaomi n'était pas encore tranchée à cette période. Celui d'août correspond exactement aux 7 onglets du RPI actuel, sans ambiguïté.

**Ce que cette chronologie contient d'unique.** Une comparaison des onglets et sous-flows contre les deux références d'octobre n'a révélé qu'un seul élément introuvable ailleurs : l'onglet **`Test-MQTT`**, présent d'avril à juillet et disparu en août. Six nœuds — deux injects, un champ de saisie « TestMQTT local », un debug, une entrée MQTT sur `SE/statut`, une sortie sur `test`. Un banc d'essai de connexion, reconstructible en deux minutes, conservé ici par ces exports.

Les écarts de nombre de nœuds entre l'export d'août et le RPI d'octobre s'expliquent par le nettoyage du 18/10 : les six nœuds « manquants » de `Gtion_EtatGlobal_V2` sont précisément les nœuds morts supprimés ce jour-là.

---

## Procédure de sauvegarde

À faire en **fin** de chaque session Node-RED, sans exception :

1. Dans l'éditeur Node-RED, menu ☰ → **Export** → **All flows** → **Download**
2. Renommer en `flows-<MACHINE>-<AAAA-MM-JJ>.json` — machine = `RPI` ou `XIAOMI`
3. Déposer le fichier ici
4. `git add node-red/flows/ && git commit -m "Sauvegarde flux Node-RED <machine> <date>" && git push`

## Procédure de restauration

Menu ☰ → **Import** → coller le contenu du fichier → **Import** → **Deploy**.

⚠️ L'import **ajoute** aux flux existants, il ne les remplace pas. Pour une restauration complète, supprimer d'abord les onglets en place, ou importer dans une instance vierge.

---

## Pièges connus

- **Ne jamais se fier au dépôt pour connaître l'état de Node-RED** sans vérifier la date du fichier. Entre mars et octobre 2026, le dépôt affichait l'état de mars alors que la production avait sept mois d'avance.
- **Les deux machines ne sont pas des copies l'une de l'autre.** Le Xiaomi a divergé et conserve du travail que le RPI n'a jamais eu. Comparer avant de supposer.
- **Dashboard 1 et Dashboard 2 ne sont pas compatibles.** Les nœuds `ui_joystick`, `ui_slider`, `ui_text` du Xiaomi n'ont pas tous d'équivalent dans le Dashboard 2 du RPI.
- La taille du fichier varie selon le formatage de l'export : un écart de plusieurs dizaines de kilo-octets ne signifie pas qu'un contenu a été perdu. Comparer le nombre de nœuds, pas le poids.
