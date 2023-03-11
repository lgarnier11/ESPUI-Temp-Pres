# ESPUI-Temp-pres
 lgarnier11<br/>
 v1.1.0 - 07/02/2023<br/>
 Adaptation en c++ du "framework" Arduino de Neodyme (afin d'utiliser vscode...).<br/>
 Objectif : gestion d'une sonde de température (étanche) et d'une sonde de pression.<br/>
 Contraintes : alimentation autonome, communication via wifi (Utilisation dans une cloche à vide).<br/>
 Le circuit devient serveur wifi, et présente une page contenant trois onglets :<br/>
 - Mesures      : Température et pression
 - Debug        : Affichage des messages
 - Paramétrages : Délais des mesures, wifi

 Par soucis de simplicité, on passe de 4 fichiers .ino à un seul fichier .cpp
