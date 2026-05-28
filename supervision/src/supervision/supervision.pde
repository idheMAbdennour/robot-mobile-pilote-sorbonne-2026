import java.io.*;
import processing.serial.*;

// ==========================================
// VARIABLES GLOBALES (structure_donnees.txt)
// ==========================================
int nb_robot;
int nb_poste_nord;
int nb_poste_sud;
int nb_poste;

int numero_mission_select = -1;
int num_robot_select = -1;
int id_last_poste_nord;
int id_last_poste_sud;
int taille_tab_mission;
int nb_mission_actuelle = 0;

// Communication & Listes
Serial myPort;
StringList fifo_messages; // [cite: 30] Max 100 messages [cite: 31]
Robot[] listeRobot; // [cite: 45]
Poste[] listePoste; // [cite: 47]
ArrayList<Mission> listeMission = new ArrayList<Mission>(); // Tableau dynamique [cite: 49]

// Gestion de l'UI et des interactions
int currentHoverRow = -1;
int selectedAssignRow = -1; // Pour savoir quelle case attribution est cliquée [cite: 85]
boolean errorBlink = false;
int errorBlinkCount = 0;
int errorBlinkTimer = 0;

// Dimensions de l'écran global [cite: 63]
int winW = 1080;
int winH = 1920;

// Zones des sous-écrans
float hScreen1, hScreen2, hScreen3;
float yScreen1, yScreen2, yScreen3;

// Déroulant vitesse (Routine 2)
boolean speedDropdownOpen = false;
int dropdownRow = -1;
String[] speedItems = {"20%", "40%", "60%", "80%"};

// ==========================================
// INITIALISATION (initialisation.txt)
// ==========================================
void setup() {
  size(1080, 1920); // Spécification de l'écran général [cite: 63]
  frameRate(30);
  
  fifo_messages = new StringList(); // [cite: 58]
  
  // Routine d'initialisation 1 (Logique) [cite: 55]
  chargerConfiguration();
  
  // Initialisation des ports séries (UART via USB) [cite: 11]
  println(Serial.list());
  try {
      if (Serial.list().length > 0) {
        String portName = Serial.list()[0];
        myPort = new Serial(this, portName, 9600);
        serialAvailable = true;
      }
    } catch (Exception e) {
      println("Erreur d'ouverture du port série.");
    }
  
  // Calcul géométrique des sous-écrans [cite: 93, 94, 95, 96, 97]
  yScreen1 = 0;
  hScreen1 = winH * 0.40; // 40% pour la Vue du système
  
  // La hauteur de l'écran 3 dépend du nombre de robots (Grille) [cite: 94]
  int cols = 3;
  int rows = ceil((float)nb_robot / cols);
  hScreen3 = 60 + (rows * 45) + 30; // Dynamique selon le nombre de robots [cite: 94]
  yScreen3 = winH - hScreen3;
  
  yScreen2 = hScreen1;
  hScreen2 = yScreen3 - yScreen2; // L'espace restant au milieu [cite: 97]
  
  // Calcul du nombre de lignes possibles pour le tableau des missions [cite: 61, 62]
  taille_tab_mission = floor((hScreen2 - 80) / 40); // 80px d'entête, 40px par ligne
}

void draw() {
  background(255);
  
  // Traitement automatique de la FIFO série [cite: 11]
  parseNextMessage();
  
  // Vérification du timeout des robots (2 minutes) [cite: 10]
  verifierTimeoutRobots();
  
  // Dessin des 3 sous-écrans
  dessinerSousEcran1();
  dessinerSousEcran2();
  dessinerSousEcran3();
  
  // Dessin des séparations physiques [cite: 64]
  stroke(0);
  strokeWeight(3);
  line(0, yScreen2, width, yScreen2);
  line(0, yScreen3, width, yScreen3);
}

