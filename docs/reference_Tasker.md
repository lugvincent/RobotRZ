# Référence Tasker — la couche Android de SE

> **Fichier** : `docs/reference_Tasker.md`
> **Objet** : comprendre et retoucher la partie Tasker sans la redécouvrir.
> Comment Termux lui parle, comment elle répond, ce que fait chaque famille de
> tâches, et quelles sont ses limites documentaires.
> **Comment il a été écrit** : **depuis l'export Tasker du dépôt** —
> 4 profils, 6 scènes, 45 tâches, 3 scripts shell. Tasker n'ayant pas d'entêtes
> comme les scripts Bash, la structure a été extraite des XML et recoupée avec
> les scripts SE qui dialoguent avec elle.
> **Articulation** : `docs/reference_SE_termux_tasker.md` est son pendant côté
> Linux — les deux décrivent le même téléphone.
> **⚠ Couverture partielle** : voir §11. Les comptes rendus de séances d'essai
> (`AA-Doc-horsGit\M - Bilans de seance\`) n'ont pas été dépouillés ; ils
> contiennent des résultats d'essai que ce document ignore.
> **Version** : 1.0 — 22/10/2026 — Vincent Philippe / Claude

---

## 1. Ce que Tasker apporte, et pourquoi elle est indispensable

Termux est un Linux dans une boîte. Il ne sait pas ouvrir Zoom, afficher une
image par-dessus l'écran d'accueil, ni demander à l'appareil photo de diffuser.
**Tasker le sait.**

La frontière est donc fonctionnelle, pas arbitraire :

| Termux | Tasker |
|---|---|
| Réseau, MQTT, capteurs, logique, scripts | Applications Android, affichage, reconnaissance vocale |

---

## 2. 🔴 C'est Tasker qui démarre le robot

Point d'architecture que rien d'autre ne dit clairement. La tâche
**`RZ_Init_Tasker`** est le véritable point d'entrée de SE :

```
①  Lancer rz_start.sh via Termux:Tasker
    chemin : /data/data/com.termux/files/home/robotRZ-smartSE/termux
    puis attendre 4 s que les démons montent
②  Lancer IP Webcam Pro en veille, attendre 2 s
③  Afficher la scène RZ_Expression, emoji neutre 😐
④  Initialiser les variables :
      %RZiAConvOnOff = Off        %RZautoVoice   = inactif
      %RZsttContext  = Mixte      %RZecranEtat   = A
      %RZtorchEtat   = Off        %RZappliActive = (vide)
```

> Conséquence pratique : **un SE qui ne démarre pas se diagnostique d'abord
> dans Tasker, pas dans Termux.** Si `RZ_Init_Tasker` ne s'est pas exécutée,
> aucun démon Termux ne tourne.

---

## 3. Comment Termux parle à Tasker — deux fichiers surveillés

Il n'y a **pas d'appel direct**. Termux écrit dans un fichier, un profil Tasker
surveille ce fichier et réagit à sa modification.

| Profil | Surveille | Déclenche | Variable |
|---|---|---|---|
| `Fichier_ModifieParSE` | `Tasker/RobotRZ/trigger.txt` | `RZ_Dispatch` | `%RZ_JSON` |
| `Menu_Commande` | `Tasker/RobotRZ/menu_cmd.json` | `RZ_MenuCmd` | `%RZ_MENU_JSON` |

Côté Termux, c'est `rz_tasker_bridge.sh` qui écrit dans `trigger.txt`.

### `RZ_Dispatch` — le routeur générique

Trois actions, et tout le mécanisme tient dedans :

```
1. Lire trigger.txt            →  %RZ_JSON
2. Parser le JSON              →  %RZtask  +  %RZparam
3. Appeler la tâche nommée     →  %RZtask (%RZparam)
```

**Termux n'a donc pas besoin de connaître Tasker.** Il écrit
`{"task":"RZ_CamStart","param":"rear"}` et c'est fini. Ajouter une tâche
Tasker ne demande aucune modification côté Linux.

---

## 4. Comment Tasker répond — `vpiv_out.txt`

Le retour emprunte un chemin différent, lui aussi par fichier :

```
Tasker  →  écrit une trame VPIV dans vpiv_out.txt
           ↓   inotifywait
       rz_tasker_out.sh  (Termux)
           ↓   mosquitto_pub
       SE/statut  →  Node-RED
```

Exemples réellement produits :

```
$V:CfgS:modeRZ:*:1#     commande de mode, depuis le menu Tasker
$I:Appli:Zoom:*:Off#    accusé de fermeture d'application
```

> Noter l'asymétrie : **Tasker émet des `$V`**, donc des consignes. Le menu du
> téléphone est une véritable télécommande, pas un simple afficheur.

---

## 5. L'arbitrage vocal — `RZ_STT_Context`

La tâche la plus subtile du projet, et elle n'était documentée nulle part.

SP calcule un contexte vocal et l'envoie à SE ; Tasker en déduit lequel des
deux moteurs vocaux doit tourner — **AutoVoice** pour les commandes,
**Gemini** pour la conversation.

| Contexte reçu | AutoVoice | Gemini |
|---|---|---|
| **`Cmde`** | démarrer | **fermer** — SP prioritaire |
| **`Conv`** | arrêter | ouvert seul |
| **`Mixte`** | démarrer | ouvert, avec AutoVoice |
| **`Inactif`** | arrêter | selon `%RZiAConvOnOff` : `On` → ouvert seul · `Off` → fermé |

Deux choses méritent d'être relevées.

**En mode `Cmde`, Gemini est fermé d'autorité.** C'est le mode déplacement :
on ne veut pas qu'une conversation retarde une commande.

**En `Inactif`, l'utilisateur garde la main.** La variable `%RZiAConvOnOff`
traduit une intention explicite : si vous avez demandé l'IA, elle reste
ouverte même quand la reconnaissance de commandes est coupée.

Le calcul du contexte lui-même est côté Termux, dans `rz_state-manager.sh` —
voir `reference_SE_termux_tasker.md` §7.

---

## 6. Les tâches, par famille

**Affichage et navigation** — `RZ_OuvreMenu`, `RZ_FermeMenu`, `RZ_OuvreEtat`,
`RZ_FermeEtat`, `RZ_OuvreVoice`, `RZ_FermeVoice`, `RZ_OuvreVoiceDetail`,
`RZ_FermeVoiceDetail`, `RZ_EtatMajScene`, `RZ_EtatRefresh`, `RZ_Aide`

**Expressions** — `RZ_Expression`, `RZ_ExprNeutre`. Le robot affiche un emoji
sur son écran ; c'est son visage.

**Applications** — `RZ_AppStart`, `RZ_AppStop`, et une tâche par application :
`RZ_Zoom`, `RZ_Baby`, `RZ_Maps`, `RZ_NavGPS`

**Caméra et torche** — `RZ_CamStart`, `RZ_CamStop`, `RZ_TorchOn`,
`RZ_TorchOff`

**Chaîne vocale** — `RZ_AVStart`, `RZ_AVStop`, `RZ_GeminiStart`,
`RZ_GeminiStop`, `RZ_IA_Conv`, `RZ_CaptureVoice`, `Rz_Traite_Ecoute`,
`RZ_VoiceError`, `RZ_STT_Context`

**Noyau** — `RZ_Init_Tasker`, `RZ_Dispatch`, `RZ_MenuCmd`,
`RZ_MenuSendMode`, `RZ_MenuSendAppli`

S'y ajoutent **quatorze tâches `taskNN_internal`** : des sous-tâches sans nom,
appelées par les précédentes. Tasker les numérote, elles n'ont pas d'identité
propre — ne pas chercher à leur en donner une.

---

## 7. Les six scènes

Les scènes sont les fenêtres que Tasker superpose à l'écran.

| Scène | Rôle |
|---|---|
| `RZ_Expression` | **Le visage du robot** — l'emoji affiché en permanence |
| `RZ_Menu` | La télécommande : modes et applications |
| `RZ_Etat` | Tableau de bord — mode, pilotage, réseau, adresses |
| `RZ_VoiceDetail` | Détail de la reconnaissance vocale |
| `RZ_VoiceHelp` | Aide des commandes vocales |
| `RZ_FermeOverlay` | Fermeture des superpositions |

`%RZecranEtat` et `%RZecranPrev` mémorisent l'écran courant et le précédent —
c'est ainsi que le retour en arrière fonctionne.

---

## 8. Les applications pilotées

| Application | Paquet Android | Usage |
|---|---|---|
| IP Webcam Pro | `com.pas.webcam.pro` | Flux vidéo, et la torche quand elle tourne |
| Zoom | `us.zoom.videomeetings` | Visioconférence |
| BabyCam | `com.arjonasoftware.babycam` | Surveillance |
| Google Maps | `com.google.android.apps.maps` | Navigation GPS |
| AutoVoice | `com.joaomgcd.autovoice` | Reconnaissance vocale |
| Termux:API | `com.termux.api` | Passerelle Android depuis Bash |
| Termux:Tasker | `com.termux.tasker` | **Lance les scripts Termux depuis Tasker** |

> `com.termux.tasker` est la pièce qui rend tout possible : sans ce greffon,
> Tasker ne peut pas lancer `rz_start.sh`, et le robot ne démarre pas.

---

## 9. Les variables globales

Trente et une, et leur préfixe dit la famille :

| Préfixe | |
|---|---|
| `%RZetat*` | `Mode`, `Ptge`, `Reseau`, `CtxReseau`, `IpSE`, `IpSP`, `Ts` — ce qu'affiche le tableau de bord |
| `%RZappli*` | `Active`, `Nom`, `Task`, `Conv` — gestion des applications |
| `%RZecran*` | `Etat`, `Prev` — navigation entre scènes |
| `%RZtorch*` | `Etat`, `Mode` |
| `%RZemoji*` | `emoji`, `emoji_defaut` |
| `%RZ_JSON`, `%RZ_MENU_JSON`, `%RZtask`, `%RZparam` | le routage |
| `%RZsttContext`, `%RZautoVoice`, `%RZiAConvOnOff`, `%RZgeminiArg`, `%RZvoiceArg` | la chaîne vocale |
| `%RZvpivOut` | la trame sortante |

---

## 10. Les trois scripts shell de `tasker/`

| Script | Fait |
|---|---|
| `rz_copy_global.sh` | Copie `global.json` vers `~/storage/shared/Tasker/RobotRZ/` |
| `rz_torch.sh` | Appelle `rz_torch_manager.sh active "$1"` |
| `rz_autovoice_bridge.sh` | L'arbitrage vocal côté Linux — 9 Ko |

Les deux premiers sont des passe-plats d'une ligne : Tasker ne sait pas écrire
du Bash complexe, alors il appelle.

---

## 11. 🔴 Pièges, doublons et limites

**1. Six fichiers de `tasker/archive/` sont vides** — depuis toujours.
`rz_apps_manager.xml`, `rz_tts_task.xml`, `rz_zoom_profile.xml`, et les trois
expressions — `angry.xml`, `neutral.xml`, `smile.xml`. Zéro octet.

> ⚠️ **Correction du 22/10/2026** : j'avais écrit que le Project Knowledge en
> détenait le contenu. **C'est faux** — ils y sont vides aussi, créés le
> 11 mars 2026 dans le même lot que ceux de `termux/`. Quatorze emplacements
> réservés, jamais remplis, nulle part. Rien n'a été perdu.

Conséquence pour les expressions : **le visage du robot n'est pas versionné.**
Les emojis affichés par la scène `RZ_Expression` ne sont définis que dans
l'export Tasker et sur le téléphone.

**2. `rz_autovoice_bridge.sh` existe en deux exemplaires identiques** —
`tasker/` et `termux/scripts/sensors/stt/`, même empreinte MD5. Lequel fait
foi ? Tant que les deux sont identiques, la question ne se pose pas ; le jour
où l'un est corrigé, elle se posera brutalement.

**3. `rz_copy_global.sh` aussi**, dans `tasker/` et `tasker/scripts/`. Même
remarque.

**4. `summary.txt` décrit une sauvegarde périmée** : 43 tâches et 2 profils,
alors que l'export actuel en compte 45 et 4. Toute la chaîne vocale —
`RZ_STT_Context`, `RZ_AVStart`, `RZ_GeminiStart`, `Rz_Traite_Ecoute` — lui est
postérieure. Ne pas s'y fier pour un inventaire.

**5. `rz_facial_scene.sh` et `rz_launch_app.sh` n'existent pas dans le
dépôt.** Ils figurent dans le Project Knowledge. À verser, ou à déclarer
abandonnés.

**6. Les quatorze `taskNN_internal` n'ont pas de nom.** C'est Tasker qui les
numérote. Les renommer dans l'export casserait les appels.

### Ce que ce document ne couvre pas

Les XML Tasker décrivent **ce que les tâches font**, pas **ce qui a été
essayé** ni **ce qui fonctionne**. Vous avez mené des séances d'essai dont les
comptes rendus sont dans `AA-Doc-horsGit\M - Bilans de seance\` —
`synthese_tasker_robotrz-suite_9mai.html`,
`implementation_tasker_STT_11juin.html`, `synthese_session_11juin.html` et
d'autres.

**Ils apporteraient ce qui manque ici** : l'état de validation de chaque
tâche, les difficultés rencontrées, et les contournements adoptés. À dépouiller
pour une v2.

---

## 12. Pour retoucher Tasker

1. **Exporter avant de modifier.** Tasker ne versionne rien, exactement comme
   Node-RED. L'export du dépôt est votre seule sauvegarde.
2. **Pour appeler une nouvelle tâche depuis Termux**, rien à modifier côté
   Tasker : écrire `{"task":"MaTache","param":"…"}` dans `trigger.txt` suffit,
   `RZ_Dispatch` fait le reste.
3. **Pour renvoyer quelque chose à SP**, écrire une trame VPIV dans
   `vpiv_out.txt`. Respecter l'ordre `PROP:INST`.
4. **Ne pas renommer les `taskNN_internal`.**
5. **Vérifier les verrous** si la tâche touche la caméra ou le micro — Zoom et
   IP Webcam se disputent la caméra, Zoom et la reconnaissance vocale se
   disputent le micro. Voir `reference_SE_termux_tasker.md` §11.
6. **Réexporter et commiter** en fin de séance.

---

*V1 — 22/10/2026. Troisième et dernière référence de sous-système écrite depuis
les sources, après l'Arduino et SE. Reste SP — Node-RED — volontairement
différée : c'est le chantier en cours, et documenter un chantier en cours
revient à écrire sur du sable.*
