# robot-mobile-pilote-sorbonne-2026

Projet 2026 de L3 Electronique (Sorbonne Université) pour réaliser un système de livraison de colis en environnement industriel.

Le dépôt contient 4 sous-projets:

- robot
- poste
- centrale
- supervision

Chaque sous-projet contient ses sources, des tests unitaires ou maquettes, et son projet Keil si applicable.

---

## Supervision (Logiciel PC)
*Mockup - Cette partie est à détailler dans le sous-projet correspondant.*

Le logiciel de supervision offre une vue symbolique et dynamique du circuit (en forme de "8" couché) et permet à un opérateur de suivre les robots en temps réel. L'opérateur peut attribuer des missions de transport de colis entre les différents postes de l'usine, visualiser les statuts des robots ("libre", "rdv_expedition", "colispris", "rdv_depose") et monitorer leurs vitesses. La communication s'effectue de manière centralisée avec la Centrale.

## Centrale
*Mockup - Cette partie est à détailler dans le sous-projet correspondant.*

La centrale agit comme le cerveau de l'installation. Elle coordonne le trafic en interrogeant successivement chaque poste ouvrier. Elle pilote les robots en injectant des signaux de commande dans le fil de guidage inductif au sol, gère l'aiguillage, prévient les collisions aux jonctions de pistes, et met à jour l'état de l'ensemble de la flotte pour la supervision.

## Poste Ouvrier
*Mockup - Cette partie est à détailler dans le sous-projet correspondant.*