// ==========================================
// PARSING & LOGIQUE DE CONFIGURATION
// ==========================================
void chargerConfiguration() {
  // Chargement depuis le chemin spécifié (ou fallback local data/) [cite: 55]
  String path = "config.txt"; 
  File f = new File("/data/idhem/Documents/Sorbonne/L3/robot-mobile-pilote-sorbonne-2026/supervision/bin/config.txt");
  if (f.exists()) {
    path = f.getAbsolutePath();
  }
  
  String[] lines = loadStrings(path);
  if (lines != null && lines.length >= 3) {
    nb_robot = int(trim(lines[0])); // [cite: 55]
    nb_poste_nord = int(trim(lines[1])); // [cite: 55, 56]
    nb_poste_sud = int(trim(lines[2])); // [cite: 56]
  } else {
    // Valeurs par défaut si le fichier est absent
    nb_robot = 21;
    nb_poste_nord = 10;
    nb_poste_sud = 10;
  }
  
  nb_poste = nb_poste_nord + nb_poste_sud; // [cite: 56]
  id_last_poste_sud = nb_poste_sud; // [cite: 58]
  id_last_poste_nord = nb_poste; // [cite: 58]
  
  // Instanciation des tableaux [cite: 56, 57]
  listeRobot = new Robot[nb_robot + 1]; 
  for (int i = 1; i <= nb_robot; i++) {
    listeRobot[i] = new Robot(i); // [cite: 56]
  }
  
  listePoste = new Poste[nb_poste + 1];
  for (int i = 1; i <= nb_poste; i++) {
    listePoste[i] = new Poste(i); // [cite: 57]
  }
}

// Extraction asynchrone des messages UART [cite: 11]
void serialEvent(Serial p) {
  String incoming = p.readStringUntil('\n');
  if (incoming != null) {
    incoming = trim(incoming);
    if (fifo_messages.size() < 100) { // Limite de 100 messages [cite: 31]
      fifo_messages.append(incoming); // [cite: 11]
    }
  }
}

void parseNextMessage() {
  if (fifo_messages.size() == 0) return;
  
  String msg = fifo_messages.get(0);
  fifo_messages.remove(0);
  
  String[] tokens = split(msg, ',');
  if (tokens.length == 0) return;
  
  int type = int(tokens[0]); // [cite: 12]
  
  if (type == 1 && tokens.length >= 5) { // Type 1 [cite: 12]
    int numero_poste = int(tokens[1]);
    int numero_robot = int(tokens[2]);
    int new_vitesse = int(tokens[3]);
    String new_status = tokens[4];
    
    if (numero_robot >= 1 && numero_robot <= nb_robot) {
      Robot r = listeRobot[numero_robot];
      r.id_last_poste = numero_poste; // [cite: 12]
      r.vitesse = new_vitesse; // [cite: 13]
      r.lastSeenMessage = millis(); // Refresh watchdog [cite: 10]
      
      // Règles de transition de l'état "garé" [cite: 13, 14]
      // Recherche si le robot appartient à une mission
      for (Mission m : listeMission) {
        if (m.id_livreur == numero_robot) {
          if (r.status == 'E' && r.id_last_poste == m.p_depart) r.gare = true; // [cite: 13]
          if (r.status == 'R' && r.id_last_poste == m.p_arrive) r.gare = true; // [cite: 13]
        }
      }
      
      if (r.status == 'E' && new_status.equals("C")) r.gare = false; // [cite: 13]
      if (r.status == 'R' && new_status.equals("L")) {
        r.gare = false; // [cite: 13, 14]
        r.dispo = true; // [cite: 14]
      }
      
      r.status = new_status.charAt(0); // [cite: 14]
      r.triggerArrowAnim(); // Activer l'affichage temporaire de la flèche [cite: 4, 5]
    }
  } 
  else if (type == 2 && tokens.length >= 4) { // Type 2 [cite: 12]
    int numero_poste_expediteur = int(tokens[1]);
    int code_livreur = int(tokens[2].charAt(0));
    int numero_poste_destinataire = int(tokens[3]);
    
    Mission m = new Mission(); // [cite: 15]
    m.p_depart = numero_poste_expediteur; // [cite: 15]
    m.p_arrive = numero_poste_destinataire; // [cite: 16]
    m.code_livreur = (char)code_livreur; // [cite: 16]
    m.id_livreur = -1; // Représente le point d'interrogation / NULL [cite: 16]
    listeMission.add(m); // [cite: 49]
  }
}

