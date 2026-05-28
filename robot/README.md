## Focus sur le sous-projet robot

Ce README décrit l état actuel du robot après refactorisation des interruptions et du debug UART.

### Objectif actuel du code robot

Le code robot est structuré autour de trois axes:

- émission IR périodique d une trame robot
- réception et acquisition capteurs via interruptions
- export debug UART à 50 Hz, piloté par micro-switchs ou commandes wire

## Architecture logicielle robot

### Boucle principale

Voir [robot/src/main.c](robot/src/main.c).

Séquence d initialisation actuelle:

- SystemInit
- UART0
- PWM IR + timer d enveloppe IR
- module debug moteurs
- module debug proximètre
- module capteur inductif (ADC + clock + enveloppe)
- SysTick à 50 Hz

Boucle infinie:

- attend le flag 50 Hz
- vérifie que le debug UART global est activé
- envoie les trames debug moteurs et inductif
- n envoie pas la trame proximètre ici (contrainte: envoi en fin de balayage)

### Gestion centralisée des IRQ

Voir [robot/src/interruptions.c](robot/src/interruptions.c).

Principe:

- un seul handler EINT3 partagé dispatch vers les routines module
- EINT1 dédié à la réception SPI
- TIMER0 dédié à l émission IR
- SysTick ne fait que lever un drapeau de cadence

Routage actuel:

- EINT3: dtmf, moteurs, capteurInductif, proximètre
- EINT1: recepSPI
- TIMER0: emissionIR

## Modules robot (état détaillé)

### 1) Émission IR

Fichiers:

- [robot/src/emissionIR.c](robot/src/emissionIR.c)
- [robot/src/emissionIR.h](robot/src/emissionIR.h)
- [robot/src/toolsIR.c](robot/src/toolsIR.c)

Ce qui est implémenté:

- PWM 38 kHz sur P1.22
- timer enveloppe IR en us (base typique 250 us)
- machine d état d émission dans l IRQ TIMER0
- génération d une trame 16 bits utile (id, vitesse, status, checksum)

Format logique de trame:

- start: 10
- bit 1: 110
- bit 0: 100
- checksum: complément à 2 sur 4 bits

### 2) Capteur inductif + debug UART

Fichiers:

- [robot/src/capteurInductif.c](robot/src/capteurInductif.c)
- [robot/src/capteurInductif.h](robot/src/capteurInductif.h)

Ce qui est implémenté:

- ADC sur P0.24, P0.25, P0.26
- clock acquisition sur front descendant P0.27
- enveloppe sur P0.28 (front montant: reset salve, front descendant: fin salve)
- mesure de durée d enveloppe avec Timer2
- période exposée en microsecondes
- calcul de moyennes par salve
- debug UART multi-modes via 3 micro-switchs
- mode wire one-shot quand mode HW = 111

Sorties debug inductif:

- I période us
- a angle
- X distance arrière
- x distance avant
- A/B/C valeurs ADC moyennées

Important:

- le point d entrée de décodage enveloppe est appelé, mais le décodage métier complet n est pas encore implémenté (voir module decode_enveloppe)

### 3) Moteurs (debug)

Fichiers:

- [robot/src/moteurs.c](robot/src/moteurs.c)
- [robot/src/moteurs.h](robot/src/moteurs.h)

Ce qui est implémenté:

- lecture de 2 switchs de mode
- routine interruption switchs
- réception commande wire 1XX (100, 101)
- émission debug UART (W, V, G/D)
- mode wire overwrite one-shot si mode HW = 11

Ce qui reste à implémenter:

- commande/contrôle moteur réel
- boucle d asservissement
- acquisition réelle vitesse/consignes hors valeurs de simulation

### 4) Proximètre (debug)

Fichiers:

- [robot/src/proximetre.c](robot/src/proximetre.c)
- [robot/src/proximetre.h](robot/src/proximetre.h)

Ce qui est implémenté:

- lecture de 2 switchs de mode
- routine interruption switchs
- génération de trame debug de balayage

Format trame:

- en-tête t ou T
- 72 mesures (pas de 5 degrés)

Ce qui reste à implémenter:

- chaîne proximètre métier (pilotage servo, acquisition capteur réelle, fin de balayage)
- appel effectif de debug_proximetre_send_frame en fin de balayage réel

### 5) Décodeur enveloppe

Fichiers:

- [robot/src/decode_enveloppe.c](robot/src/decode_enveloppe.c)
- [robot/src/decode_enveloppe.h](robot/src/decode_enveloppe.h)

État actuel:

- API en place: decode_enveloppe_commande(period_us)
- implémentation actuelle: stub

Ce qui reste à implémenter:

- reconstruction de trame à partir de la série de périodes
- routage final des commandes vers modules concernés

### 6) DTMF

Fichiers:

- [robot/src/dtmf.c](robot/src/dtmf.c)
- [robot/src/dtmf.h](robot/src/dtmf.h)

État:

- parsing de séquence DTMF type #IDAction*
- mise à jour d un état interne de mouvement
- pilotage LED statut/concerne

