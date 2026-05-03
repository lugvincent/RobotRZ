:: ============================================================
:: Fichier    : SSH_Win_Xiaomi_V4.bat
:: Objectif   : Connexion SSH au smartphone + démarrage services
:: Description: Ping de vérification, connexion SSH,
::              lancement automatique de start_services.sh
::              Node-RED tourne dans tmux → survit à la session SSH
:: Précaution : IP fixe à vérifier si changement de réseau
::              Bluetooth téléphone OK + Termux ouvert + sshd lancé
:: Version    : 5.0 — 2026-04 — (votre nom)
:: ============================================================

@echo off
chcp 65001 >nul

set IP_ADDRESS=192.168.1.13
set USERNAME=u0_a234
set PORT=8022

:: === Vérification connexion ===
echo 🔍 Vérification connexion vers %IP_ADDRESS%...
ping -n 2 %IP_ADDRESS% >nul
if %errorlevel% neq 0 (
    echo ❌ Erreur : %IP_ADDRESS% ne répond pas.
    pause
    exit /b
)

:: === Connexion SSH + lancement des services ===
echo ✅ Téléphone joignable.
echo 🚀 Connexion SSH et démarrage des services...

:: -t force un terminal interactif (nécessaire pour les couleurs)
:: Le second argument est la commande lancée dès la connexion
ssh -t -p %PORT% %USERNAME%@%IP_ADDRESS% ^
    "bash ~/robotRZ/scripts/start_services.sh ; exec bash"

pause