void verifierTimeoutRobots() {
  for (int i = 1; i <= nb_robot; i++) {
    if (millis() - listeRobot[i].lastSeenMessage > 120000) { // 2 minutes = 120000 ms [cite: 10]
      listeRobot[i].visibleOnCircuit = false; // Disparaît du circuit [cite: 10]
      listeRobot[i].dispo = false; // Sa case devient rouge [cite: 10]
    } else {
      listeRobot[i].visibleOnCircuit = true;
    }
  }
}

// ==========================================
// RENDU DU SOUS-ÉCRAN 1 : VUE DU SYSTEME [cite: 65]
// ==========================================
void dessinerSousEcran1() {
  pushMatrix();
  translate(0, yScreen1);
  
  // Fond et Onglet [cite: 64]
  dessinerOnglet("VUE DU SYSTEME", 10);
  
  // Dessin du circuit stylisé (Le 8 allongé) [cite: 65, 66]
  stroke(0);
  strokeWeight(2);
  noFill();
  
  // Les boucles en [ et ] [cite: 66, 73]
  rect(80, 100, 320, 240, 20); // Boucle Gauche (Sud) [cite: 66, 67]
  rect(680, 100, 320, 240, 20); // Boucle Droite (Nord) [cite: 66, 67]
  
  // Forme centrale en Y (Orange) [cite: 66]
  stroke(243, 156, 18);
  strokeWeight(8);
  line(540, 240, 540, 340); // Base du Y
  line(540, 240, 440, 100); // Branche Gauche du Y
  line(540, 240, 640, 100); // Branche Droite du Y
  
  // Dessin des Postes et des Flèches [cite: 68, 70]
  stroke(0);
  strokeWeight(2);
  
  // Branche Sud (Gauche) [cite: 58, 59]
  for (int i = 1; i <= nb_poste_sud; i++) {
    float mapX = map(i, 1, nb_poste_sud, 100, 380);
    // Postes sur la ligne supérieure de la boucle
    line(mapX, 85, mapX, 115); // Trait du poste [cite: 68]
    
    // Rendu des labels PNN [cite: 68]
    fill(0);
    textSize(12);
    textAlign(CENTER, BOTTOM);
    text("P" + nf(i, 2), mapX, 80); // [cite: 68, 69]
    
    // Flèches d'orientation (Routines d'actualisation) [cite: 4, 70]
    dessinerFlechePoste(mapX, 130, i); 
  }
  
  // Branche Nord (Droite) [cite: 58, 59]
  for (int i = 1; i <= nb_poste_nord; i++) {
    int idPoste = nb_poste_sud + i;
    float mapX = map(i, 1, nb_poste_nord, 700, 980);
    line(mapX, 85, mapX, 115);
    
    fill(0);
    textSize(12);
    textAlign(CENTER, BOTTOM);
    text("P" + nf(idPoste, 2), mapX, 80);
    
    dessinerFlechePoste(mapX, 130, idPoste);
  }
  
  // Positionnement et Rendu des Robots [cite: 73]
  for (int i = 1; i <= nb_robot; i++) {
    Robot r = listeRobot[i];
    if (!r.visibleOnCircuit) continue; // [cite: 10]
    
    // Calcul de la position graphique selon les règles métiers [cite: 2, 3, 98]
    float rx = 200, ry = 200;
    boolean horizontal = false;
    
    if (r.id_last_poste == id_last_poste_nord || r.id_last_poste == id_last_poste_sud) {
      // Aligné le long du Y [cite: 2, 98]
      rx = 540;
      ry = (r.id_last_poste == id_last_poste_nord) ? 170 : 290;
      horizontal = true; // [cite: 79]
    } else {
      // Position sur les branches standard [cite: 3]
      if (r.id_last_poste <= nb_poste_sud) {
        rx = map(r.id_last_poste, 1, nb_poste_sud, 100, 380) + 15;
        ry = r.gare ? 145 : 100; // Décalé vers le bas si garé [cite: 3, 79]
      } else {
        int idx = r.id_last_poste - nb_poste_sud;
        rx = map(idx, 1, nb_poste_nord, 700, 980) + 15;
        ry = r.gare ? 145 : 100; // [cite: 3, 79]
      }
    }
    
    // Dessin du rectangle du robot [cite: 73]
    pushMatrix();
    translate(rx, ry);
    if (horizontal) rotate(HALF_PI);
    
    stroke(0);
    strokeWeight(1.5);
    fill(255);
    rectMode(CENTER);
    rect(0, 0, 45, 25); // Dimensions proportionnelles [cite: 80]
    
    // Label Orange RNN [cite: 77]
    fill(243, 156, 18);
    textSize(11);
    textAlign(CENTER, CENTER);
    text("R" + nf(r.id_robot, 2), 0, -20); // [cite: 77, 78]
    
    // Jauge de vitesse & Statut interne [cite: 74]
    int bars = r.getNbBars(); // [cite: 100, 101]
    color cStatus = r.getStatusColor(); // [cite: 3, 76, 99]
    
    fill(cStatus);
    noStroke();
    for (int b = 0; b < bars; b++) {
      rect(-16 + (b * 7), 0, 5, 15); // [cite: 74, 75]
    }
    popMatrix();
    rectMode(CORNER);
  }
  
  popMatrix();
}

