# Quelle branche est laquelle — à lire avant de pousser

> **Une seule branche sert au développement : `main`.** Les trois autres sont des
> traces historiques, figées. Ce fichier est à la racine du dépôt pour qu'on tombe
> dessus sans le chercher.
>
> *V1 — 23/10/2026 — Vincent Philippe / Claude*

---

## Le tableau, en un coup d'œil

| Branche | Dernier commit | Rôle | On y pousse ? |
|---|---|---|---|
| **`main`** | 23/10/2026 | **La branche de travail.** Tout le développement, toute la documentation à jour | ✅ **oui, et elle seule** |
| `sauvegarde/avant-correction-VPIV-2026-10-22` | 22/10/2026 | Point de retour figé, posé avant la campagne de correction de l'ordre VPIV | 🔒 jamais |
| `vMars` | 14/03/2026 | *« version vérifiée stabilisée pour SE et Arduinos »* — un jalon de mars | 🔒 jamais |
| `master` | 03/05/2026 | **L'ancien dépôt**, structure d'avant la réorganisation de mars 2026 | 🔒 jamais |

---

## Comment ne pas se tromper

```bash
git branch --show-current        # dit sur quelle branche on est
git switch main                  # revient sur la bonne
```

Deux habitudes qui suffisent :

1. **Vérifier avant de pousser.** `git branch --show-current` doit répondre `main`.
2. **Se méfier de `master`.** C'est le piège principal : beaucoup d'outils et de
   tutoriels supposent que la branche principale s'appelle `master`. Ici, **non**.

Pour revenir au point de retour en cas de besoin :

```bash
git reset --hard origin/sauvegarde/avant-correction-VPIV-2026-10-22
```

---

## Pourquoi `master` est à part — et ce n'est pas un détail

**`master` n'a aucun ancêtre commun avec `main`.** Ce ne sont pas deux branches d'un
même arbre : ce sont deux histoires séparées. `main` a été reparti de zéro lors de la
réorganisation de mars 2026.

| | |
|---|---|
| Commits que `master` a et que `main` n'a pas | **8** (août 2025 → mai 2026) |
| Fichiers présents sur `master` et absents de `main` | **669** |
| Ancêtre commun avec `main` | **aucun** |

Ces 669 fichiers sont l'ancienne arborescence — `DOC/`, `CODE/`, `FUTUR-DOC/` — dont le
contenu utile a depuis été trié dans `AA-Doc-horsGit`. **Rien n'a été perdu** : `master`
est un dépôt-musée, pas une branche oubliée en cours de route.

`vMars` est différent : il partage l'ancêtre `18f6aba` avec `main` et n'a que 4 commits
propres, dont celui qui actait la suppression de la documentation lors de la
réorganisation.

---

## 🔴 Ce que `master` porte encore, et que le nettoyage de `main` n'a pas pu atteindre

Le nettoyage de sécurité d'octobre 2026 a porté sur `main`. **`master` n'a jamais été
nettoyé, et le dépôt est public.** Relevé le 23/10 :

| Sur `master` | Où |
|---|---|
| L'adresse IPv4 **publique** du serveur, en clair, libellée « Adresse public du serveur » | `CODE/Mes-Scripts/IMPORTANTCommandesBase.txt`, son archive, et le `.drawio` |
| `SSH (local) Id : u0_a234` et le port `SSH 8022` | `DOC/01_ARCHITECTURE/DiagrapRZCom-Protocole-Appli.drawio` |
| Le nom DuckDNS | 18 occurrences |

**Ce qui n'y est pas** — vérifié : aucun jeton GitHub, aucune clé API, aucun mot de passe
MQTT, aucune adresse IPv6. Les longues chaînes du `.drawio` qui contiennent « SSH » sont
du **base64** de dessin, pas des clés.

### Ce qui atténue, et ce qui n'atténue pas

L'adresse IPv4 date de 2025 et votre connexion est dynamique — c'est d'ailleurs pourquoi
vous utilisez DuckDNS. **Elle est donc très probablement périmée.**

Ce qui ne s'atténue pas : l'identifiant SSH `u0_a234` et le port `8022` restent valables,
et le nom DuckDNS, lui, suit l'adresse. Les trois ensemble décrivent un point d'entrée.

### Les options, quand le chantier sécurité s'ouvrira

| Option | Effet | Réserve |
|---|---|---|
| Supprimer la branche `master` | Elle disparaît de la vue GitHub | Les commits restent un temps accessibles par leur empreinte ; des copies peuvent exister |
| Passer le dépôt en **privé** | Ferme tout d'un coup, y compris l'historique | Le dépôt n'est plus consultable publiquement |
| Ne rien faire | — | L'adresse est probablement périmée, l'identifiant SSH ne l'est pas |

**Le dépôt étant public, c'est le point à trancher en même temps que la redirection de
la box**, pas après.

---

## Les quatre fausses adresses à ne pas poursuivre

Un balayage de `master` fait remonter cinq adresses IPv4 d'apparence publique. **Quatre
sont des fautes de frappe** de `192.168.1.40` — `192.128.1.40`, `192.198.1.40`,
`198.168.1.40` — plus `26.2.0.3`, qui est un numéro de version pris pour une adresse.
Seule la cinquième est réelle.

---

*Vérifications refaites par `outils/vpiv/audit_ordre_global.py` et les commandes `git`
citées ci-dessus.*
