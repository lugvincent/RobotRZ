# PLAN DE RANGEMENT DE LA DOCUMENTATION LOCALE

**Fichier** : `docs/plan_rangement_doc_2026-10.md`
**Objet** : réorganiser `RobotRZ-horsIO` — le dossier documentaire local, hors dépôt — et le renommer `AA-Doc-horsGit`.
**Établi le** : 19/10/2026, à partir d'un inventaire de 1752 fichiers et 215,6 Mo.
**Auteur** : Vincent Philippe / Claude
**Version** : 1.0

---

## Avant de commencer — à lire une fois

**Rien n'est supprimé par ce plan.** Tout ce qui doit disparaître est d'abord déplacé dans un dossier de quarantaine, `_A_SUPPRIMER_apres_verif`. Vous ne le viderez qu'à la toute fin, une fois que tout fonctionne. Un déplacement sur le même disque est instantané, quelle que soit la taille.

**Passez les commandes une par une**, pas en bloc. Chaque phase se termine par une vérification. Si une commande renvoie une erreur, arrêtez-vous et signalez-la plutôt que de passer à la suivante.

**Ouvrez un terminal PowerShell** et placez-vous dans le dossier :

```
cd "$HOME\Documents\ProjetRZ\RobotRZ-horsIO"
```

Vérifiez que vous y êtes bien — l'invite doit se terminer par `RobotRZ-horsIO>`.

---

## Diagnostic qui a conduit à ce plan

| Constat | Chiffre |
|---|---|
| Fichiers au total | 1752 |
| Volume | 215,6 Mo |
| Dont un dépôt Git abandonné dans `SchémaHtml\.git` | 813 fichiers · 83,7 Mo |
| Dont trois tableurs géants (deux versions du même + un ancien) | 3 fichiers · 64 Mo |
| Dont un programme d'installation égaré | 6,8 Mo |
| **Documentation réelle, une fois ces trois retirés** | **≈ 936 fichiers · 61 Mo** |
| Doublons exacts | 131 contenus, 1,7 Mo — négligeable |

**Cause du désordre** : deux axes de classement concurrents. Un axe thématique (`A - Architecture`, `B - Node-RED`…) et un axe temporel (`AA-RZ-Vactuel-Doc`, `G - Old`). Comme `AA-RZ-Vactuel-Doc` contenait lui-même des sous-dossiers thématiques, chaque document avait deux emplacements également légitimes — d'où la paralysie, les fichiers posés à la racine et le dossier `non classé`. Un troisième classement thématique complet existait même dans `FUTUR-DOC\DOC\01_ARCHITECTURE`, `02_ARDUINO`…

**Règle retenue** : **le thème décide du dossier, l'âge décide de la place à l'intérieur du dossier.** Une seule question, une seule réponse.

---

## Structure cible

```
AA-Doc-horsGit\
│
├── A - Architecture, Règles, Tables\   └── old\
├── B - SP - Node-RED\                  └── old\
├── C - SE - Termux - Tasker\           └── old\
├── D - Arduino\                        └── old\
│
├── E - Échange ponctuel — à vider      exception assumée : sas de transit
├── F - Dev-en-cours                    exception assumée : le travail du moment
│
├── H - Outils
├── I - Archive                         sujets clos
│
├── J - Électronique et câblage         schémas, brochages — traversent les versions
├── K - Documents utilisateurs          notices, guides, présentations
├── L - RZ-v1 sur RPi4                  génération antérieure, encore en service
├── M - Bilans de séance                journal de travail, par date
│
├── Z - Secret                          clés, identifiants, codes
│
└── LISEZ-MOI.md                        le vade-mecum
```