void dessinerFlechePoste(float x, float y, int idPoste) {
  // Déterminer si la flèche doit s'effacer (Animation 1s) [cite: 5, 102]
  // Pour la simulation, on la laisse visible si une mise à jour est active
  pushMatrix();
  translate(x, y);
  
  stroke(0);
  fill(46, 204, 113); // Vert [cite: 60, 70]
  strokeWeight(1);
  
  // Déviation angulaire selon le statut 'gare' [cite: 4, 71, 72, 102]
  // On simule une flèche par défaut pointant le circuit
  beginShape();
  vertex(0, -10);
  vertex(5, -5);
  vertex(2, -5);
  vertex(2, 10);
  vertex(-2, 10);
  vertex(-2, -5);
  vertex(-5, -5);
  endShape(CLOSE);
  
  popMatrix();
}

// ==========================================
// RENDU DU SOUS-ÉCRAN 2 : GESTIONNAIRE DE MISSION [cite: 81]
// ==========================================
void dessinerSousEcran2() {
  pushMatrix();
  translate(0, yScreen2);
  
  // Fond et Onglet [cite: 64]
  dessinerOnglet("GESTIONNAIRE DE MISSION", 10);
  
  // Configuration des coordonnées de la table [cite: 88]
  float tableX = 40;
  float tableY = 70;
  float tableW = width - 80;
  
  // Dessin de l'en-tête du tableau [cite: 81]
  fill(190); // Gris 25% [cite: 81]
  stroke(0);
  strokeWeight(3); // Séparations épaisses [cite: 88]
  rect(tableX, tableY, tableW, 40);
  
  fill(0);
  textSize(14);
  textAlign(CENTER, CENTER);
  text("Trajet", tableX + 110, tableY + 20); // [cite: 82]
  text("Robot", tableX + 310, tableY + 20); // [cite: 83]
  text("Code Livreur", tableX + 510, tableY + 20); // [cite: 86]
  text("Status", tableX + 690, tableY + 20); // [cite: 86]
  text("Vitesse", tableX + 870, tableY + 20); // [cite: 87]
  
  // Rendu dynamique des lignes de mission [cite: 81, 89]
  currentHoverRow = -1;
  for (int i = 0; i < taille_tab_mission; i++) {
    float rowY = tableY + 40 + (i * 40);
    
    // Séparateurs fins entre les lignes [cite: 88]
    stroke(0);
    strokeWeight(1);
    fill(255);
    rect(tableX, rowY, tableW, 40);
    
    // Lignes verticales internes (épaisses selon cahier des charges) [cite: 88]
    strokeWeight(3);
    line(tableX + 220, rowY, tableX + 220, rowY + 40);
    line(tableX + 400, rowY, tableX + 400, rowY + 40);
    line(tableX + 600, rowY, tableX + 600, rowY + 40);
    line(tableX + 780, rowY, tableX + 780, rowY + 40);
    
    if (i < listeMission.size()) {
      Mission m = listeMission.get(i);
      fill(0);
      textSize(14);
      textAlign(CENTER, CENTER);
      
      // 1. Colonne Trajet [cite: 82]
      text("P" + nf(m.p_depart, 2) + " --> P" + nf(m.p_arrive, 2), tableX + 110, rowY + 20);
      
      // 2. Colonne Robot (Bouton d'attribution interactif) [cite: 83, 108]
      if (m.id_livreur == -1) {
        // Mode attribution en cours / Point d'interrogation [cite: 83]
        if (selectedAssignRow == i) {
          fill(52, 152, 219, 191); // Fond bleu opaque 75% si sélectionné [cite: 85]
          rect(tableX + 223, rowY + 3, 174, 34);
        }
        
        // Gestion du clignotement d'erreur [cite: 9, 106, 114]
        if (errorBlink && numero_mission_select == i) {
          if ((millis() / 200) % 2 == 0) {
            fill(231, 76, 60); // Clignotement rouge [cite: 9, 106]
            rect(tableX + 223, rowY + 3, 174, 34);
          }
        }
        
        stroke(52, 152, 219);
        strokeWeight(3); // Contour gras et bleu [cite: 84]
        noFill();
        rect(tableX + 225, rowY + 5, 170, 30);
        
        fill(52, 152, 219);
        textSize(16);
        text("?", tableX + 310, rowY + 20); // Point d'interrogation gras bleu [cite: 83]
      } else {
        fill(0);
        text(nf(m.id_livreur, 2), tableX + 310, rowY + 20); // Numéro assigné [cite: 83]
      }
      
      // 3. Colonne Code Livreur [cite: 86]
      fill(0);
      text(String.valueOf(m.code_livreur), tableX + 510, rowY + 20);
      
      // 4. Colonne Status [cite: 86]
      if (m.id_livreur != -1) {
        char st = listeRobot[m.id_livreur].status;
        displayStatusChar(st, tableX + 690, rowY + 20); // [cite: 86]
      } else {
        fill(0);
        text("?", tableX + 690, rowY + 20); // [cite: 87]
      }
      
      // 5. Colonne Vitesse (Menu déroulant si cliqué) [cite: 87, 109]
      if (m.id_livreur != -1) {
        int speedVal = listeRobot[m.id_livreur].vitesse;
        fill(0);
        // Conversion de la valeur discrète en libellé textuel pour l'affichage courant
        String speedTxt = (speedVal >= 13) ? "80%" : (speedVal >= 10) ? "60%" : (speedVal >= 7) ? "40%" : "20%";
        text(speedTxt, tableX + 870, rowY + 20);
      } else {
        fill(0);
        text("?", tableX + 870, rowY + 20); // [cite: 87]
      }
    }
  }
  
  // Rendu de l'overlay pour le dropdown Vitesse ouvert (Routine d'interaction 2) [cite: 116]
  if (speedDropdownOpen && dropdownRow != -1) {
    float dropY = tableY + 40 + (dropdownRow * 40) + 40;
    fill(255);
    stroke(0);
    strokeWeight(2);
    rect(tableX + 780, dropY, 180, 160);
    for (int k = 0; k < 4; k++) {
      fill(0);
      text(speedItems[k], tableX + 870, dropY + 20 + (k * 40)); // [cite: 87, 117]
    }
  }
  
  // Animation Timer d'erreur [cite: 9, 106]
  if (errorBlink && millis() - errorBlinkTimer > 1200) {
    errorBlink = false; // Arrêt après 3 cycles brefs [cite: 9, 106]
  }
  
  popMatrix();
}

