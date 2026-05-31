# Suivi d'Avancement - Projet Robot Mobile

Ce document rassemble toutes les exigences du cahier des charges spécifiques au **robot**, avec l'état d'avancement du code et des cases pour le suivi du montage matériel et des tests.

Légende :
- `[ ]` À faire
- `[x]` Terminé / Fait
- `[/]` En cours / Partiellement fait

---

## 1. Moteurs & PWM (Page 8)
*Objectif : Pilotage variable avec PWM de période < 50µs pour ne pas être audible. Pas de marche arrière.*
- [x] **Code** : Terminée. `pwm.c` configure une fréquence de 38kHz, gère les PWM gauche et droite entre 0 et 100%.
  - *Reste à faire* : RAS.
- [ ] **Câblage & Électronique** (Ampli de puissance, moteurs)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 2. Odométrie & Communication SPI (Page 11)
*Objectif : Mesure de distance parcourue via carte Basys2 FPGA pour lire les codeurs incrémentaux et fournir Vg, Vd, Pg, Pd à 50Hz via SPI.*
- [ ] **Code** : Terminé. `recep_spi.c` utilise le bit-bang SPI sur interruption matérielle (EINT1) pour récupérer les valeurs.
  - *Reste à faire* : c'est le LPC qui doit envoyer l'horloge de synchro, pas le fpga !!!
- [ ] **Câblage & Électronique** (Connexion Basys2 <-> LPC1769)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 3. Asservissement & Trajectoire (Page 8)
*Objectif : Rapprocher le robot du fil via $\omega_{rot}$ et vitesse $V_{moy}$, recalculer vitesses de roues $\omega_g$, $\omega_d$ selon le rayon et écart des roues.*
- [ ] **Code** : À faire. Squelette présent dans `asservissement.c` mais aucune boucle mathématique n'est programmée.
  - *Reste à faire* : Coder le calcul PID/Correcteur pour la ligne, calculer les consignes moteur `pwm_g` et `pwm_d` à chaque cycle 50Hz.
- [ ] **Câblage & Électronique**
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**
  - Redimensionner le gain d'après les mesures de distances max acceptables

## 4. Métrologie Inductive du Fil (Pages 3, 8)
*Objectif : Espionner l'amplitude des signaux alternatifs dans le fil pour l'asservissement et décoder les ordres.*
- [x] **Code** : Terminé. `capteur_inductif.c` gère les ADC, moyenne les capteurs avant/arrière/horizontal à chaque 20ms, et affiche le debug associé.
  - *Reste à faire* : Faire des mesures pour trouver la constante de linéarité entre la mesure et la distance du centre de la bobine au fil
- [ ] **Câblage & Électronique** (Bobines, circuit RLC, redressement, filtrage)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 5. Décodage des Ordres Filaire (Pages 3, 4)
*Objectif : Identifier les enveloppes (E, e, 0, o, 1, i) dans le signal inductif et lire les ordres.*
- [/] **Code** : Partiellement fait. `decode_enveloppe.c` reconnaît les chronogrammes correctement. Les commandes de vitesses et les réglages débug sont implémentés.
  - *Reste à faire* : Interpréter les commandes pour tourner aux jonctions (prendre voie Nord, Sud, déchargement, chargement : codes 001 à 100).