`G - Old` disparaît : chaque lettre a son propre `old\`, au plus près de ce qu'il complète.
`CODE` reste à la racine, c'est du code, pas de la documentation.

---

# PHASE 0 — Sauvegarde

**Ne sautez pas cette phase.** Elle prend deux minutes et rend tout le reste réversible.

### 0.1 — Copier l'ensemble du dossier

```
Copy-Item "$HOME\Documents\ProjetRZ\RobotRZ-horsIO" "$HOME\Documents\SAUVEGARDE-horsIO-2026-10-19" -Recurse
```

Comptez une à deux minutes pour 215 Mo.

### 0.2 — Vérifier que la copie est complète

```
(Get-ChildItem "$HOME\Documents\SAUVEGARDE-horsIO-2026-10-19" -Recurse -File -Force).Count
```

Le nombre affiché doit être proche de **1752**. S'il est très inférieur, la copie a échoué — recommencez avant d'aller plus loin.

### 0.3 — Créer la quarantaine

```
New-Item -ItemType Directory "_A_SUPPRIMER_apres_verif" -Force
```

---

# PHASE 1 — Le dépôt caché

`FUTUR-DOC\AA-RZ-Vactuel-Doc\SchémaHtml\.git` est un clone du dépôt RobotRZ, branche `master`, huit commits. **Aucun de ces huit commits n'existe dans le dépôt actuel** — l'historique du projet a été reparti de zéro en passant de `master` à `main`. Les 670 fichiers que ce dépôt croit supprimés sont tous présents ailleurs sur votre disque : vous ne perdez aucun document, seulement la trace des modifications entre ces huit commits.

On en extrait donc un journal lisible, puis on le met en quarantaine.

### 1.1 — Mémoriser le chemin

```
$sch = (Get-ChildItem "FUTUR-DOC\AA-RZ-Vactuel-Doc" -Directory -Filter "Sch*Html").FullName
```

```
$sch
```

La seconde ligne doit afficher le chemin complet se terminant par `SchémaHtml`.

### 1.2 — Extraire le journal lisible

```
git -C $sch log --stat --date=short --pretty=format:"%n===== %h  %ad  %an%n%s%n%b" --all > "$HOME\Documents\journal_ancien_depot.txt" 2>&1
```

### 1.3 — Vérifier le journal

```
Get-Item "$HOME\Documents\journal_ancien_depot.txt" | Select-Object Length
```

```
Get-Content "$HOME\Documents\journal_ancien_depot.txt" -TotalCount 25
```

Vous devez voir les huit commits, du plus récent (`8840c00 Merge branch 'master'`) au plus ancien (`a014d44 Initialisation de la structure du projet`), chacun suivi de la liste des fichiers touchés. Si le fichier est vide ou illisible, **arrêtez-vous ici** et signalez-le.

### 1.4 — Ranger le journal dans l'archive

Le dossier `I - Archive` existe déjà. On y dépose le journal :

```
Move-Item "$HOME\Documents\journal_ancien_depot.txt" "FUTUR-DOC\I - Archive\journal_ancien_depot_master_2026-10-19.txt"
```

### 1.5 — Mettre le dépôt en quarantaine

```
Move-Item "$sch\.git" "_A_SUPPRIMER_apres_verif\git_SchemaHtml" -Force
```

### 1.6 — Vérifier le gain

```
"{0:N1} Mo" -f ((Get-ChildItem "_A_SUPPRIMER_apres_verif" -Recurse -File -Force | Measure-Object Length -Sum).Sum / 1MB)
```

Vous devez lire environ **83 Mo**.

---

# PHASE 2 — Les intrus volumineux

### 2.1 — Le programme d'installation égaré

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\non classé\Claude Setup.exe" "_A_SUPPRIMER_apres_verif\" -Force
```

### 2.2 — Les trois tableurs géants

Ils pèsent 21,5 Mo chacun alors que leurs équivalents `.ods` font 1,7 Mo. Ce rapport de douze trahit des images incrustées ou une feuille corrompue. Ils datent d'août et novembre 2025, et la Table A les a remplacés.

**Avant de les déplacer, ouvrez-en un** pour vérifier qu'il ne contient rien d'irremplaçable :

```
Invoke-Item "FUTUR-DOC\DOC\01_ARCHITECTURE\Listes-orga-code-var-pin-RZ-New.xlsx"
```

Si son contenu est bien repris ailleurs — Table A, ou la version `.ods` du même nom — fermez-le et poursuivez. Sinon, arrêtez-vous et dites-le-moi.

```
Move-Item "FUTUR-DOC\DOC\01_ARCHITECTURE\Listes-orga-code-var-pin-RZ-New.xlsx" "_A_SUPPRIMER_apres_verif\" -Force
```