void displayStatusChar(char st, float x, float y) {
  // Attribution des couleurs réglementaires par statut [cite: 86]
  if (st == 'L') fill(46, 204, 113);      // Vert [cite: 86]
  else if (st == 'E') fill(231, 76, 60); // Rouge [cite: 86]
  else if (st == 'C') fill(243, 156, 18); // Fuschia (Simulé Orange-Fuschia) [cite: 86]
  else if (st == 'D') fill(52, 152, 219);  // Bleu [cite: 86]
  else fill(0);
  
  text(String.valueOf(st), x, y);
}

// ==========================================
// RENDU DU SOUS-ÉCRAN 3 : LISTE ROBOTS [cite: 89]
// ==========================================
void dessinerSousEcran3() {
  pushMatrix();
  translate(0, yScreen3);
  
  // Fond et Onglet [cite: 64]
  dessinerOnglet("LISTE ROBOTS", 10);
  
  // Dessin de la matrice quadrillée [cite: 52, 60]
  int cols = 3;
  float cellW = (width - 80) / (float)cols;
  float cellH = 45;
  
  for (int i = 0; i < nb_robot; i++) {
    int idRobot = i + 1;
    int col = i % cols;
    int row = i / cols;
    
    float cx = 40 + (col * cellW);
    float cy = 60 + (row * cellH);
    
    Robot r = listeRobot[idRobot];
    
    // Définition de la couleur de fond opaque à 50% [cite: 90]
    if (r.dispo) {
      fill(46, 204, 113, 127); // Vert émeraude transparent [cite: 7, 90, 104]
    } else {
      fill(231, 76, 60, 127);  // Rouge alerte transparent [cite: 7, 90, 104]
    }
    
    stroke(255);
    strokeWeight(2);
    rect(cx, cy, cellW, cellH);
    
    // Affichage des chaînes normalisées [cite: 7, 89, 90, 104]
    fill(0);
    textSize(14);
    textAlign(CENTER, CENTER);
    if (r.dispo) {
      text("R" + nf(idRobot, 2), cx + cellW/2, cy + cellH/2); // [cite: 7, 89, 104]
    } else {
      // Si non disponible mais conservé au timeout [cite: 10, 107]
      if (!r.visibleOnCircuit) {
        text("R" + nf(idRobot, 2) + " (Timeout)", cx + cellW/2, cy + cellH/2); // [cite: 10, 107]
      } else {
        text("----", cx + cellW/2, cy + cellH/2); // Remplacement textuel requis [cite: 7, 89, 104]
      }
    }
  }
  
  // Case d'équilibrage si le nombre total est impair [cite: 61]
  if (nb_robot % cols != 0) {
    int emptyCol = cols - 1;
    int emptyRow = ceil((float)nb_robot / cols) - 1;
    fill(235);
    rect(40 + (emptyCol * cellW), 60 + (emptyRow * cellH), cellW, cellH); // [cite: 61]
  }
  
  popMatrix();
}