- [ ] **Câblage & Électronique**
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 6. Émission Infrarouge (IR) de Statut (Page 7)
*Objectif : Émettre en permanence 16 bits (ID, Vitesse, Statut, Checksum) à 38kHz. Trame envoyée 3 fois suivie d'un blanc équivalent.*
- [x] **Code** : Terminé. Implémenté fidèlement dans `emission_ir.c`. Les chronogrammes et la modulation 38kHz sont justes.
- [ ] **Câblage & Électronique** (LED IR de forte puissance, transistor d'amplification)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 7. Sécurité Jonction DTMF (Page 6)
*Objectif : Prévenir la collision aux croisements. Le robot écoute une trame DTMF `# [ID] A/D *` (Arrêt/Départ) et clignote une LED si concerné.*
- [x] **Code** : Terminé. Décodeur implémenté et validé sur machine d'état dans `dtmf.c`.
  - *Reste à faire* : Appeler la mise à 0 des moteurs. Peit-être implémenter une MAE pour les états du moteur et du robot ? (`changer_pwm_moteurs(0,0)`) dans l'action d'arrêt et les relancer dans l'action D.
- [ ] **Câblage & Électronique** (Microphone, amplificateur, puce HT9170D vers GPIO)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 8. Balayage Proximètre Infrarouge (Pages 9)
*Objectif : Éviter les piétons avec un capteur GP2Y0A02YK0F monté sur servomoteur. Bips buzzer d'obstacles.*
- [x] **Code** : Terminé. Gestion du servomoteur (4 modes de balayage), acquisition ADC du GP2Y, calcul des distances et gestion du buzzer via `proximetre.c`.
- [ ] **Câblage & Électronique** (Capteur GP2Y, servomoteur, buzzer)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 9. Statuts et Boutons Ouvriers (Pages 2, 10, 11)
*Objectif : Mémorisation de la destination, LED RGB indiquant le statut (Vert/Rouge/Bleu/Magenta), boutons "colis_récupéré" et "colis_chargé".*
- [x] **Code** : Terminé. Interruption boutons dans `buttons.c`, affichage couleurs dans `status.c`.
  - *Reste à faire* : L'ID de destination A,B,C,D est censé être stocké après la lecture de la consigne sur le fil, à s'assurer de bien le sauver et de ne l'oublier qu'au déchargement final.
- [ ] **Câblage & Électronique** (Boutons poussoirs avec anti-rebond, LED RGB)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 10. Affichage Lettre Service - Moteur PàP (Page 11)
*Objectif : Afficher colisA, colisB, colisC, colisD ou vide en tournant un moteur pas-à-pas avec du texte.*
- [ ] **Code** : À faire. Aucun code ne pilote le moteur pas-à-pas. (`led_register.c` sert probablement pour autre chose).
  - *Reste à faire* : Créer un module `stepper.c`, générer la séquence de phase du moteur pour se positionner sur le bon quart de tour.
- [ ] **Câblage & Électronique** (Driver de moteur pas-à-pas, moteur lui-même, roue en carton propre)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 11. Arrêt Coup de Poing & Avertisseur Ouvrier (Page 10)
*Objectif : Arrêt d'urgence qui coupe le circuit mais prévient le logiciel. Beeps de 1KHz pendant 0,75s pour charger/décharger le robot.*
- [ ] **Code** : À faire.
  - *Reste à faire* : Définir une entrée GPIO pour le signal logiciel de l'arrêt d'urgence. Coder l'avertisseur sonore de 0,75s pour appeler l'ouvrier (dans `timers.c` ou `status.c`).
- [ ] **Câblage & Électronique** (Bouton coup de poing avec 2 contacts : 1 coupe circuit puissance, l'autre sur un pin GPIO)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 12. Capteurs Capacitifs ("Fausse Peau") (Page 10)
*Objectif : 4 capteurs (avant, arrière, côtés). Arrêt si devant, ralentissement 5cm/s si côté, accélération 10% si derrière.*
- [ ] **Code** : À faire. Actuellement absent.
  - *Reste à faire* : Compter le nombre d'oscillations astables toutes les 10ms. Déduire l'approche capacitive et modifier les consignes moteurs (Ralentir, Arrêter, Accélérer).
- [ ] **Câblage & Électronique** (Surface métallique, oscillateurs astables avec condensateur de surface)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

## 13. Réception Ultrasonore Poste Ouvrier (Page 6)
*Objectif : Recevoir le signal ultrason du poste pour identifier l'ID du poste, sa distance, et s'il est à gauche ou à droite.*
- [ ] **Code** : À faire. Actuellement absent.
  - *Reste à faire* : Coder une interruption liée à l'enveloppe ultrasons, mesurer la durée des impulsions (200µs ou 300µs = gauche/droite) et l'écart (500µs + ID*200µs).
- [ ] **Câblage & Électronique** (Récepteur ultrasonore, amplification, démodulation d'enveloppe)
- [ ] **Test Simulation**
- [ ] **Test Réel Validé**

---
## 14. Mode Débogage (Optionnel mais requis pour soutenance)
*Objectif : Configuration via DIP switches.*
- [x] **Code** : Validé pour les moteurs, les capteurs inductifs, le proximètre. Envoi via liaison série 50Hz ok.
- [ ] **Câblage & Électronique** (DIP Switches sur toutes les broches listées)
- [X] **Test Simulation**
- [ ] **Test Réel Validé**
