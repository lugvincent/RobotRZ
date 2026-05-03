:: Passage en UTF-8
chcp 65001 >nul

@echo off
:: Définition de l'IP fixe (si elle ne change pas) - attention vérifier pour ce script que blutooth du tel ok, que termux et ssh lance sshd 
set IP_ADDRESS=192.168.1.13

:: Vérification de la connexion au téléphone
ping -n 2 %IP_ADDRESS% >nul
if %errorlevel% neq 0 (
    echo Erreur : L'appareil %IP_ADDRESS% ne répond pas au ping.
    pause
    exit /b
)

:: Définition du nom d'utilisateur SSH
set USERNAME=u0_a234

:: Connexion SSH 
echo Connexion à %USERNAME%@%IP_ADDRESS%...
:: Lancement de la connexion SSH via OpenSSH -mieux que Plink gere les couleurs ANSI et compatible avec les clés-
ssh -p 8022 %USERNAME%@%IP_ADDRESS%


pause
