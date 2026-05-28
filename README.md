# robot-mobile-pilote-sorbonne-2026

Projet 2026 de L3 Electronique (Sorbonne Université) pour réaliser un système de livraison de colis en environnement industriel.

Le dépôt contient 4 sous-projets:

- robot
- poste
- centrale
- supervision

Chaque sous-projet contient ses sources, des tests unitaires ou maquettes, et son projet Keil si applicable.

## Sous-projet robot

Documentation détaillée: [robot/README.md](robot/README.md)

### Tâches robot terminées (refactor)

- architecture IRQ centralisée avec dispatch EINT3 par module
- séparation du debug UART par module (moteurs, inductif, proximètre)
- cadence debug 50 Hz via SysTick pour moteurs et inductif
- acquisition inductive de base (ADC + clock + enveloppe) et trames debug
- mode simulation capteurs pour tests UART via SIMULATE_SENSOR_VALUES

### Tâches robot à implémenter (priorité)

- moteurs: implémenter la logique métier réelle (commande, mesure, asservissement)
- proximètre: implémenter la chaîne métier réelle (pilotage balayage + acquisition)
- détecteur/décodeur enveloppe: remplacer le stub par l algorithme de décodage complet

### Tâches robot à intégrer dans la boucle système

- appeler `debug_proximetre_send_frame()` à la fin du balayage réel (et non dans la boucle 50 Hz)
- appeler les initialisations manquantes des modules branchés dans les IRQ si utilisés en production (DTMF/SPI)
- compléter le routage final des commandes décodées (capteur inductif, dfecodeur enveloppe) vers les modules applicatifs

### Critère de fin de lot robot

- tous les modules debug conservent leur format UART actuel
- moteurs/proximètre/enveloppe fonctionnent en mode capteur réel (pas seulement simulation)
- les chemins IRQ utilisés en production sont tous initialisés explicitement au démarrage