// ==========================================
// UTILS GRAPHISTE (Composants visuels génériques)
// ==========================================
void dessinerOnglet(String title, float xOffset) {
  noStroke();
  fill(128); // Rectangle gris 50% [cite: 64]
  rect(xOffset, 0, 260, 35, 5, 5, 0, 0);
  
  stroke(243, 156, 18); // Contour orange [cite: 64]
  strokeWeight(2);
  noFill();
  rect(xOffset, 0, 260, 35, 5, 5, 0, 0);
  
  fill(255); // Texte blanc [cite: 64]
  textSize(12);
  textAlign(CENTER, CENTER);
  text(title, xOffset + 130, 17);
}

// ==========================================
// ENGIN D'INTERACTION SOURIS (interaction.txt)
// ==========================================
void mousePressed() {
  
  // ----------------------------------------
  // Zone d'interaction du Sous-Écran 2 [cite: 108]
  // ----------------------------------------
  if (mouseY > yScreen2 && mouseY < yScreen3) {
    float relY = mouseY - yScreen2;
    float tableY = 70;
    
    // Détection d'un clic sur la colonne Vitesse (Dropdown) [cite: 109, 116]
    if (mouseX > 40 + 780 && mouseX < 40 + 960) {
      int clickedRow = floor((relY - tableY - 40) / 40);
      if (clickedRow >= 0 && clickedRow < listeMission.size()) {
        Mission m = listeMission.get(clickedRow);
        if (m.id_livreur != -1) { // Une vitesse n'est modifiable que si un robot est affecté [cite: 109]
          speedDropdownOpen = true;
          dropdownRow = clickedRow;
          numero_mission_select = clickedRow; // [cite: 116]
          num_robot_select = m.id_livreur; // [cite: 117]
          return;
        }
      }
    }
    
    // Fermeture ou traitement du dropdown vitesse [cite: 116]
    if (speedDropdownOpen) {
      float dropY = tableY + 40 + (dropdownRow * 40) + 40;
      if (mouseX > 40 + 780 && mouseX < 40 + 960 && relY > dropY && relY < dropY + 160) {
        int itemIdx = floor((relY - dropY) / 40);
        if (itemIdx >= 0 && itemIdx < 4) {
          // Injection de la vitesse logique simulée [cite: 117]
          int targetSpeed = (itemIdx == 0) ? 3 : (itemIdx == 1) ? 6 : (itemIdx == 2) ? 9 : 13;
          listeRobot[num_robot_select].vitesse = targetSpeed; // [cite: 117]
          
          // Déclenchement de la routine UART Sortie 2 [cite: 17, 117]
          envoyerMessageType4(num_robot_select, targetSpeed); 
        }
      }
      speedDropdownOpen = false;
      dropdownRow = -1;
      return;
    }
    
    // Détection d'un clic sur la colonne d'attribution (Robot "?") [cite: 108, 111]
    if (mouseX > 40 + 220 && mouseX < 40 + 400) {
      int clickedRow = floor((relY - tableY - 40) / 40);
      if (clickedRow >= 0 && clickedRow < listeMission.size()) {
        if (listeMission.get(clickedRow).id_livreur == -1) { // [cite: 109]
          selectedAssignRow = clickedRow;
          numero_mission_select = clickedRow; // [cite: 111]
        }
      }
      return;
    }
  }
  
  // ----------------------------------------
  // Zone d'interaction du Sous-Écran 3 (Grille Robot) [cite: 110, 112]
  // ----------------------------------------
  if (mouseY > yScreen3) {
    float relY = mouseY - yScreen3;
    int cols = 3;
    float cellW = (width - 80) / (float)cols;
    float cellH = 45;
    
    int col = floor((mouseX - 40) / cellW);
    int row = floor((relY - 60) / cellH);
    
    if (col >= 0 && col < cols && row >= 0) {
      int clickedRobotId = (row * cols) + col + 1;
      if (clickedRobotId <= nb_robot && selectedAssignRow != -1) { // [cite: 112]
        Robot r = listeRobot[clickedRobotId];
        
        if (r.dispo) { // [cite: 112]
          r.dispo = false; // Passe indisponible [cite: 112]
          listeMission.get(selectedAssignRow).id_livreur = r.id_robot; // [cite: 113]
          
          // Exécuter Routine d'envoi de message 1 [cite: 17, 113]
          envoyerMessageType3(selectedAssignRow);
          
          // Reset sélection
          selectedAssignRow = -1;
        } else {
          // Erreur : Robot indisponible -> Flash rouge [cite: 9, 106, 114]
          errorBlink = true;
          errorBlinkTimer = millis();
          selectedAssignRow = -1;
        }
      }
    }
    return;
  }
  
  // Clic à côté : désélection complète [cite: 116]
  selectedAssignRow = -1;
  speedDropdownOpen = false;
}

