// ==========================================
// INTERFACE DE SUPERVISION - PROCESSING 2.1.2
// ==========================================

// Configuration générale de l'écran [cite: 1]
int winWidth = 1080;
int winHeight = 1920;

// Définition des hauteurs des sous-écrans [cite: 1]
int hSub1 = 700; // Vue du système
int hSub2 = 500; // Gestionnaire de mission [cite: 29]
int hSub3 = 620; // Liste Robots [cite: 26, 30]

// Variables d'état pour les interactions de la colonne "Robot" [cite: 22]
int selectedMissionRow = -1; 

// Échelle et centre pour le dessin du circuit en "8 allongé" [cite: 3, 4]
float cx = 1080 / 2;
float cy = 700 / 2 + 30;
float loopWidth = 320;
float loopHeight = 220;
float gapY = 160; // Écartement pour la forme en Y [cite: 4]

void setup() {
  size(1080, 1920); // Résolution native [cite: 1]
  smooth();
}

void draw() {
  background(245); // Fond clair général
  
  // -------------------------------------------------------------
  // SOUS-ÉCRAN 1 : VUE DU SYSTEME [cite: 2, 3]
  // -------------------------------------------------------------
  drawSubHeader(0, 0, "VUE DU SYSTEME"); //[cite: 2]
  
  // Dessin du Y central orange [cite: 4]
  stroke(243, 156, 18); // Orange [cite: 2, 4]
  strokeWeight(8);
  noFill();
  line(cx, cy + 40, cx, cy - 40); // Base du Y
  line(cx, cy - 40, cx - gapY, cy - 140); // Branche gauche [cite: 4]
  line(cx, cy - 40, cx + gapY, cy - 140); // Branche droite [cite: 4]
  
  // Annotations numériques du Y (Noir) [cite: 4]
  fill(0); textSize(18); textAlign(CENTER, CENTER);
  text("102", cx - gapY - 20, cy - 150); //[cite: 4]
  text("103", cx + gapY + 20, cy - 150); //[cite: 4]
  text("104", cx - 40, cy + 50); //[cite: 4]
  text("105", cx + 40, cy + 50); //[cite: 4]
  
  // Boucles du 8 allongé (Rectangles noirs sans le côté adjacent au Y) [cite: 3, 5]
  stroke(0);
  strokeWeight(4);
  
  // Boucle Gauche [cite: 5]
  float leftBoxX = cx - gapY - loopWidth;
  float leftBoxY = cy - loopHeight/2;
  noFill();
  beginShape();
  vertex(cx - gapY, leftBoxY); 
  vertex(leftBoxX + 15, leftBoxY); // Coins légèrement arrondis [cite: 5]
  quadraticVertex(leftBoxX, leftBoxY, leftBoxX, leftBoxY + 15);
  vertex(leftBoxX, leftBoxY + loopHeight - 15);
  quadraticVertex(leftBoxX, leftBoxY + loopHeight, leftBoxX + 15, leftBoxY + loopHeight);
  vertex(cx - gapY, leftBoxY + loopHeight);
  endShape();
  
  // Boucle Droite [cite: 5]
  float rightBoxX = cx + gapY;
  float rightBoxY = cy - loopHeight/2;
  beginShape();
  vertex(cx + gapY, rightBoxY);
  vertex(rightBoxX + loopWidth - 15, rightBoxY);
  quadraticVertex(rightBoxX + loopWidth, rightBoxY, rightBoxX + loopWidth, rightBoxY + 15);
  vertex(rightBoxX + loopWidth, rightBoxY + loopHeight - 15);
  quadraticVertex(rightBoxX + loopWidth, rightBoxY + loopHeight, rightBoxX + loopWidth - 15, rightBoxY + loopHeight);
  vertex(cx + gapY, rightBoxY + loopHeight);
  endShape();
  
  // Exemple d'affichage d'un Poste (P02) et d'un Robot (R01) [cite: 7, 14]
  float testPosteX = leftBoxX + 100;
  float testPosteY = leftBoxY;
  
  // Ligne de poste perpendiculaire [cite: 6]
  stroke(0); strokeWeight(3);
  line(testPosteX, testPosteY - 15, testPosteX, testPosteY + 15);
  
  // Label du poste (Côté extérieur) [cite: 8]
  fill(0); textSize(14); textAlign(CENTER, BOTTOM);
  text("P02", testPosteX, testPosteY - 20); //[cite: 7, 8]
  
  // Flèche verte avec contour orange (Côté intérieur clignotant pour démo) [cite: 9, 10]
  if (frameCount % 60 < 45) { 
    stroke(243, 156, 18); strokeWeight(2); fill(46, 204, 113); //[cite: 9]
    pushMatrix();
    translate(testPosteX, testPosteY + 25); // Positionnement intérieur [cite: 10]
    beginShape();
    vertex(-5, 0); vertex(5, 0); vertex(5, 10); vertex(10, 10); vertex(0, 20); vertex(-10, 10); vertex(-5, 10);
    endShape(CLOSE);
    popMatrix();
  }
  
  // Dessin d'un exemple de Robot mobile (R01) [cite: 11, 14]
  float robotX = testPosteX + 50;
  float robotY = testPosteY - 12; 
  
  stroke(0); strokeWeight(2); fill(0); // Rectangle noir [cite: 11]
  rect(robotX, robotY, 40, 24); // Longueur proportionnelle [cite: 17]
  
  // Jauge de vitesse interne (ex: 3 barres = 60%, couleur VERT) [cite: 11, 12, 13]
  int barCount = 3; 
  fill(46, 204, 113); noStroke(); //[cite: 13]
  for(int b=0; b<barCount; b++) {
    rect(robotX + 4 + (b * 7), robotY + 5, 5, 14); //[cite: 11]
  }
  
  // Label Orange à côté [cite: 14]
  fill(243, 156, 18); textSize(14); textAlign(LEFT, CENTER); //[cite: 14]
  text("R01", robotX + 45, robotY + 12); //[cite: 14, 15]
  
  // Ligne de séparation noire avec le Sous-Écran 2 [cite: 2]
  stroke(0); strokeWeight(2);
  line(0, hSub1, width, hSub1);
  
  // -------------------------------------------------------------
  // SOUS-ÉCRAN 2 : GESTIONNAIRE DE MISSION [cite: 2, 18]
  // -------------------------------------------------------------
  drawSubHeader(0, hSub1, "GESTIONNAIRE DE MISSION"); //[cite: 2]
  
  int tableX = 50; // Marges appliquées [cite: 27]
  int tableY = hSub1 + 80;
  int tableWidth = winWidth - 100; 
  int rowHeight = 50;
  int[] colWidths = {250, 200, 150, 150, 230}; 
  String[] headers = {"Trajet", "Robot", "ID Livraison", "Status", "Vitesse"}; //[cite: 19, 20, 23, 24]
  
  // Ligne d'en-tête (Gris 25%) [cite: 18]
  fill(64); 
  stroke(0); strokeWeight(4); // Séparations épaisses [cite: 25]
  rect(tableX, tableY, tableWidth, rowHeight);
  
  fill(255); textSize(16); textAlign(CENTER, CENTER);
  int currentX = tableX;
  for(int i=0; i<headers.length; i++) {
    text(headers[i], currentX + colWidths[i]/2, tableY + rowHeight/2);
    currentX += colWidths[i];
  }
  
  // Données de test [cite: 19, 20, 23, 24]
  String[][] missionData = {
    {"P05 --> P01", "07", "A", "L", "60%"},
    {"P03 --> P08", "?", "B", "E", "20%"},
    {"P04 --> P01", "18", "C", "D", "80%"},
    {"P06 --> P12", "?", "B", "C", "40%"},
    {"P13 --> P10", "?", "D", "E", "60%"}
  };
  
  // Dessin des lignes de données [cite: 18, 25]
  for(int r=0; r<missionData.length; r++) {
    int rY = tableY + rowHeight + (r * rowHeight);
    currentX = tableX;
    
    for(int c=0; c<5; c++) {
      stroke(0); 
      strokeWeight(1); // Séparations horizontales intérieures fines [cite: 25]
      fill(255);       // Fond blanc de base
      
      // Configuration de la colonne interactive "Robot" (Index 1) [cite: 20]
      if (c == 1 && missionData[r][c].equals("?")) { //[cite: 20]
        if (selectedMissionRow == r) {
          fill(52, 152, 219, 191); // Fond bleu opaque à 75% si actif [cite: 22]
        }
        rect(currentX, rY, colWidths[c], rowHeight);
        
        // Dessin du contour intérieur bleu requis [cite: 21]
        stroke(52, 152, 219); strokeWeight(2); noFill(); //[cite: 20, 21]
        rect(currentX + 3, rY + 3, colWidths[c] - 6, rowHeight - 6);
        fill(52, 152, 219); // Texte bleu "?" [cite: 20]
      } else {
        // Rendu des autres cellules standards
        rect(currentX, rY, colWidths[c], rowHeight);
        
        // Couleurs de la colonne Status (Index 3) [cite: 23]
        if (c == 3) {
          if (missionData[r][c].equals("L")) fill(46, 204, 113); // Vert [cite: 23]
          else if (missionData[r][c].equals("E")) fill(231, 76, 60); // Rouge [cite: 23]
          else if (missionData[r][c].equals("C")) fill(241, 130, 141); // Fuschia [cite: 23]
          else if (missionData[r][c].equals("D")) fill(52, 152, 219); // Bleu [cite: 23]
        } else {
          fill(0); // Texte noir standard [cite: 23, 24]
        }
      }
      
      // Affichage du texte de la cellule
      textSize(16); textAlign(CENTER, CENTER);
      text(missionData[r][c], currentX + colWidths[c]/2, rY + rowHeight/2);
      
      currentX += colWidths[c];
    }
  }
  
  // Redessiner les contours épais verticaux par-dessus pour respecter le style graphique [cite: 25]
  stroke(0); strokeWeight(4);
  currentX = tableX;
  int totalTableHeight = rowHeight + (missionData.length * rowHeight);
  for(int i=0; i<=headers.length; i++) {
    line(currentX, tableY, currentX, tableY + totalTableHeight);
    if(i < headers.length) currentX += colWidths[i];
  }
  // Lignes horizontales épaisses (En-tête et fermeture basse) [cite: 25]
  line(tableX, tableY, tableX + tableWidth, tableY);
  line(tableX, tableY + rowHeight, tableX + tableWidth, tableY + rowHeight); //[cite: 25]
  line(tableX, tableY + totalTableHeight, tableX + tableWidth, tableY + totalTableHeight);
  
  // Ligne de séparation noire avec le Sous-Écran 3 [cite: 2]
  stroke(0); strokeWeight(2);
  line(0, hSub1 + hSub2, width, hSub1 + hSub2);
  
  // -------------------------------------------------------------
  // SOUS-ÉCRAN 3 : LISTE ROBOTS [cite: 2, 26]
  // -------------------------------------------------------------
  int startSub3Y = hSub1 + hSub2;
  drawSubHeader(0, startSub3Y, "LISTE ROBOTS"); //[cite: 2]
  
  int gridX = 50;
  int gridY = startSub3Y + 80;
  int cellW = 100;
  int cellH = 50;
  int gridCols = 9; // Grille optimisée pour loger les robots [cite: 26]
  
  for (int i = 1; i <= 36; i++) { // Rendu d'une série de test [cite: 26, 31]
    int index = i - 1;
    int colIdx = index % gridCols;
    int rowIdx = index / gridCols;
    
    int cx_cell = gridX + (colIdx * (cellW + 10));
    int cy_cell = gridY + (rowIdx * (cellH + 10));
    
    // Définition de la couleur de fond opaque à 50% [cite: 26]
    if (i % 3 == 0) {
      fill(231, 76, 60, 128); // Rouge à 50% d'alpha [cite: 26]
    } else {
      fill(46, 204, 113, 128); // Vert à 50% d'alpha [cite: 26]
    }
    
    stroke(180); strokeWeight(1);
    rect(cx_cell, cy_cell, cellW, cellH, 4);
    
    // Affichage de l'identifiant du robot (Noir) [cite: 26]
    fill(0); textSize(14); textAlign(CENTER, CENTER);
    text("R" + nf(i, 2), cx_cell + cellW/2, cy_cell + cellH/2); //[cite: 26]
  }
}

