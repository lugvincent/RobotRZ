:: Passage en UTF-8
chcp 65001 >nul

@echo off
set SERVER_IP=192.168.1.13
set IP_ADDRESS=

:: Essayer de récupérer l'IP du smartphone via current_ip.txt
::for /f %%i in ('powershell -Command "(Invoke-WebRequest -Uri http://%SERVER_IP%:8080/current_ip.txt -UseBasicParsing).Content"') do set IP_ADDRESS=%%i

:: Debug : Afficher la valeur brute récupérée
echo Valeur brute récupérée : "%IP_ADDRESS%"

:: Vérifier si l'IP a bien été récupérée
if "%IP_ADDRESS%"=="" (
    echo Impossible de récupérer l'IP. Utilisation de l'IP fixe %SERVER_IP%.
    set IP_ADDRESS=%SERVER_IP%
)

echo IP détectée : %IP_ADDRESS%

:: Vérifier si le périphérique est joignable
ping -n 2 %IP_ADDRESS% >nul
if %errorlevel% neq 0 (
    echo Erreur : L'appareil %IP_ADDRESS% ne répond pas au ping.
    pause
    exit /b
)

:: Définition du nom d'utilisateur SSH
set USERNAME=u0_a234

:: Tentative de connexion SSH avec Plink
echo Connexion à %USERNAME%@%IP_ADDRESS%...
:: Lancement de la connexion SSH via OpenSSH (mieux que Plink gere les couleurs ANSI et compatible avec les clés
ssh -p 8022 %USERNAME%@%IP_ADDRESS%

pause