// ==========================================
// ROUTINES D'ENVOI UART USB (gestion_comm_out.txt)
// ==========================================
void envoyerMessageType3(int mIdx) {
  Mission m = listeMission.get(mIdx);
  // Construction de la trame [cite: 17]
  String frame = "R" + m.id_livreur + "P" + m.p_depart + "P" + m.p_arrive + "\n\r"; // [cite: 17, 18, 19]
  
  if (myPort != null) {
    myPort.write(frame); // [cite: 19]
  }
  println("UART OUT (Type 3) : " + trim(frame));
}

void envoyerMessageType4(int robotId, int speed) {
  // Construction de la trame [cite: 19]
  String frame = "R" + robotId + "V" + speed + "\n\r"; // [cite: 19, 20]
  
  if (myPort != null) {
    myPort.write(frame); // [cite: 20]
  }
  println("UART OUT (Type 4) : " + trim(frame));
}

// ==========================================
// DÉFINITIONS DES CLASSES OBJETS (structure_donnees.txt)
// ==========================================
class Robot {
  int id_robot; // [cite: 33]
  int id_last_poste; // [cite: 34]
  int vitesse; // [cite: 35]
  char status; // [cite: 35]
  boolean gare; // [cite: 36]
  boolean dispo; // [cite: 37]
  
