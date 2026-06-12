// ================================================================
//  SIMULATEUR DE CENTRALE UART (LPCXpresso1769) — simulation.pde
//  Entièrement compatible avec supervision.pde (Processing 2.1.2)
// ================================================================

import processing.serial.*;

// Instance série dédiée à l'émission de simulation
Serial simSerialPort = null; 
boolean isSimSerialActive = false;

// Variables de configuration des messages de simulation
int   simRobotId  = 1;
int   simPosteId  = 1;
int   simVitesse  = 5;
char  simStatus   = 'L'; // L, E, C, D
char  simCodeLiv  = 'A'; // A, B, C, D
int   simPosteDest= 2;

// Éléments d'état du message sélectionné (0 = Type 1, 1 = Type 2)
int selectedMsgType = 0; 

// Initialisation globale appelée à la suite du setup principal
void initSimulator() {
  // Optionnel : Configurer ici un port de loopback distinct si nécessaire
  // Par défaut, nous tentons d'utiliser le port configuré dans l'application principale
  if (serialPort != null) {
    simSerialPort = serialPort;
    isSimSerialActive = true;
  }
  
  String[] args = {"Simulateur de Centrale LPCXpresso1769"};
  SimulatorApplet simWindow = new SimulatorApplet();
  PApplet.runSketch(args, simWindow);
}

/**
 * Génère et envoie la trame sélectionnée sur le bus série
 */
void sendSimulatedMessage() {
  String frame = "";
  
  if (selectedMsgType == 0) {
    // Type 1 : Mise à jour Robot -> "P[pp]R[rr]V[vv]S[s]\n"
    frame = "P" + nf(simPosteId, 2) 
          + "R" + nf(simRobotId, 2) 
          + "V" + hex(simVitesse, 2) 
          + "S" + simStatus 
          + "\n";
  } else {
    // Type 2 : Nouvelle Mission -> "P[pp]C[c]P[qq]\n"
    frame = "P" + nf(simPosteId, 2) 
          + "C" + simCodeLiv 
          + "P" + nf(simPosteDest, 2) 
          + "\n";
  }
  
  // Émission physique sur le port série
  if (simSerialPort != null) {
    simSerialPort.write(frame);
    // Interception par le terminal d'affichage s'il est présent
    logIncomingUART(frame.trim()); 
    println("[SIMULATEUR] Trame émise : " + frame.trim());
  } else {
    // Mode dégradé sans liaison matérielle : injection directe dans la FIFO logique
    fifo_messages.append(frame.trim());
    logIncomingUART(frame.trim());
    println("[SIMULATEUR (FIFO Directe)] Trame injectée : " + frame.trim());
  }
}

// ── EXTENSION DU PROGRAMME POUR LA TROISIÈME FENÊTRE ──────────────
public class SimulatorApplet extends PApplet {
  PFont fontUI;
  
  public void setup() {
    size(550, 500);
    frameRate(20);
    fontUI = createFont("Arial", 13, true);
  }
  
  public void draw() {
    background(35, 38, 45);
    textFont(fontUI);
    
    // --- BANNER DE TITRE ---
    fill(28, 30, 35);
    noStroke();
    rect(0, 0, width, 40);
    fill(255, 165, 0); // Accentuation orange réglementaire
    textSize(14);
    textAlign(LEFT, CENTER);
    text("  PANNEL DE SIMULATION CENTRALE LOGIQUE", 10, 20);
    
    // --- SÉLECTION TYPE DE MESSAGE ---
    textSize(13);
    fill(200);
    textAlign(LEFT, TOP);
    text("1. Sélectionner le type de message :", 25, 60);
    
    // Bouton Type 1
    drawButton(25, 85, 230, 35, "Type 1 : Statut Robot", selectedMsgType == 0);
    // Bouton Type 2
    drawButton(285, 85, 230, 35, "Type 2 : Demande Mission", selectedMsgType == 1);
    
    // --- ZONE PARAMÈTRES DYNAMIQUES ---
    fill(45, 50, 60);
    stroke(60, 65, 75);
    rect(25, 140, 490, 240, 4);
    
    fill(230);
    text("2. Configurer les paramètres du message :", 40, 155);
    
    if (selectedMsgType == 0) {
      // Configuration graphiques des Sliders pour Type 1
      drawSlider(40, 190, 350, "ID Robot [1 - " + nb_robot + "]", simRobotId, 1, nb_robot, 0);
      drawSlider(40, 240, 350, "ID Poste Courant [1 - " + nb_poste + "]", simPosteId, 1, nb_poste, 1);
      drawSlider(40, 290, 350, "Vitesse Consigne [0x0 - 0xF]", simVitesse, 0, 15, 2);
      
      // Sélecteur d'état (Status)
      text("Statut Opérationnel (S) :", 40, 340);
      drawSelectorChar(210, 335, new char[]{'L', 'E', 'C', 'D'}, simStatus, 3);
    } else {
      // Configuration graphiques des Sliders pour Type 2
      drawSlider(40, 190, 350, "Poste Émetteur (pp)", simPosteId, 1, nb_poste, 4);
      drawSlider(40, 240, 350, "Poste Destination (qq)", simPosteDest, 1, nb_poste, 5);
      
      // Sélecteur de code livreur (Code)
      text("Code Classe Livreur (C) :", 40, 300);
      drawSelectorChar(210, 295, new char[]{'A', 'B', 'C', 'D'}, simCodeLiv, 6);
    }
    
    // --- VISUALISATION DE LA TRAME SORTANTE ---
    fill(20);
    stroke(80);
    rect(25, 395, 490, 35, 4);
    fill(50, 205, 50); // Vert console
    textAlign(CENTER, CENTER);
    
    String previewStr = "";
    if (selectedMsgType == 0) {
      previewStr = "Trame générée : P" + nf(simPosteId, 2) + "R" + nf(simRobotId, 2) + "V" + hex(simVitesse, 2) + "S" + simStatus + "\\n";
    } else {
      previewStr = "Trame générée : P" + nf(simPosteId, 2) + "C" + simCodeLiv + "P" + nf(simPosteDest, 2) + "\\n";
    }
    text(previewStr, width/2, 412);
    
    // --- BOUTON DE TRANSMISSION EMISSION ---
    drawSendButton(175, 445, 200, 42, "EMETTRE (SEND)");
  }
  
