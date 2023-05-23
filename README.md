# ESPUI-Temp-pres
 lgarnier11<br/>
 v1.1.2 - 22/05/2023<br/>
 si PRESSURE_MEASUREMENT n'est pas défini, on ne charge pas la librairie BMP280 (#define PRESSURE_MEASUREMENT)<br/>
 refactoring des logs<br/>
 v1.1.1 - 21/05/2023<br/>
 Mise en commentaire de la gestion de la pression (le module consomme trop)<br/>
 Mise en commentaire de la communication vers l'application Livet<br/>
 v1.1.0 - 07/02/2023<br/>
 Adaptation en c++ du "framework" Arduino de Neodyme (afin d'utiliser vscode...).<br/>
 Objectif : gestion d'une sonde de température (étanche) et d'une sonde de pression.<br/>
 Contraintes : alimentation autonome, communication via wifi (Utilisation dans une cloche à vide).<br/>
 Le circuit devient serveur wifi, et présente une page contenant trois onglets :<br/>
 - Mesures      : Température et pression
 - Debug        : Affichage des messages
 - Paramétrages : Délais des mesures, wifi

 Par soucis de simplicité, on passe de 4 fichiers .ino à un seul fichier .cpp
