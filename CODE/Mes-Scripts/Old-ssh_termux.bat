@echo off
set IP_ADDRESS=192.168.1.12
set USERNAME=u0_a154
set PASSWORD=nthLEga99?

echo Connexion en cours...
echo y | plink -ssh -P 8022 %USERNAME%@%IP_ADDRESS% -pw %PASSWORD%
pause