```
Move-Item "FUTUR-DOC\DOC\02_ARDUINO\Listes-orga-code-var-pin-RZ-New.xlsx" "_A_SUPPRIMER_apres_verif\Listes-orga-code-var-pin-RZ-New_copie02.xlsx" -Force
```

```
Move-Item "FUTUR-DOC\DOC\01_ARCHITECTURE\Listes-orgaArduino-code-pin-cable.xlsx" "_A_SUPPRIMER_apres_verif\" -Force
```

Les versions `.ods` restent en place : ce sont elles qu'on gardera.

### 2.3 — Contrôle

```
"{0:N1} Mo hors quarantaine" -f ((Get-ChildItem . -Recurse -File -Force | Where-Object { $_.FullName -notlike "*_A_SUPPRIMER*" } | Measure-Object Length -Sum).Sum / 1MB)
```

```
"{0:N1} Mo en quarantaine" -f ((Get-ChildItem "_A_SUPPRIMER_apres_verif" -Recurse -File -Force | Measure-Object Length -Sum).Sum / 1MB)
```

La quarantaine doit approcher **154 Mo**.

---

# PHASE 3 — Le fichier au nom impossible

Un seul fichier bloque la compression du dossier. Son nom est en réalité un titre de document markdown, avec une émoticône et un tiret long, tronqué et sans extension :

```
# 📡 Module de Communication — RZmegaV4 (Arduino Me
```

### 3.1 — Le retrouver et le renommer

```
Get-ChildItem "FUTUR-DOC\DOC\01_ARCHITECTURE" -File | Where-Object { $_.Name -match '[^ -~ -ÿ]' } | Rename-Item -NewName "module_communication_RZmegaV4.md"
```

### 3.2 — Vérifier

```
Get-ChildItem "FUTUR-DOC\DOC\01_ARCHITECTURE" -File | Where-Object { $_.Name -match '[^ -~ -ÿ]' }
```

Cette commande ne doit **rien** afficher.

---

# PHASE 4 — Créer la nouvelle arborescence

Les dossiers sont créés à la racine de `RobotRZ-horsIO`, à côté de `FUTUR-DOC` et `CODE`. On y déplacera le contenu ensuite, puis `FUTUR-DOC` disparaîtra.

### 4.1 — Les lettres thématiques et leurs `old`

```
"A - Architecture, Regles, Tables","B - SP - Node-RED","C - SE - Termux - Tasker","D - Arduino" | ForEach-Object { New-Item -ItemType Directory $_ -Force ; New-Item -ItemType Directory "$_\old" -Force }
```

### 4.2 — Les autres

```
"E - Echange ponctuel - a vider","F - Dev-en-cours","H - Outils","I - Archive","J - Electronique et cablage","K - Documents utilisateurs","L - RZ-v1 sur RPi4","M - Bilans de seance","Z - Secret" | ForEach-Object { New-Item -ItemType Directory $_ -Force }
```

*Note : les noms sont écrits sans accents pour éviter tout souci d'encodage en ligne de commande. Vous pourrez les accentuer ensuite dans l'explorateur si vous préférez — mais sachez que ce sont ces accents qui ont compliqué deux commandes aujourd'hui.*

### 4.3 — Vérifier

```
Get-ChildItem . -Directory | Select-Object Name
```

Vous devez voir les treize nouveaux dossiers, plus `CODE`, `FUTUR-DOC` et `_A_SUPPRIMER_apres_verif`.

---

# PHASE 5 — Déplacer le contenu

Un déplacement par étape, avec le raisonnement. Si un dossier source n'existe pas, la commande le signalera sans rien casser — passez à la suivante.

### 5.1 — Les lettres existantes remontent d'un niveau

```
Move-Item "FUTUR-DOC\A - Architecture - Régles -Tables - ModulesCompiles\*" "A - Architecture, Regles, Tables\" -Force
```

```
Move-Item "FUTUR-DOC\B - SP - Node-RED\*" "B - SP - Node-RED\" -Force
```