Chaque poste ouvrier dispose d'un clavier DTMF 16 touches permettant à l'ouvrier de demander des expéditions (ex: #15A* pour expédier le colis A vers le poste 15). Le poste relève les identifiants et statuts des robots qui passent devant lui par infrarouge, informe la centrale, et émet un signal ultrasonore pour s'annoncer au robot (fournissant son ID et son côté de piste).

---

## Robot Mobile (Détail Complet)

Le robot mobile est l'acteur principal de la livraison. De type "char" (différentiel, rotation par différence de vitesse et roulette folle arrière), il suit le fil au sol, décode les ordres émis par la Centrale, évite les collisions et interagit avec les ouvriers. 

L'avancement précis du code se trouve dans : [robot/README_TODO.md](robot/README_TODO.md).

Voici le détail exhaustif de ses sous-systèmes embarqués (basés sur le microcontrôleur LPC1769) :

### Bilan des Entrées/Sorties (Pins du LPC1769)

| Module | Signal | I/O | Port Px.y | Description simplifiée |
|---|---|---|---|---|
| **Traction** | PWM_MOT_G / D | OUT | P2.0, P2.1 | Commandes PWM moteurs gauche et droit |
| | SW_DBG_MOT[1:0] | IN | P0.4, P0.5 | Micro-switchs debug moteur |
| **UART** | UART0_TX / RX | I/O | P0.2, P0.3 | Transmission série debug (50Hz) |
| **Inductif** | ADC_BOBINE_1 à 3 | IN | P0.24 à P0.26 | Entrées analogiques des bobines |
| | CLOCK_FIL_50KHZ | IN | P0.27 | Horloge synchro porteuse fil |
| | ENVELOPPE_FIL | IN | P0.28 | Décodage des ordres |
| | TOP_MESURE_BOBINE | OUT | P1.25 | Top oscilloscope pour synchro |
| | SW_DBG_IND[2:0] | IN | P0.0, P0.1, P0.6 | Micro-switchs debug inductif |
| **IR (Émission)**| LED_IR_TX | OUT | P2.2 | LED Infrarouge (Statuts/Vitesse) |
| | SW_ID_ROBOT[3:0] | IN | P1.18 à P1.21| DIP Switchs pour l'ID du robot |
| | SYNC_TRAME_IR | OUT | P1.29 | Top oscilloscope synchro IR |
| **DTMF** | DTMF_Q[3:0] | IN | P0.16 à P0.19| Sorties de la puce HT9170D |
| | DTMF_DV | IN | P2.12 | Interruption Data-Valid |
| **Ultrasons** | ENVELOPPE_US | IN | P2.13 | Réception enveloppe ultrason |
| **Proximètre** | ADC_DIST_PROXI | IN | P0.23 | Entrée analogique GP2Y0A02YK0F |
| | PWM_SERVO_PROXI | OUT | P2.3 | Servomoteur de balayage |
| | SW_BALAYAGE[1:0] | IN | P1.30, P1.31| Switchs angle/vitesse balayage |
| | BUZZER_1KHZ | OUT | P2.4 | Avertisseur sonore |
| **Capa** | CAPA_COUNT | IN | P1.26 | Entrée fréquence oscillateurs |
| | CAPA_SEL[1:0] | OUT | P1.27, P1.28| Sélection du côté capacitif |
| **Sécurité** | ARRET_URGENCE_SW | IN | P2.10 | Bouton arrêt coup de poing |
| | BTN_COLIS_REC / DEC | IN | P2.6, P2.7 | Boutons ouvrier (chargé/déchargé) |
| **IHM** | LED_RGB_STATUS | OUT | P0.22, P3.25, P3.26 | LED RGB de statut (R, G, B) |
| | STEPPER_IN1..4 | OUT | P0.21, P2.5, P2.8, P4.28 | Moteur Pas-à-Pas (Lettre de service) |
| | LED_SHIFT (Data/Clk/Latch)| OUT | P2.9, P4.29, P0.20 | Registre à décalage (LEDs externes) |
| **Odométrie** | ODO_SCLK, MISO, CS[3:0] | I/O | P0.7 à P0.11, P0.15 | Bus SPI vers FPGA Basys2 |
| | ODO_DATA_READY | IN | P2.11 | Interruption 50Hz FPGA prêt |


### A. Déplacements et Motorisation
- **Moteurs et PWM** : La propulsion s'effectue via un pilotage PWM inaudible (période < 50 µs, soit 38 kHz). Il n'y a pas de marche arrière. Les commandes de vitesses se situent entre 25% et 80% de la vitesse maximale nominale.
- **Asservissement (Odométrie)** : La vitesse moyenne ($V_{moy}$) et la vitesse de rotation ($\omega_{rot}$) sont calculées pour suivre le fil. Une liaison SPI synchrone avec une carte Basys2 FPGA permet de récupérer les données des codeurs incrémentaux afin d'asservir la vitesse en temps réel (50 Hz).
- **Aiguillages** : Lors de commandes pour prendre la boucle Nord ou Sud à une jonction en Y, le robot gère l'aiguillage en discriminant les fréquences porteuses (motifs E, e, 0, o, 1, i) et bascule sur le bon signal pour rejoindre la destination.

### B. Guidage et Métrologie Inductive
- **Suivi de ligne** : Le robot est muni de bobines pour capter par induction le champ magnétique généré par un courant alternatif injecté dans le fil au sol.
- **ADC et Mesures** : Des capteurs inductifs (avant, arrière, horizontal) sont échantillonnés continuellement par les convertisseurs (ADC). Le robot analyse l'amplitude des signaux pour déduire son écartement latéral et son angle par rapport à la piste afin d'ajuster sa trajectoire.

### C. Réception des Ordres (Décodage d'enveloppe)
- Le signal alternatif dans le fil est modulé en tout ou rien. Le robot décode les chronogrammes de l'enveloppe pour reconnaître les zéros, les uns, et les entêtes de trames. 
- **Ordres possibles** : Changement de vitesse, assignation de route (Nord/Sud), ordres de chargement/déchargement pour un poste donné, ou changement de mode de débug.

### D. Prévention des Collisions et Obstacles
- **Sécurité aux jonctions (DTMF)** : À la jonction entre la branche Nord et Sud, la Centrale gère le trafic. Le robot possède un récepteur pour capter une séquence audio DTMF (`# [ID] A/D *`) de la centrale lui intimant de s'arrêter (`A`) ou de repartir (`D`).
- **Proximètre Infrarouge** : Un capteur IR directif (GP2Y0A02YK0F) est monté sur un servomoteur de balayage. Les mesures angulaires identifient les obstacles (piétons, etc.) pour réduire la vitesse ou stopper le robot.
- **Avertisseur sonore** : Un buzzer (1 KHz) émet des bips dont la fréquence d'espacement dépend de la distance de l'obstacle détecté (de 0,5 Hz à 100cm, jusqu'à 2,5 Hz à 20cm).
- **Sécurité Capacitive ("Fausse Peau")** : Des oscillateurs astables surveillent les 4 côtés du robot. La modification de la capacité induite par l'approche d'un piéton alerte le robot qui ralentit ou stoppe en fonction du côté concerné.

### E. Interactions avec les Postes et l'Usine
- **Communication Infrarouge (IR)** : Le robot annonce en permanence son identité (configurée par DIP switchs), sa vitesse et son statut via une puissante émission IR pulsée à 38 kHz (trame de 16 bits incluant le checksum).
- **Signaux Ultrasonores** : Les postes ouvriers émettent des enveloppes d'ultrasons. Le robot s'en sert pour calculer de quel côté de la piste se garer (gauche/droite) et pour s'arrêter précisément au niveau du poste cible en déviant du fil.

### F. Interface Homme-Machine et Statuts
- **Statuts dynamiques** : Le cycle logistique du robot est défini par 4 états symbolisés par une LED RGB : *Libre* (Vert), *RDV Expédition* (Rouge), *Colis pris* (Magenta) et *RDV Dépose* (Bleu).
- **Affichage lettre de service** : Un moteur pas-à-pas rotatif équipé de textes affiche pour l'ouvrier la nature du colis transporté (colisA, colisB, colisC, colisD, ou un espace vide).
- **Boutons Ouvrier** : Deux boutons physiques embarqués permettent à l'ouvrier d'indiquer "colis récupéré" ou "colis chargé" pour renvoyer le livreur sur la piste.
- **Sécurité matérielle** : Un arrêt d'urgence "coup de poing" suspend l'électronique de puissance et notifie le logiciel.

### G. Débogage et Mise au point (Télémétrie)
Le microcontrôleur émet par liaison série (UART), à une cadence de 50 Hz, des trames de données de télémétrie. Ces flux (distances inductives, valeurs PWM, balayages proximètre) sont configurables "à chaud" en changeant la position de micro-switchs, ou par des commandes du fil, offrant une vue complète du système en direct.
