:: (alternative à REM pour commentaire dans un bat) _ 
:: Ici on active à la maison, depuis mon PC portable une liaison SSH avec Doro, sachant que ce dernier ouvre SSH dès que termux est lancé
:: La clef public de ssh windows a été donné à DOro pour le permettre et on a enlever le code préliminaire à l'utilisation de la clef
:: il ne reste plus qu'à envoyer la commande d'ouverture avec l'id de Doro/ssh  : u0_154 en précisant le port !
@echo off
:: old ssh u0_a154@192.168.1.12 -p 8022 - Adresse des clef ssh sur windows la privé id_rsa dans C:\Users\Vincent.VINCENTPHACER\.ssh
:: ssh -i C:\Users\Vincent.VINCENTPHACER\.ssh\id_rsa u0_a154@192.168.1.12 -p 8022 - ci-dessous v moins compact :
:: Chemin complet vers la clé privée sur mon PC
set SSH_KEY=C:\Users\Vincent.VINCENTPHACER\.ssh\id_rsa
:: Connexion SSH
ssh -i %SSH_KEY% u0_a154@192.168.1.12 -p 8022