```
Move-Item "FUTUR-DOC\C - SE - Termux - Tasker\*" "C - SE - Termux - Tasker\" -Force
```

```
Move-Item "FUTUR-DOC\D - A - Arduino\*" "D - Arduino\" -Force
```

```
Move-Item "FUTUR-DOC\E - Pour-Echange_Ponctuel_A vider\*" "E - Echange ponctuel - a vider\" -Force
```

```
Move-Item "FUTUR-DOC\F - Dev-en-cours\*" "F - Dev-en-cours\" -Force
```

```
Move-Item "FUTUR-DOC\H - Outils\*" "H - Outils\" -Force
```

```
Move-Item "FUTUR-DOC\I - Archive\*" "I - Archive\" -Force
```

### 5.2 — `G - Old` se répartit

Son contenu est constitué d'anciennes versions. Sans ouvrir chaque fichier, on ne peut pas deviner de quel thème il relève. On le pose donc dans l'`old` de la lettre la plus probable, `A`, et vous répartirez à la lecture :

```
Move-Item "FUTUR-DOC\G - Old\*" "A - Architecture, Regles, Tables\old\" -Force
```

### 5.3 — `AA-RZ-Vactuel-Doc` se ventile dans les lettres

C'est le cœur de la correction : ce dossier dupliquait l'axe thématique.

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\Tables\*" "A - Architecture, Regles, Tables\" -Force
```

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\SP-Node-red\*" "B - SP - Node-RED\" -Force
```

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\Tasker" "C - SE - Termux - Tasker\Tasker" -Force
```

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\SE\*" "C - SE - Termux - Tasker\" -Force
```

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\Arduino\*" "D - Arduino\" -Force
```

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\Outil - Installation\*" "H - Outils\" -Force
```

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\provisoire\*" "F - Dev-en-cours\" -Force
```

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\Synthèse_Appli_RZ\*" "A - Architecture, Regles, Tables\" -Force
```

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\Serveurs RPI\*" "B - SP - Node-RED\" -Force
```

### 5.4 — Les pages HTML : bilans d'un côté, versions anciennes de l'autre

**Dix-huit pages de `SchémaHtml` sont strictement identiques** à des pages déjà présentes dans le dépôt Git, donc déjà sauvegardées. Elles partent en quarantaine :

```
"cadrage_plan_regles_dynamiques.html","guide_tasker_bonnes_pratiques.html","schema_mecanisme_resolution_intents_V1.html","schema_pipeline_vocal_V1.html","vademecum_ack_timeout_V1.html","vademecum_tables_V1.html" | ForEach-Object { Move-Item "$sch\$_" "_A_SUPPRIMER_apres_verif\" -Force -ErrorAction SilentlyContinue }
```

```
Move-Item "$sch\robotRZ_Doc_Html_Valide" "_A_SUPPRIMER_apres_verif\robotRZ_Doc_Html_Valide" -Force
```

*Ce sous-dossier contenait douze pages, toutes déjà dans le dépôt.*

**Les bilans de séance** — une trentaine, de mai à août 2026 — rejoignent leur lettre. Ils restent délibérément locaux, hors du site :

```
Get-ChildItem "$sch" -File -Filter "*.html" | Where-Object { $_.Name -match 'synthese|bilan|session|implementation|correction' } | Move-Item -Destination "M - Bilans de seance\" -Force
```

**Ce qui reste** sont des versions anciennes de pages du site — `index.html` de juillet, `sp_nodeRED_V1`, `roadmap_nodered_V1` en deux générations, `tables_donnees_V1`, `circulation_donnees_robotrz` :

```
Get-ChildItem "$sch" -File | Move-Item -Destination "B - SP - Node-RED\old\" -Force
```

### 5.5 — `non classé` : à trier à la main

Douze fichiers, 6,9 Mo. On les pose dans le sas de transit plutôt que de deviner :

```
Move-Item "FUTUR-DOC\AA-RZ-Vactuel-Doc\non classé\*" "E - Echange ponctuel - a vider\" -Force
```

### 5.6 — `FUTUR-DOC\DOC` : le troisième classement

```
Move-Item "FUTUR-DOC\DOC\00_PROJET\*" "A - Architecture, Regles, Tables\old\" -Force
```

```
Move-Item "FUTUR-DOC\DOC\01_ARCHITECTURE\*" "A - Architecture, Regles, Tables\" -Force
```

```
Move-Item "FUTUR-DOC\DOC\02_ARDUINO\*" "D - Arduino\" -Force
```

```
Move-Item "FUTUR-DOC\DOC\03_NODE-RED\*" "B - SP - Node-RED\" -Force
```

```
Move-Item "FUTUR-DOC\DOC\04_SE-TASKER\*" "C - SE - Termux - Tasker\" -Force
```

```
Move-Item "FUTUR-DOC\DOC\05_OPERATION\*" "K - Documents utilisateurs\" -Force
```

```
Move-Item "FUTUR-DOC\DOC\06_Application-Tiers\*" "K - Documents utilisateurs\" -Force
```

```
Move-Item "FUTUR-DOC\DOC\99_ARCHIVES\*" "I - Archive\" -Force
```

*`05_OPERATION` et `06_Application-Tiers` vont vers les documents utilisateurs : ce sont des documents d'usage, pas de conception. Vérifiez à la lecture, c'est le seul choix de ce plan que je fais sans certitude.*

### 5.7 — Les fichiers en vrac à la racine de FUTUR-DOC

```
Move-Item "FUTUR-DOC\protocoleVPIV.odt","FUTUR-DOC\table_A_SE_v3.xlsx","FUTUR-DOC\tableA-Legende.csv","FUTUR-DOC\urgence and co.odt","FUTUR-DOC\ListeTT_Module.odt" "A - Architecture, Regles, Tables\" -Force -ErrorAction SilentlyContinue
```

```
Move-Item "FUTUR-DOC\ui_VarG.js","FUTUR-DOC\.`$schema_flows_SP_RZ.drawio.bkp" "B - SP - Node-RED\old\" -Force -ErrorAction SilentlyContinue
```

```
Move-Item "FUTUR-DOC\synthese_arduino_nodered.md" "D - Arduino\" -Force -ErrorAction SilentlyContinue
```

```
Move-Item "FUTUR-DOC\doc_SE_ref_nodeREDold.docx","FUTUR-DOC\doc_prep_SE_nodeRED.docx" "C - SE - Termux - Tasker\old\" -Force -ErrorAction SilentlyContinue
```

```
Move-Item "FUTUR-DOC\archive conversationréseau\*" "I - Archive\" -Force -ErrorAction SilentlyContinue
```

```
Move-Item "FUTUR-DOC\Nouveau Texte OpenDocument.odt" "_A_SUPPRIMER_apres_verif\" -Force -ErrorAction SilentlyContinue
```

### 5.8 — Ce qui reste dans FUTUR-DOC

```
Get-ChildItem "FUTUR-DOC" -Recurse -File -Force | Select-Object FullName
```

Si cette commande affiche encore des fichiers, **ne supprimez pas `FUTUR-DOC`** : envoyez-moi la liste, on décidera de leur destination. Si elle n'affiche rien :

```
Remove-Item "FUTUR-DOC" -Recurse -Force
```

---

# PHASE 6 — Le dossier secret

### 6.1 — Y déplacer ce qui doit y être

```
Get-ChildItem . -Recurse -File -Force | Where-Object { $_.Name -match 'clef|secret|password|token|id-code' -and $_.FullName -notlike "*_A_SUPPRIMER*" } | Select-Object FullName
```

Examinez la liste, puis déplacez à la main ce qui relève du secret vers `Z - Secret`.

### 6.2 — Deux précautions

Le `.gitignore` sera mis à jour en phase 7 pour exclure `Z - Secret` par son nom **et** par motif de fichier.

Et une recommandation qui dépasse ce plan : vos identifiants sont aujourd'hui dans un fichier texte **en clair**. L'exclusion Git les protège d'une publication accidentelle, rien d'autre. Un gestionnaire de mots de passe local et chiffré — KeePassXC, gratuit, un seul fichier — offrirait une vraie protection pour le même confort.

---

# PHASE 7 — Renommage et dépôt

### 7.1 — Sortir du dossier

Windows refuse de renommer un dossier qui est le répertoire courant d'un processus. C'est ce qui a bloqué votre tentative.

```
cd "$HOME\Documents\ProjetRZ"
```

### 7.2 — Fermer Visual Studio Code

S'il a le dossier ouvert, il tient lui aussi un verrou. Fermez l'application entièrement, puis rouvrez un terminal PowerShell depuis le menu Démarrer et replacez-vous :

```
cd "$HOME\Documents\ProjetRZ"
```

### 7.3 — Renommer

```
Rename-Item "RobotRZ-horsIO" "AA-Doc-horsGit"
```

### 7.4 — ⚠️ Mettre à jour le `.gitignore` immédiatement

**C'est l'étape à ne pas oublier.** Le `.gitignore` exclut le dossier **par son nom**. Après renommage, l'exclusion ne s'applique plus, et le dossier — clés comprises — devient candidat au prochain commit, dans un dépôt **public**.

Signalez-moi que le renommage est fait : je mets à jour le `.gitignore` et je pousse. Ou, si vous préférez le faire vous-même, remplacez la ligne `RobotRZ-horsIO/` par :

```
AA-Doc-horsGit/
Z - Secret/
*clef*
*secret*
*password*
Id-code-*
```

### 7.5 — Vérifier que rien de sensible n'est suivi

```
cd "$HOME\Documents\ProjetRZ"
```

```
git status --short
```

Aucune ligne ne doit mentionner `AA-Doc-horsGit`. Si une seule apparaît, **ne commitez rien** et signalez-le.

---

# PHASE 8 — Contrôle final

### 8.1 — Compter

```
cd "$HOME\Documents\ProjetRZ\AA-Doc-horsGit"
```

```
(Get-ChildItem . -Recurse -File -Force | Where-Object { $_.FullName -notlike "*_A_SUPPRIMER*" }).Count
```

```
Get-ChildItem . -Directory | ForEach-Object { $n=(Get-ChildItem $_.FullName -Recurse -File -Force).Count ; $m=[math]::Round((Get-ChildItem $_.FullName -Recurse -File -Force | Measure-Object Length -Sum).Sum/1MB,1) ; "{0,5} fich. {1,7} Mo   {2}" -f $n,$m,$_.Name }
```

Hors quarantaine, vous devez approcher **936 fichiers et 61 Mo**.

### 8.2 — Tester la compression, qui échouait

```
Compress-Archive -Path "A - Architecture, Regles, Tables" -DestinationPath "$HOME\Documents\test_zip.zip" -Force
```

Si cette commande aboutit, le problème de nom est réglé. Supprimez ensuite le fichier d'essai :

```
Remove-Item "$HOME\Documents\test_zip.zip"
```

### 8.3 — Vider la quarantaine — **pas avant quelques jours d'usage**

Travaillez une semaine avec la nouvelle organisation. Si rien ne vous manque :

```
Remove-Item "_A_SUPPRIMER_apres_verif" -Recurse -Force
```

Et quand vous serez tout à fait serein, la sauvegarde de la phase 0 :

```
Remove-Item "$HOME\Documents\SAUVEGARDE-horsIO-2026-10-19" -Recurse -Force
```

---

# Ce qu'il restera à faire ensuite

**Le vade-mecum `LISEZ-MOI.md`**, à la racine du dossier, répondant à la seule question qui compte au retour d'une pause : *où est quoi, et qu'est-ce qui fait foi ?* Il couvrira les quatre emplacements — ce dossier local avec ses lettres, les pages HTML du site dans le dépôt, le dossier technique, et la synthèse d'état avec sa double copie. Une page jumelle sera ajoutée au site HTML.

**Trois lettres neuves restent vides** et se rempliront au fil de vos relectures : `J - Electronique et cablage` pour les schémas Arduino–capteurs, `K - Documents utilisateurs`, et `L - RZ-v1 sur RPi4` pour la génération antérieure encore en service. Leur contenu est aujourd'hui dispersé ; il n'était pas identifiable depuis un simple inventaire de noms de fichiers.

**Le sas `E`** contient maintenant les douze fichiers de l'ancien `non classé`. À vider à la lecture.
