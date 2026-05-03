:: passage utf8
chcp 65001 >nul

@echo off
:: Définition des variables
set SERVER_IP=192.168.1.13
set IP_ADDRESS=

:: Essayer de récupérer l'IP du smartphone via current_ip.txt - Ne fonctionne que si le serveur Python est activé coté termux
for /f %%i in ('powershell -Command "(Invoke-WebRequest -Uri http://%SERVER_IP%:8080/current_ip.txt -UseBasicParsing).Content"') do set IP_ADDRESS=%%i

:: Si l'IP n'est pas récupérée, utiliser l'IP fixe
if "%IP_ADDRESS%"=="" set IP_ADDRESS=%SERVER_IP%
echo IP récupérée: %IP_ADDRESS%
:: Définition du nom d'utilisateur SSH
set USERNAME=u0_a234

:: Affichage d'un message de connexion
echo Connexion à %IP_ADDRESS% ...

:: Lancement de la connexion SSH via OpenSSH (mieux que Plink gere les couleurs ANSI et compatible avec les clés
ssh -p 8022 %USERNAME%@%IP_ADDRESS%


pause