  // Données de contrôle internes
  long lastSeenMessage;
  boolean visibleOnCircuit;
  long arrowAnimTimer;
  
  Robot(int id) {
    this.id_robot = id; // [cite: 33]
    this.id_last_poste = 1; // [cite: 34]
    this.vitesse = 0; // [cite: 35]
    this.status = 'L'; // Libre par défaut [cite: 35]
    this.gare = false; // [cite: 36]
    this.dispo = true; // Disponible au démarrage [cite: 37]
    this.lastSeenMessage = millis();
    this.visibleOnCircuit = true;
  }
  
  // Cartographie discrète vitesse -> barres de jauge [cite: 99, 100, 101]
  int getNbBars() {
    if (vitesse == 0) return 0; // [cite: 100]
    if (vitesse >= 1 && vitesse <= 3) return 1; // [cite: 100]
    if (vitesse >= 4 && vitesse <= 6) return 2; // [cite: 100]
    if (vitesse >= 7 && vitesse <= 9) return 3; // [cite: 101]
    if (vitesse >= 10 && vitesse <= 12) return 4; // [cite: 101]
    if (vitesse >= 13) return 5; // [cite: 101]
    return 0;
  }
  
  color getStatusColor() {
    if (status == 'L') return color(46, 204, 113); // VERT [cite: 76]
    if (status == 'E') return color(231, 76, 60);  // ROUGE [cite: 76]
    if (status == 'C') return color(155, 89, 182); // FUSCHIA (Violet-Fuschia) [cite: 76]
    if (status == 'D') return color(52, 152, 219);  // BLEU [cite: 76]
    return color(128);
  }
  
  void triggerArrowAnim() {
    this.arrowAnimTimer = millis();
  }
}

class Poste {
  int id_poste; // [cite: 37]
  boolean robot_vu; // [cite: 38]
  boolean robot_gare; // [cite: 39]
  
  Poste(int id) {
    this.id_poste = id; // [cite: 37]
    this.robot_vu = false; // [cite: 38]
    this.robot_gare = false; // [cite: 39]
  }
}

class Mission {
  int p_depart; // [cite: 40]
  int p_arrive; // [cite: 41]
  int id_livreur; // [cite: 42]
  char code_livreur; // [cite: 43]
}