  // -- Fonctions utilitaires de dessin IHM --
  void drawButton(int x, int y, int w, int h, String label, boolean active) {
    if (active) fill(230, 120, 10); else fill(55, 60, 70);
    stroke(active ? 255 : 90);
    rect(x, y, w, h, 4);
    fill(255);
    textAlign(CENTER, CENTER);
    text(label, x + w/2, y + h/2);
  }
  
  void drawSlider(int x, int y, int w, String label, int val, int minV, int maxV, int sliderId) {
    fill(190);
    textAlign(LEFT, CENTER);
    text(label + " : " + val, x, y);
    
    int barY = y + 15;
    stroke(80);
    fill(30);
    rect(x, barY, w, 8, 4);
    
    float sliderX = map(val, minV, maxV, x, x + w);
    noStroke();
    fill(50, 150, 255);
    rect(x, barY, sliderX - x, 8, 4);
    ellipse(sliderX, barY + 4, 16, 16);
    
    // Interaction souris sur le slider
    if (mousePressed && mouseX >= x && mouseX <= x+w && mouseY >= barY-10 && mouseY <= barY+15) {
      int newVal = (int)map(mouseX, x, x+w, minV, maxV);
      newVal = constrain(newVal, minV, maxV);
      updateSliderValue(sliderId, newVal);
    }
  }
  
  void drawSelectorChar(int x, int y, char[] options, char current, int selId) {
    for (int i = 0; i < options.length; i++) {
      boolean sel = (options[i] == current);
      if (sel) fill(50, 150, 255); else fill(70, 75, 85);
      stroke(sel ? 255 : 100);
      rect(x + (i * 45), y, 38, 26, 3);
      fill(255);
      textAlign(CENTER, CENTER);
      text(options[i], x + (i * 45) + 19, y + 13);
      
      if (mousePressed && mouseX >= x+(i*45) && mouseX <= x+(i*45)+38 && mouseY >= y && mouseY <= y+26) {
        updateCharValue(selId, options[i]);
      }
    }
  }
  
  void drawSendButton(int x, int y, int w, int h, String label) {
    if (mousePressed && mouseX >= x && mouseX <= x+w && mouseY >= y && mouseY <= y+h) {
      fill(34, 139, 34);
      stroke(255);
    } else {
      fill(46, 204, 113);
      stroke(255);
    }
    rect(x, y, w, h, 6);
    fill(255);
    textSize(14);
    textAlign(CENTER, CENTER);
    text(label, x + w/2, y + h/2);
  }
  
  void updateSliderValue(int id, int v) {
    if (id == 0) simRobotId = v;
    if (id == 1) simPosteId = v;
    if (id == 2) simVitesse = v;
    if (id == 4) simPosteId = v;
    if (id == 5) simPosteDest = v;
  }
  
  void updateCharValue(int id, char c) {
    if (id == 3) simStatus = c;
    if (id == 6) simCodeLiv = c;
  }
  
  public void mousePressed() {
    // Changement d'onglets de types de message
    if (mouseX >= 25 && mouseX <= 255 && mouseY >= 85 && mouseY <= 120) selectedMsgType = 0;
    if (mouseX >= 285 && mouseX <= 515 && mouseY >= 85 && mouseY <= 120) selectedMsgType = 1;
    
    // Déclenchement de l'envoi de la commande
    if (mouseX >= 175 && mouseX <= 375 && mouseY >= 445 && mouseY <= 487) {
      sendSimulatedMessage();
      delay(80); // Antirebond logiciel rudimentaire
    }
  }
}