// Fonction utilitaire pour générer l'onglet supérieur de chaque écran [cite: 2]
void drawSubHeader(int x, int y, String title) {
  pushStyle();
  fill(128); // Rectangle gris 50% [cite: 2]
  stroke(243, 156, 18); // Contour orange [cite: 2]
  strokeWeight(3);
  
  // Tracé de l'onglet biseauté propre à l'interface [cite: 2]
  beginShape();
  vertex(x + 50, y + 10);
  vertex(x + 350, y + 10);
  vertex(x + 320, y + 50);
  vertex(x + 50, y + 50);
  endShape(CLOSE);
  
  // Texte Blanc appliqué [cite: 2]
  fill(255); textSize(16); textAlign(LEFT, CENTER);
  text(title, x + 70, y + 30); //[cite: 2]
  popStyle();
}

// Gestion des interactions de clic souris [cite: 22]
void mousePressed() {
  int tableX = 50;
  int tableY = hSub1 + 80;
  int rowHeight = 50;
  int robotColLeft = tableX + 250; // Début de la colonne 2 (Robot) [cite: 19, 20]
  int robotColRight = robotColLeft + 200; //[cite: 20]
  
  if (mouseX >= robotColLeft && mouseX <= robotColRight) { //[cite: 20]
    for (int r = 0; r < 5; r++) { 
      int rY = tableY + rowHeight + (r * rowHeight);
      if (mouseY >= rY && mouseY <= rY + rowHeight) {
        // Toggle l'état sélectionné au clic sur le point d'interrogation [cite: 20, 22]
        if (selectedMissionRow == r) {
          selectedMissionRow = -1; 
        } else {
          selectedMissionRow = r; //[cite: 22]
        }
      }
    }
  }
}
