# Genèse et évolution du projet RobotRZ

> **Fichier** : `docs/genese_et_evolution_RZ.md`
> **Objet** : pourquoi ce robot existe, et comment il est passé de la V0 à la V6.
> C'est le document à lire en premier par qui découvre le projet, ou par qui le
> reprend après une interruption longue.
> **Articulation** : répond au *pourquoi*. Le *quoi* est dans
> `docs/tables_RZ_descripteur.md` (les tables et le protocole), le *comment*
> dans `docs/dev_RZ_HTML/dossier_technique_V1.html`, et l'*où en est-on* dans
> `docs/synthese_etat.md`.
> **Précaution** : document historique. Il décrit des états passés, dont
> certains sont périmés **à dessein**. Les pièges de lecture sont réunis au §8 ;
> les lire avant de citer quoi que ce soit d'ici.
> **Version** : 1.0 — 22/10/2026 — Vincent Philippe
> **Sources** : fusion de `RZ-info-historique.odt` (19/12/2025) et de la partie I
> de `historique.odt` (23/03/2026). Les deux originaux sont dans
> `AA-Doc-horsGit\A - Architecture_Tables_Schemas\old\`.
> La partie II de `historique.odt` n'est pas reprise ici : c'est un chapitre
> d'architecture, pas d'histoire, et toutes ses valeurs réseau sont périmées.

---

## 1. Pourquoi ce robot existe

RobotRZ est un robot compagnon mobile, modulaire, piloté par MQTT, dont
l'intelligence est répartie entre quatre sous-systèmes.

**À l'origine, il n'y avait pas de cahier des charges.** Dans les mots du
projet : *« plus une envie et un fantasme, un rêve à développer, et donc une
phase d'exploration »*. L'organisation ne reposait pas sur un plan, mais sur
l'intégration et la maîtrise du plus grand nombre possible de capteurs,
d'actionneurs et de techniques — de construction comme de codage, dans
plusieurs langages.

L'intention était d'acquérir **les outils et l'expérience nécessaires à un
projet complexe, sans se focaliser sur un usage particulier**. C'est une clé de
lecture importante : beaucoup de choix anciens s'expliquent par la volonté
d'essayer, non par un besoin fonctionnel.

L'objectif fonctionnel, lui, est resté stable : un robot pilotable à distance,
dont les capteurs et actionneurs informent l'utilisateur de l'environnement, et
que l'on télécommande.

### Ce que le projet apporte

- Une communication robuste par le protocole **VPIV**
- Une modularité qui facilite les évolutions et l'ajout de modules
- Une intégration fluide avec des applications Android tierces

### Usages visés

| | |
|---|---|
| **Robotique domestique** | Surveillance, assistance, automatisation de tâches |
| **Pédagogie** | Apprendre l'électronique, la programmation et les réseaux sur une plateforme réelle |
| **Prototypage** | Socle pour des projets plus ambitieux |

---

## 2. Les acteurs

### Matériel

| Composant | Rôle |
|---|---|
| **Arduino Mega** | Capteurs et actionneurs, temps réel |
| **Arduino UNO** | Esclave moteur exclusif |
| **Smartphone SE** | Cerveau embarqué — Termux, Tasker, MQTT, capteurs du téléphone |
| **Raspberry Pi** | Broker MQTT et serveur Node-RED, sur la box |
| **SP** | Interface de pilotage — tableau de bord Node-RED |

### Logiciel

| Composant | Rôle | Technologie |
|---|---|---|
| **Node-RED** | Orchestration des flux et interface | JavaScript / Node.js |
| **Tasker** | Automatisation sur SE | Android |
| **VPIV** | Protocole de communication unifié | MQTT et série |

---

## 3. V0 à V4 — la phase d'exploration

> *La numérotation des versions a été faite a posteriori.*

Les V0 à V4 sont une lente évolution continue, sans rupture nette. Elles ont
permis de choisir une forme, d'éprouver des matériels et des systèmes
d'alimentation, et d'explorer des modes d'encodage de l'information. Plusieurs
câblages se sont succédé, ainsi que des modifications du tronc et de la tête —
ces dernières pour maîtriser les oscillations de la caméra.

**La toute première version reposait sur un RPi 3**, et la liaison SE ↔ SP se
faisait en Bluetooth.

Le code n'était pas factorisé, ou très peu. Les boucles Arduino pouvaient être
longues, avec des éléments bloquants. Le programme a été optimisé
progressivement — *« dans la douleur, du fait de l'absence de factorisation »*.

Plusieurs modes de fonctionnement étaient prévus dès le début, mais se
limitaient à activer ou désactiver des capteurs et actionneurs. Trois buts :
économiser l'énergie, éviter l'inflation du temps de traitement de l'Arduino,
et adapter l'affichage au contexte — déplacement, communication.

### La V0 en détail

| Poste | Élément | Rôle |
|---|---|---|
| Microcontrôleur | Arduino Uno puis Mega + Raspberry Pi 3 B+ | Capteurs sur le Mega, vidéo et IA sur le RPi |
| Caméra | Module Raspberry Pi Camera | Retour image par OpenCV |
| Microphones | 1 micro, puis carte 6 micros ReSpeaker | Présence/absence, puis localisation du son — la carte 6 micros n'a pas migré vers la V4 |
| Ultrasons | 6 HC-SR04 + 1 sur servomoteur en rotation sur la tête | Détection d'obstacles, avec retour visuel |
| LED | Anneau de LED + LED RGB | Communication visuelle, émotions |
| Écran | LCD I2C | 32 caractères |
| Buzzer | Actif | Alertes sonores |
| Enceinte | JBL Bluetooth | Qualité audio |
| Communication | Bluetooth HC-05, I2C, USB-UART, Wi-Fi | Un second Arduino dédié aux moteurs apparaît en cours de route |
| SP | Tablette | Écran de pilotage unique |
| Alimentation | Batterie plomb 12 V | D'abord des piles ou une alimentation externe |

### Protocoles de la V4

| Protocole | Usage |
|---|---|
| Bluetooth | Pilotage par tablette ou smartphone |
| I2C | Entre RPi, Arduino et écran LCD |
| USB-UART | Entre RPi et Arduino |
| Wi-Fi | Vidéo, audio, commandes en réseau local |
| MQTT | Échanges RPi ↔ Arduino |

Structure du code : **programme Arduino monolithique**, un seul `.ino`, aucune
factorisation.

---

## 4. Ce que la V4 a prouvé, et pourquoi elle devait mourir

La V4 était le résultat d'une construction progressive, au fil des besoins
constatés. Elle atteignait les objectifs en termes de **potentiel** — mais
*« sans économie de moyen, sans conception structurée »*.

Le constat est net, et c'est lui qui justifie tout ce qui suit :

> On avait une preuve de fonctionnement, mais un système à bout de souffle,
> devenu quasiment impossible à maintenir **malgré une inflation de
> documentation**.

Cette dernière remarque mérite d'être relevée : l'abondance de documentation
n'avait pas rendu le système maintenable. C'est une leçon que le projet a mise
en pratique depuis — la documentation du projet est elle-même régulièrement
élaguée et réorganisée, plutôt qu'accumulée.

La phase d'exploration n'a donc pas échoué : **elle a produit exactement ce
qu'on lui demandait** — l'épreuve des techniques, la fiabilité des options
alternatives, la réflexion sur les usages. De quoi concevoir une V6 structurée,
la V5 en étant la version de travail et de test.

---

## 5. V4 → V6 : les décisions

Trois objectifs : répondre aux défauts de la V4, garantir la fiabilité,
faciliter les évolutions.

### 5.1 Structure physique

- **Articulation du tronc** — validée. Augmente la stabilité en déplacement en
  limitant l'oscillation de la caméra.
- **Articulation de la tête sur deux axes** — validée. Cohérente avec l'axe du
  tronc, elle enrichit les expressions et fonctionne sans les moteurs de
  déplacement.
- **Deux positions d'équilibre** — tronc levé et tronc couché, pour que la
  coupure des servomoteurs (économie d'énergie) ne fasse pas s'affaisser le
  robot, quelle que soit sa position initiale.
- **Plus aucun capteur sur support aluminium** — trop de faux contacts à
  l'usage, sous l'effet des secousses et des manipulations.
- **Accès fiabilisé à l'électronique** — convertisseurs notamment, et plaque
  avant renforcée.
- **Fixation des batteries fiabilisée**, et maintenues **le plus à l'arrière
  possible** pour l'équilibre au freinage.
- **La forme de la V4 est la forme finale.** Hauteur, largeur et silhouette
  sont déjà optimales pour éviter les blocages et permettre le transport. Seul
  le poids reste à optimiser — pour la consommation comme pour la manipulation
  — avec une poignée de portage solide.

### 5.2 Câblage

- **Aucun montage non fixé.** Bornier ou soudure, systématiquement.
- **Deux transitions démontables seulement** : tête ↔ corps, et avant ↔ arrière
  du corps, pour l'entretien et la réparation.
- **Lisibilité** : distinction visible entre alimentation primaire, 5 V et
  câbles de données. Protection des câbles prévue.
- **Aucun relais de câblage de données**, hors les deux transitions ci-dessus.

### 5.3 Intelligence embarquée — la décision structurante

La V4 répartissait le traitement **sans logique stricte** entre le smartphone
embarqué, un RPi 4 B+ 8 Go, l'Arduino Mega et l'Arduino UNO. Cette répartition
était le produit d'ajouts successifs, pas d'une conception.

**Le RPi embarqué est supprimé.** Le raisonnement :

- Il faisait double emploi avec le smartphone, sur lequel Termux apporte un
  Linux complet. Associé à Tasker et aux applications Android tierces, le
  téléphone fusionne tous ces usages.
- Ses ports GPIO ne servaient qu'à la caméra — qui elle-même faisait double
  emploi avec celle du smartphone. Les capteurs ont toujours été sur l'Arduino,
  pour bénéficier de sa gestion d'horloge.
- Le smartphone **sécurise sa propre alimentation** grâce à sa batterie
  intégrée.
- On économise une alimentation dédiée.

> ⚠️ **Ne pas confondre deux Raspberry Pi.** Celui qui est supprimé est le RPi 4
> **embarqué dans le robot**. Le RPi 3 qui héberge aujourd'hui le broker MQTT et
> Node-RED est une **autre machine**, fixe, sur la box. Voir
> `docs/installation_RPI_serveur.md`.

### 5.4 Structure des échanges

Tous les traitements lourds — OpenCV, IA — passent sur le smartphone embarqué.
Un intermédiaire disparaît, et Tasker ouvre au robot la porte des applications
Android, tout en fournissant les capteurs et actionneurs du téléphone.

**Sans carte SIM**, le SE fonctionne dès qu'il partage le réseau du SP, et via
Internet au besoin, puisqu'un serveur est installé sur la box.

L'Arduino garde ses capacités de traitement et sa gestion du temps, comme
avant, mais se connecte désormais par le **port USB-C du smartphone**, qui
transmet les données en parallèle au SP. Le SP, à proximité de l'utilisateur,
passe les commandes et reçoit les données via Node-RED.

Quatre principes en découlent, et ils sont toujours en vigueur :

1. **Chaque appareil gère ses tâches bas niveau** et n'échange que le
   nécessaire.
2. **Un protocole unique**, VPIV, pour tous les échanges.
3. **Node-RED conserve dans un fichier JSON** la définition et les valeurs par
   défaut de toutes les variables du système. Cette centralisation permet
   d'ajuster les paramètres des capteurs et actionneurs depuis un seul endroit.
   *C'est l'origine de ce qui s'appelle aujourd'hui la Table C.*
4. **Tout est factorisé**, l'Arduino en premier lieu. Aucune fonction bloquante
   n'y subsiste, et les modes de fonctionnement ont été redéfinis pour
   optimiser les échanges.

---

## 6. V0 → V6 en un tableau

| Élément | V0 | V6 | Nature de l'évolution |
|---|---|---|---|
| **Architecture** | Raspberry Pi + Arduino Mega | Smartphone SE + Arduino Mega | Simplification et centralisation sur SE |
| **Reconnaissance de forme** | Caméra RPi + OpenCV | Caméras du SE + OpenCV | Suppression du matériel dédié |
| **Microphones** | 1 micro | 3 micros | Localisation des sons |
| **LED** | Anneau + LED RGB | Anneau + ruban | Communication lumineuse enrichie |
| **Écran** | LCD I2C | Supprimé | Tasker génère des scènes sur l'écran du smartphone, au potentiel sans limite |
| **Protocoles** | Bluetooth, I2C, USB-UART, Wi-Fi | MQTT, Wi-Fi, USB-UART | Suppression de l'I2C, échanges simplifiés |
| **Programmes** | Monolithique `.ino` | Factorisé `.ino`, `.h`, `.cpp` | Maintenabilité et modularité |
| **Encodage** | Format personnalisé | Format uniformisé | Standardisation — **voir le §8, le format cité par la source est périmé** |
| **Autres capteurs** | Capteur de lumière, buzzer, ultrasons | Capteur de force pour la laisse électronique | Ajouts et suppressions sérialisés, pour éviter les effets de bord |

---

## 7. Repères chronologiques

| Jalon | Date | Ce qui change |
|---|---|---|
| **V4** | 15/06/2025 | Intégration de Node-RED et de MQTT |
| **V5** | 01/11/2025 | Refonte de l'architecture modulaire — version de travail préparatoire |
| **V6** | 17/12/2025 | Stabilisation du protocole VPIV, ajout de la gestion des urgences |

Les V0 à V3 ne portent pas de date : la numérotation leur a été attribuée
après coup, sur une évolution continue sans jalon net.

---

## 8. ⚠️ Pièges de lecture

Ce document rapporte des états passés. Trois de ses éléments **ne doivent pas
être cités comme actuels**.

### 8.1 Le format d'encodage mentionné est périmé

Les sources d'origine donnent comme format uniformisé de la V6 :

```
$Var,Instance,Prop,Valeur#          ← PÉRIMÉ, ne pas utiliser
```

C'est un **format intermédiaire** de la transition V4 → V6. Il sépare par des
virgules et place l'instance **avant** la propriété. Le format en vigueur est :

```
$CAT:VAR:PROP:INST:VAL#             ← actuel
```

Deux-points, et surtout **PROP en position 3, INST en position 4**. L'ordre
inverse a déjà causé des erreurs : deux fichiers de légende le présentaient
comme une convention délibérée, et ont été archivés pour cette raison le
21/10/2026. Référence qui fait foi : `docs/tables_RZ_descripteur.md`, §5.

### 8.2 Un en-tête de tableau erroné dans la source

Le dernier tableau de `historique.odt` est titré « V0 | V1 » alors qu'il compare
V0 et V6 — son contenu reprend et prolonge le tableau V0/V4 qui le précède. Le
tableau du §6 ci-dessus porte le bon intitulé.

### 8.3 Les documents associés cités n'existent plus là où on les cherche

`RZ-info-historique.odt` listait trois documents associés. Après le rangement
d'octobre 2026 :

| Document cité | Où il est |
|---|---|
| `README_Projet_RobotZ_V1.md` | `A - Architecture_Tables_Schemas\old\` |
| `syntheseNewRobot.odt` | `old\syntheseNewRobotAfinir.odt` — 1,8 Mo, inachevé |
| `ModeFonctionnement-RZ-Miro.jpg` | `old\` — le chemin `\DOC\00_PROJET` n'existe plus |

---

*Fusion réalisée le 22/10/2026. Les deux sources ne se recouvraient pas : le
recouvrement mesuré entre elles était nul. `RZ-info-historique` apportait le
pourquoi et les acteurs, `historique.odt` l'évolution matérielle et les
décisions de conception. Leur réunion était donc une addition, sans arbitrage
à rendre — hors les trois pièges du §8.*