Note:

- module présent dans le dispatch EINT3
- initialisation explicite du module DTMF non appelée actuellement dans main

### 7) Réception SPI

Fichiers:

- [robot/src/recepSPI.c](robot/src/recepSPI.c)
- [robot/src/recepSPI.h](robot/src/recepSPI.h)

État:

- lecture bit à bit sur interruption EINT1
- gestion CS et reconstruction octet
- stockage de valeurs décodées (Vg, Vd, Pg, Pd)

Note:

- module routé dans EINT1
- initialisations SPI/GPIO dédiées non appelées actuellement dans main

### 8) État global robot

Fichiers:

- [robot/src/robotState.c](robot/src/robotState.c)
- [robot/src/robotState.h](robot/src/robotState.h)

Rôle:

- stockage centralisé des valeurs robot
- getters/setters utilisés par les modules
- drapeau global activation debug UART
- mode simulation capteurs via SIMULATE_SENSOR_VALUES

## Logique debug UART refactorée

Le debug n est plus centralisé dans un module unique. Chaque module gère:

- ses switchs
- son mode courant
- son émission de trame
- son mode wire overwrite one-shot si applicable

Cadence:

- 50 Hz via SysTick pour moteurs et inductif dans main
- proximètre: prévu à chaque balayage (pas dans la boucle 50 Hz)

## Brochage principal utilisé (robot)

Inductif:

- P0.24 AD0.1
- P0.25 AD0.2
- P0.26 AD0.3
- P0.27 clock
- P0.28 enveloppe

Moteurs debug switchs:

- P0.11
- P0.12

Proximètre debug switchs:

- P0.12
- P0.13

IR:

- P1.22 PWM IR 38 kHz

DTMF:

- données sur P0.16..P0.19
- valid tone sur P0.20

SPI réception:

- EINT1 sur P2.11
- MISO sur P0.8
- CS sur P0.6, P0.7, P0.9, P0.10

UART:

- UART0 sur P0.2 TX, P0.3 RX

## État d avancement (important)

Ce qui est disponible maintenant:

- architecture interruptions refactorée
- infrastructure debug UART modulaire
- acquisition inductif de base + métriques + trames debug
- émission IR fonctionnelle

Ce qui est explicitement encore à implémenter côté métier:

- module moteurs: logique métier réelle (pas seulement debug)
- module proximètre: logique métier réelle (pas seulement debug)
- module détecteur/décodeur enveloppe: algorithme complet (actuellement stub)

En résumé:

- aujourd hui, la couche debug est en place
- les couches fonctionnelles métier restent en cours sur moteurs, proximètre et décodage enveloppe

## Plan de tâches robot

### Tâches terminées

- refactor des interruptions autour de [robot/src/interruptions.c](robot/src/interruptions.c)
- modules debug séparés: [robot/src/moteurs.c](robot/src/moteurs.c), [robot/src/capteurInductif.c](robot/src/capteurInductif.c), [robot/src/proximetre.c](robot/src/proximetre.c)
- mode simulation UART centralisé via [robot/src/robotState.h](robot/src/robotState.h)
- point d entrée de décodage enveloppe exposé via [robot/src/decode_enveloppe.h](robot/src/decode_enveloppe.h)

### Tâches en cours

- finalisation des trames debug et des modes micro-switch/wire

### Tâches restantes à implémenter (métier)

#### Moteurs

- brancher la commande réelle des actionneurs (pas seulement trame debug)
- brancher la mesure réelle des vitesses/PWM de retour
- implémenter la boucle d'asservissement (linéaire + angulaire)
- relier les commandes décodées wire aux consignes moteur métier

#### Proximètre

- implémenter le cycle de balayage réel (servo + acquisition)
- remplir les 72 mesures avec des données capteur réelles
- appeler `debug_proximetre_send_frame()` en fin de balayage réel
- ajouter filtrage/validation des mesures (saturation, valeurs invalides)

#### Détecteur / décodeur enveloppe

- implémenter [robot/src/decode_enveloppe.c](robot/src/decode_enveloppe.c)
- implémenter l'interprétation de la suite de périodes (us) en symboles
- assembler les trames et détecter les erreurs (timing hors plage, checksum si applicable)
- router les commandes décodées vers les bons modules (moteurs, inductif, autres)

### Tâches d'intégration système

- appeler explicitement les init manquantes des modules utilisés en IRQ (DTMF/SPI selon configuration)
- valider la cohérence du brochage réel carte vs brochage documenté
- ajouter des tests

### Critères de validation finale

- debug UART conforme à la spécification sur tous les modes
- moteurs/proximètre/enveloppe opérationnels en mode capteurs réels

## Build et exécution

Projet Keil robot:

- [robot/Robot.uvprojx](robot/Robot.uvprojx)

Recommandation de validation:

- vérifier les trames UART avec SIMULATE_SENSOR_VALUES à 1
- puis basculer en mode réel capteurs et valider module par module
