// ================================================================
//  SUPERVISION ROBOT MOBILE — Processing 2.1.2
//
//  Architecture : 3 sous-écrans empilés verticalement
//    Sous-écran 1 (haut)    : VUE DU SYSTEME   — circuit en 8
//    Sous-écran 2 (milieu)  : GESTIONNAIRE DE MISSION — tableau
//    Sous-écran 3 (bas)     : LISTE ROBOTS      — grille d'état
//
//  Communication série (UART 115200 bauds) :
//    Message entrant type 1  : "P[pp]R[rr]V[vv]S[s]"
//      pp = numéro poste (2 chiffres décimaux)
//      rr = numéro robot (2 chiffres décimaux)
//      vv = vitesse      (2 chiffres hexadécimaux, 00-0F)
//      s  = status       (L | E | C | D)
//    Message entrant type 2  : "P[pp]C[c]P[qq]"
//      pp = poste expéditeur, c = code livreur (A-D), qq = poste dest.
//    Message sortant type 3  : "R[rr]P[pp]P[qq]\n\r"
//    Message sortant type 4  : "R[rr]V[vv]\n\r"
//
//  Fichier de configuration : config.txt (même dossier que le .pde)
//    Ligne 1 : nb_robot      (entier)
//    Ligne 2 : nb_poste_nord (entier)
//    Ligne 3 : nb_poste_sud  (entier)
// ================================================================

import processing.serial.*;
import processing.event.*;

// ── CONFIGURATION (lue depuis config.txt) ──────────────────────
int nb_robot      = 0;
int nb_poste_nord = 0;
int nb_poste_sud  = 0;
int nb_poste      = 0;
int id_last_poste_nord = 0;   // = nb_poste
int id_last_poste_sud  = 0;   // = nb_poste_sud

// ── DIMENSIONS ÉCRAN ───────────────────────────────────────────
final int W     = 1080;
final int H     = 1840;
final int TAB_H = 32;    // hauteur des onglets de sous-écran
final int MG    = 22;    // marge générale

// ── ZONES DES SOUS-ÉCRANS ──────────────────────────────────────
int s1_y, s1_h;   // VUE DU SYSTEME    (haut)
int s2_y, s2_h;   // GESTIONNAIRE      (milieu)
int s3_y, s3_h;   // LISTE ROBOTS      (bas)

// ── TABLEAU MISSION (sous-écran 2) ────────────────────────────
int   tableTopY;
int[] colW         = new int[5];
final int ROW_H    = 44;
final int HDR_H    = 44;
int   taille_tab_mission = 0;
int   tabScrollOff       = 0;

// ── GRILLE ROBOTS (sous-écran 3) ──────────────────────────────
int rob_cols, rob_rows, rob_cw, rob_ch, s3_content_y;

// ── GÉOMÉTRIE DU CIRCUIT ───────────────────────────────────────
// Forme en 8 : Y orange central + boucle [ gauche + boucle ] droite
int Y_cx, Y_cy;                           // centre du croisement Y
int Y_tl_x, Y_tl_y;                       // raccord Y — boucle gauche, haut
int Y_tr_x, Y_tr_y;                       // raccord Y — boucle droite, haut
int Y_bl_x, Y_bl_y;                       // raccord Y — boucle gauche, bas
int Y_br_x, Y_br_y;                       // raccord Y — boucle droite, bas
int L_left, R_right;                       // bords extérieurs des boucles

PVector[] posteXY;    // position de chaque poste (1-indexé)
float[]   posteTick;  // angle du trait de poste  (perpendiculaire au circuit)
boolean[] pFV;        // flèche visible ?
boolean[] pFG;        // flèche en mode « garé » ?
long[]    pFHA;       // heure de disparition de la flèche (millis)

// chemins des branches pour le calcul de position des postes
// [0] = branche sud (boucle gauche), [1] = branche nord (boucle droite)
PVector[][] branchPath = new PVector[2][];

// ── DIMENSIONS DU ROBOT SUR CIRCUIT ───────────────────────────
final int ROB_LONG  = 48;   // côté long  (perpendiculaire au circuit)
final int ROB_SHORT = 22;   // côté court (dans l'axe du circuit)

// ── STRUCTURES DE DONNÉES ─────────────────────────────────────
Robot[]            listeRobot;
Poste[]            listePoste;
ArrayList<Mission> listeMission  = new ArrayList<Mission>();
StringList         fifo_messages = new StringList();

int  numero_mission_select = -1;
int  num_robot_select      = -1;
long[] robot_last_seen;

// ── ÉTAT D'INTERACTION ─────────────────────────────────────────
boolean awaiting_robot_click = false;
int     selected_mission_row = -1;

// Menu déroulant vitesse
boolean dd_open        = false;
int     dd_mission_row = -1;
int     dd_x, dd_y, dd_w;
final String[] DD_ITEMS  = {"20%","40%","60%","80%"};
final int[]    DD_SPEEDS = {3, 6, 9, 12};   // valeurs hex (≈ 20–80 %)
final int      DD_IH     = 44;               // hauteur d'un item

// Animation clignotement erreur
int     blink_row   = -1;
int     blink_count = 0;
long    blink_next  = 0;
boolean blink_red   = false;

// ── LIAISON SÉRIE ─────────────────────────────────────────────
Serial serialPort;

// ── PALETTE DE COULEURS ───────────────────────────────────────
final color C_ORG = color(215,143,18);    // orange (Y, onglets)
final color C_G50 = color(128);           // gris 50 % (fond onglet)
final color C_G25 = color(196);           // gris 25 % (en-tête tableau)
final color C_BLU = color(30,100,220);    // bleu  (case attribution, status D)
final color C_GRN = color(35,180,55);     // vert  (flèches, status L, robot dispo)
final color C_RED = color(210,30,30);     // rouge (status E, robot indispo)
final color C_FUS = color(200,0,180);     // fuchsia (status C)
final color C_BLK = color(0);
final color C_WHT = color(255);

// ── POLICES ────────────────────────────────────────────────────
PFont fMono, fBold, fSm;

// ================================================================
//  SETUP
// ================================================================
void setup() {
  size(1080, 1840);
  smooth();
  fMono = createFont("Courier New", 16, true);
  fBold = createFont("Arial Bold",  17, true);
  fSm   = createFont("Courier New", 14, true);

  loadConfig();
  computeLayout();
  computeCircuit();
  initSerial();
  frameRate(30);

  initTerminal();
  initSimulator();
}

// ── Chargement et initialisation de la configuration ──────────
void loadConfig() {
  // Chemin absolu spécifié dans le cahier des charges
  String cfgPath =
    "/data/idhem/Documents/Sorbonne/L3/" +
    "robot-mobile-pilote-sorbonne-2026/supervision/bin/config.txt";
  String[] lines = loadStrings(cfgPath);
  if (lines != null && lines.length >= 3) {
    try {
      nb_robot      = Integer.parseInt(trim(lines[0]));
      nb_poste_nord = Integer.parseInt(trim(lines[1]));
      nb_poste_sud  = Integer.parseInt(trim(lines[2]));
    } catch (Exception e) { useDefaultConfig(); }
  } else {
    useDefaultConfig();
  }
  applyConfig();
}

void useDefaultConfig() {
  nb_robot = 10; nb_poste_nord = 7; nb_poste_sud = 7;
  println("[SUPERVISION] config.txt introuvable — valeurs par défaut utilisées.");
}

void applyConfig() {
  nb_poste           = nb_poste_nord + nb_poste_sud;
  id_last_poste_sud  = nb_poste_sud;
  id_last_poste_nord = nb_poste;

  // Tableaux de robots et de postes (indices 1..n)
  listeRobot = new Robot[nb_robot + 1];
  for (int i = 1; i <= nb_robot; i++) listeRobot[i] = new Robot(i);

  listePoste = new Poste[nb_poste + 1];
  for (int i = 1; i <= nb_poste; i++)  listePoste[i]  = new Poste(i);

  robot_last_seen = new long[nb_robot + 1];
}

// ── Calcul des zones écran ─────────────────────────────────────
void computeLayout() {
  // Sous-écran 3 — taille dépend du nombre de robots
  rob_cw = 130; rob_ch = 55;
  rob_cols = max(1, (W - 2*MG) / rob_cw);
  rob_rows = (int)Math.ceil((float)nb_robot / rob_cols);
  s3_h         = TAB_H + 12 + rob_rows * rob_ch + 12;
  s3_y         = H - s3_h;
  s3_content_y = s3_y + TAB_H + 12;

  // Sous-écran 1 — hauteur fixe (circuit)
  s1_y = 0;
  s1_h = 650;

  // Sous-écran 2 — espace restant
  s2_y = s1_h;
  s2_h = s3_y - s2_y;

  // Largeurs des colonnes du tableau de mission
  int tw = W - 2*MG;
  colW[0] = (int)(tw * 0.27f); // Trajet
  colW[1] = (int)(tw * 0.15f); // Robot
  colW[2] = (int)(tw * 0.20f); // Code Livreur
  colW[3] = (int)(tw * 0.17f); // Status
  colW[4] = tw - colW[0] - colW[1] - colW[2] - colW[3]; // Vitesse

  tableTopY          = s2_y + TAB_H + MG;
  taille_tab_mission = max(1, (s2_h - TAB_H - 2*MG - HDR_H) / ROW_H);
}

// ── Géométrie du circuit ───────────────────────────────────────
void computeCircuit() {
  int ct = s1_y + TAB_H + 70;   // haut du circuit (marge haute augmentée)
  int cb = s1_y + s1_h - 70;    // bas  du circuit (marge basse augmentée)
  // Marges plus larges pour laisser la place aux labels de postes
  // et aux robots garés à l'extérieur du circuit
  L_left  = 110;
  R_right = W - 110;

  // Demi-ouverture horizontale des bras supérieurs du Y
  int sp = 175;
  Y_tl_x = W/2 - sp;  Y_tl_y = ct;
  Y_tr_x = W/2 + sp;  Y_tr_y = ct;
  // Les deux boucles partagent un point commun en bas du tronc du Y (W/2, cb)
  Y_bl_x = W/2;        Y_bl_y = cb;
  Y_br_x = W/2;        Y_br_y = cb;
  Y_cx   = W/2;
  // Jonction du Y à 1/4 de la hauteur totale du circuit (bras courts, tronc long)
  Y_cy   = ct + (cb - ct) / 4;

  // Les chemins incluent les extrémités des biseaux pour que les postes
  // qui tombent sur un biseau reçoivent exactement le bon angle perpendiculaire.
  int cr = 18;  // doit correspondre à cr dans drawCircuit()

  // Chemin de la boucle gauche  [  (branche sud)
  branchPath[0] = new PVector[] {
    new PVector(Y_tl_x,      Y_tl_y),       // raccord Y haut-gauche
    new PVector(L_left + cr, Y_tl_y),       // fin segment horizontal haut
    new PVector(L_left,      Y_tl_y + cr),  // fin biseau coin haut-gauche
    new PVector(L_left,      Y_bl_y - cr),  // fin segment vertical gauche
    new PVector(L_left + cr, Y_bl_y),       // fin biseau coin bas-gauche
    new PVector(Y_cx,        Y_bl_y)        // raccord tronc Y bas
  };
  // Chemin de la boucle droite ]  (branche nord)
  branchPath[1] = new PVector[] {
    new PVector(Y_tr_x,       Y_tr_y),      // raccord Y haut-droit
    new PVector(R_right - cr, Y_tr_y),      // fin segment horizontal haut
    new PVector(R_right,      Y_tr_y + cr), // fin biseau coin haut-droit
    new PVector(R_right,      Y_br_y - cr), // fin segment vertical droit
    new PVector(R_right - cr, Y_br_y),      // fin biseau coin bas-droit
    new PVector(Y_cx,         Y_br_y)       // raccord tronc Y bas
  };

  posteXY   = new PVector[nb_poste + 1];
  posteTick = new float[nb_poste + 1];
  pFV       = new boolean[nb_poste + 1];
  pFG       = new boolean[nb_poste + 1];
  pFHA      = new long[nb_poste + 1];

  // Postes 1..nb_poste_sud   → boucle sud (gauche)
  distributePosts(0, 1,                nb_poste_sud);
  // Postes nb_poste_sud+1..nb_poste → boucle nord (droite)
  distributePosts(1, nb_poste_sud + 1, nb_poste);
}

// Répartit n postes uniformément le long du chemin d'une branche.
// Les biseaux étant des segments explicites dans le chemin, l'angle perpendiculaire
// est automatiquement exact pour tout poste (y compris ceux dans les coins).
void distributePosts(int branch, int from, int to) {
  PVector[] path = branchPath[branch];
  int n = to - from + 1;
  if (n <= 0) return;

  int     ns  = path.length - 1;
  float[] sl  = new float[ns];
  float   tot = 0;
  for (int i = 0; i < ns; i++) {
    sl[i] = PVector.dist(path[i], path[i+1]);
    tot  += sl[i];
  }

  for (int k = 0; k < n; k++) {
    float t   = tot * (k + 1.0f) / (n + 1.0f);
    float acc = 0;
    PVector pt  = new PVector(path[ns].x, path[ns].y);
    float   ang = 0;

    for (int s = 0; s < ns; s++) {
      float na = acc + sl[s];
      if (t <= na + 1e-4f || s == ns - 1) {
        float lt = (sl[s] > 1e-4f) ? constrain((t - acc) / sl[s], 0, 1) : 0;
        pt = PVector.lerp(path[s], path[s+1], lt);
        float dx = path[s+1].x - path[s].x;
        float dy = path[s+1].y - path[s].y;
        ang = atan2(dy, dx) + HALF_PI;  // perpendiculaire au segment courant
        break;
      }
      acc = na;
    }
    posteXY  [from + k] = pt;
    posteTick[from + k] = ang;
  }
}


// ── Initialisation de la liaison série ────────────────────────
void initSerial() {
  try {
    String[] ports = Serial.list();
    if (ports.length > 0) {
      serialPort = new Serial(this, ports[0], 115200);
      serialPort.bufferUntil('\n');
      println("[SUPERVISION] Port série : " + ports[0]);
    } else {
      println("[SUPERVISION] Aucun port série détecté — mode simulation.");
    }
  } catch (Exception e) {
    println("[SUPERVISION] Erreur série : " + e.getMessage());
  }
}

// ================================================================
//  BOUCLE PRINCIPALE
// ================================================================
void draw() {
  background(220);

  // Vidange de la FIFO de messages
  while (fifo_messages.size() > 0) {
    processMsg(fifo_messages.get(0));
    fifo_messages.remove(0);
  }

  // Expiration des flèches de poste (1 seconde)
  for (int i = 1; i <= nb_poste; i++)
    if (pFV[i] && millis() > pFHA[i]) pFV[i] = false;

  // Timeout robot : disparition après 2 minutes sans message
  for (int i = 1; i <= nb_robot; i++)
    if (listeRobot[i].visible && robot_last_seen[i] > 0
        && millis() - robot_last_seen[i] > 120000)
      listeRobot[i].visible = false;

  // Mise à jour de l'animation de clignotement
  if (blink_row >= 0 && blink_count > 0 && millis() > blink_next) {
    blink_red  = !blink_red;
    blink_next = millis() + 150;
    if (!blink_red) blink_count--;
    if (blink_count <= 0) { blink_row = -1; blink_red = false; }
  }

  drawS1();
  drawS2();
  drawS3();
  drawSeparators();
}

// ================================================================
//  SOUS-ÉCRAN 1 — VUE DU SYSTEME
// ================================================================
void drawS1() {
  noStroke(); fill(248, 248, 252);
  rect(0, s1_y, W, s1_h);

  drawCircuit();
  for (int i = 1; i <= nb_robot; i++)
    if (listeRobot[i].visible) drawRobot(listeRobot[i]);

  drawTab(0, s1_y, "VUE DU SYSTEME");
}

// ── Dessin du circuit (boucles + Y + postes) ──────────────────
void drawCircuit() {
  pushStyle();
  int cr = 18;   // taille du chanfrein aux coins des boucles

  // Boucle gauche [  (branche sud)
  stroke(C_BLK); strokeWeight(3.5f); noFill();
  drawOpenRect(Y_tl_x, Y_tl_y, L_left,  Y_tl_y,
               L_left,  Y_bl_y, Y_bl_x, Y_bl_y, cr, true);

  // Boucle droite ]  (branche nord)
  drawOpenRect(Y_tr_x, Y_tr_y, R_right, Y_tr_y,
               R_right, Y_br_y, Y_br_x, Y_br_y, cr, false);

  // Y orange : 2 bras supérieurs divergents + 1 tronc vertical descendant
  stroke(C_ORG); strokeWeight(8); noFill();
  line(Y_tl_x, Y_tl_y, Y_cx, Y_cy);   // bras supérieur gauche
  line(Y_tr_x, Y_tr_y, Y_cx, Y_cy);   // bras supérieur droit
  line(Y_cx,   Y_cy,   Y_cx, Y_bl_y); // tronc vertical vers le bas

  // Postes (traits + labels + flèches)
  for (int i = 1; i <= nb_poste; i++) drawPoste(i);

  popStyle();
}

// Dessine un rectangle ouvert à coins légèrement chanfreinés.
// P1→P2 = segment horizontal haut, P2→P3 = côté vertical, P3→P4 = segment bas.
// isLeft : true → coin à gauche ([), false → coin à droite (])
void drawOpenRect(float x1, float y1, float x2, float y2,
                  float x3, float y3, float x4, float y4,
                  int r, boolean isLeft) {
  float s = isLeft ? r : -r;   // sens du chanfrein
  line(x1,    y1,    x2+s,  y2    );   // segment haut
  line(x2+s,  y2,    x2,    y2+r  );   // chanfrein coin haut
  line(x2,    y2+r,  x3,    y3-r  );   // côté vertical
  line(x3,    y3-r,  x3+s,  y3    );   // chanfrein coin bas
  line(x3+s,  y3,    x4,    y4    );   // segment bas
}

// ── Dessin d'un poste (trait + label PNN + flèche verte) ──────
void drawPoste(int idx) {
  if (posteXY[idx] == null) return;
  float px  = posteXY[idx].x,  py  = posteXY[idx].y;
  float ang = posteTick[idx];
  float hl  = ROB_LONG / 2.0f;             // demi-longueur du trait

  // Vecteur unitaire le long du trait
  float tdx = cos(ang) * hl,  tdy = sin(ang) * hl;

  pushStyle();

  // Trait du poste
  stroke(C_BLK); strokeWeight(2.5f);
  line(px - tdx, py - tdy, px + tdx, py + tdy);

  // Direction extérieure (côté label = extérieur de la boucle)
  boolean isSud = (idx <= nb_poste_sud);
  float lcx = isSud ? (float)(L_left  + Y_tl_x)/2
                    : (float)(R_right + Y_tr_x)/2;
  float lcy = (float)(Y_tl_y + Y_bl_y)/2;
  float nx  = cos(ang), ny = sin(ang);
  float dot = nx*(lcx-px) + ny*(lcy-py);
  float ox  = (dot > 0) ? -nx : nx;
  float oy  = (dot > 0) ? -ny : ny;

  // Label PNN côté extérieur (orientation normale du texte)
  fill(C_BLK); noStroke();
  textFont(fSm); textSize(14); textAlign(CENTER, CENTER);
  text("P" + nf(idx, 2), px + ox*(hl+15), py + oy*(hl+15));

  // Flèche verte côté intérieur (visible temporairement après message)
  if (pFV[idx]) {
    float ax = px - ox*(hl+10);
    float ay = py - oy*(hl+10);
    if (pFG[idx]) {
      // Flèche de parking : pointe vers le poste depuis l'intérieur
      drawArrow(ax, ay, ox, oy, 20);
    } else {
      // Flèche de déplacement : dans le sens du circuit
      float cdx = cos(ang - HALF_PI);
      float cdy = sin(ang - HALF_PI);
      drawArrow(ax, ay, cdx, cdy, 20);
    }
  }

  popStyle();
}

// Dessine une flèche verte (remplie, contour noir) en (x,y) dans la direction (dx,dy)
void drawArrow(float x, float y, float dx, float dy, float sz) {
  float a = atan2(dy, dx);
  pushMatrix();
  translate(x, y); rotate(a);
  fill(C_GRN); stroke(C_BLK); strokeWeight(1.5f);
  beginShape();
  vertex(-sz*0.50f, -sz*0.28f);
  vertex( sz*0.15f, -sz*0.28f);
  vertex( sz*0.15f, -sz*0.50f);
  vertex( sz*0.55f,  0        );
  vertex( sz*0.15f,  sz*0.50f);
  vertex( sz*0.15f,  sz*0.28f);
  vertex(-sz*0.50f,  sz*0.28f);
  endShape(CLOSE);
  popMatrix();
}

// ── Dessin d'un robot sur le circuit ──────────────────────────
void drawRobot(Robot r) {
  PVector pos = robPos(r);
  if (pos == null) return;
  float ang = robAng(r);

  pushMatrix();
  translate(pos.x, pos.y);
  rotate(ang);

  // Rectangle blanc contour noir
  fill(C_WHT); stroke(C_BLK); strokeWeight(2);
  rectMode(CENTER);
  rect(0, 0, ROB_LONG, ROB_SHORT);

  // Jauge de vitesse (5 barres)
  int   bars = vToBars(r.vitesse);
  color gc   = stColor(r.status);
  float bw   = (ROB_LONG - 6.0f) / 5;
  float bh   = ROB_SHORT - 6.0f;
  for (int b = 0; b < 5; b++) {
    float bx = -ROB_LONG/2.0f + 3 + b*bw + bw/2;
    fill(b < bars ? gc : C_G25);
    noStroke();
    rect(bx, 0, bw - 1, bh);
  }
  // Redessine le contour par-dessus la jauge
  noFill(); stroke(C_BLK); strokeWeight(2);
  rect(0, 0, ROB_LONG, ROB_SHORT);

  popMatrix();
  rectMode(CORNER);

  // Label RNN orange (côté extérieur de la boucle, orientation normale)
  float lox, loy;
  if (r.id_last_poste == id_last_poste_sud) {
    lox = -1; loy = 0;
  } else if (r.id_last_poste == id_last_poste_nord) {
    lox = 1; loy = 0;
  } else {
    boolean isSud = (r.id_last_poste >= 1 && r.id_last_poste <= nb_poste_sud);
    float lcx = isSud ? (float)(L_left  + Y_tl_x)/2
                      : (float)(R_right + Y_tr_x)/2;
    float lcy = (float)(Y_tl_y + Y_bl_y)/2;
    float nx  = cos(ang + HALF_PI),  ny = sin(ang + HALF_PI);
    float dot = nx*(lcx - pos.x) + ny*(lcy - pos.y);
    lox = (dot > 0) ? -nx : nx;
    loy = (dot > 0) ? -ny : ny;
  }
  pushStyle();
  fill(C_ORG); noStroke();
  textFont(fSm); textSize(15); textAlign(CENTER, CENTER);
  text("R" + nf(r.id_robot, 2), pos.x + lox*35, pos.y + loy*35);
  popStyle();
}

// Calcule la position PVector d'un robot sur l'écran
PVector robPos(Robot r) {
  int lp = r.id_last_poste;
  if (lp <= 0) return null;

  // Cas spéciaux : robot aligné le long du tronc du Y
  if (lp == id_last_poste_sud)
    return new PVector(Y_cx - ROB_LONG*0.8f,
                       Y_cy + (Y_bl_y - Y_cy) * 0.4f);
  if (lp == id_last_poste_nord)
    return new PVector(Y_cx + ROB_LONG*0.8f,
                       Y_cy + (Y_bl_y - Y_cy) * 0.4f);

  if (posteXY[lp] == null) return null;

  // Robot garé : côté extérieur du poste
  if (r.gare) {
    float px  = posteXY[lp].x,  py  = posteXY[lp].y;
    float a   = posteTick[lp];
    boolean isSud = (lp <= nb_poste_sud);
    float lcx = isSud ? (float)(L_left  + Y_tl_x)/2
                      : (float)(R_right + Y_tr_x)/2;
    float lcy = (float)(Y_tl_y + Y_bl_y)/2;
    float nx  = cos(a), ny = sin(a);
    float dot = nx*(lcx-px) + ny*(lcy-py);
    float ox  = (dot > 0) ? -nx : nx;
    float oy  = (dot > 0) ? -ny : ny;
    return new PVector(px + ox*(ROB_LONG/2.0f + ROB_SHORT*0.8f),
                       py + oy*(ROB_LONG/2.0f + ROB_SHORT*0.8f));
  }

  // Robot en déplacement : entre son poste et le suivant
  int np = nextPoste(lp);
  if (np > 0 && posteXY[np] != null)
    return new PVector((posteXY[lp].x + posteXY[np].x)/2,
                       (posteXY[lp].y + posteXY[np].y)/2);

  // Repli : sur le poste
  return new PVector(posteXY[lp].x, posteXY[lp].y);
}

// Retourne le poste suivant dans la même branche (ou -1 si dernier)
int nextPoste(int p) {
  if (p >= 1 && p < nb_poste_sud)  return p + 1;
  if (p > nb_poste_sud && p < nb_poste) return p + 1;
  return -1;
}

// Angle d'orientation du rectangle robot
float robAng(Robot r) {
  int lp = r.id_last_poste;
  if (lp == id_last_poste_sud || lp == id_last_poste_nord) return 0; // horizontal
  if (lp >= 1 && lp <= nb_poste) return posteTick[lp];
  return 0;
}

// Convertit la vitesse (0-15) en nombre de barres affichées (0-5)
int vToBars(int v) {
  if (v == 0)  return 0;
  if (v <= 3)  return 1;
  if (v <= 6)  return 2;
  if (v <= 9)  return 3;
  if (v <= 12) return 4;
  return 5;
}

// Retourne la couleur de jauge correspondant au statut
color stColor(char s) {
  if (s == 'L') return C_GRN;
  if (s == 'E') return C_RED;
  if (s == 'C') return C_FUS;
  if (s == 'D') return C_BLU;
  return C_G25;
}

// ================================================================
//  SOUS-ÉCRAN 2 — GESTIONNAIRE DE MISSION
// ================================================================
void drawS2() {
  noStroke(); fill(252, 252, 255);
  rect(0, s2_y, W, s2_h);

  drawTab(0, s2_y, "GESTIONNAIRE DE MISSION");
  drawMissionTable();
}

void drawMissionTable() {
  int tx = MG,  ty = tableTopY,  tw = W - 2*MG;
  int tH = HDR_H + taille_tab_mission * ROW_H;

  pushStyle();

  // Fond en-tête gris 25 %
  fill(C_G25); noStroke();
  rect(tx, ty, tw, HDR_H);

  // Bordure extérieure épaisse
  noFill(); stroke(C_BLK); strokeWeight(3);
  rect(tx, ty, tw, tH);

  // Séparation en-tête / corps (épaisse)
  strokeWeight(3);
  line(tx, ty+HDR_H, tx+tw, ty+HDR_H);

  // Séparations de colonnes (épaisses)
  strokeWeight(2.5f);
  int cx = tx;
  for (int c = 0; c < 4; c++) { cx += colW[c]; line(cx, ty, cx, ty+tH); }

  // Labels d'en-tête (gras)
  fill(C_BLK); noStroke();
  String[] hdr = {"Trajet","Robot","Code Livreur","Status","Vitesse"};
  cx = tx;
  for (int c = 0; c < 5; c++) {
    textFont(fBold); textSize(16); textAlign(CENTER, CENTER);
    text(hdr[c], cx + colW[c]/2.0f, ty + HDR_H/2.0f);
    cx += colW[c];
  }

  // Lignes de données
  for (int row = 0; row < taille_tab_mission; row++) {
    int ry = ty + HDR_H + row * ROW_H;
    stroke(C_BLK); strokeWeight(1);
    line(tx, ry, tx+tw, ry);   // séparation fine entre lignes

    int mi = tabScrollOff + row;
    if (mi < listeMission.size())
      drawMRow(listeMission.get(mi), mi, tx, ry,
               blink_row == mi && blink_red);
    // Lignes vides → fond blanc (ne rien faire)
  }

  // Indicateurs de défilement (si nécessaire)
  if (listeMission.size() > taille_tab_mission) {
    fill(C_BLK); noStroke();
    textFont(fBold); textSize(22); textAlign(CENTER, CENTER);
    int ax = W - 18;
    if (tabScrollOff > 0)
      text("▲", ax, ty + HDR_H + 24);
    if (tabScrollOff + taille_tab_mission < listeMission.size())
      text("▼", ax, ty + tH - 24);
  }

  if (dd_open) drawDropdown();

  popStyle();
}

// Dessine le contenu d'une ligne du tableau de mission
void drawMRow(Mission m, int mi, int tx, int ry,
              boolean doBlinkRed) {
  pushStyle();
  textAlign(CENTER, CENTER);

  // ── Colonne 0 : Trajet ─────────────────────────────────────
  textFont(fMono); textSize(14); fill(C_BLK); noStroke();
  String traj = "P" + nf(m.p_depart,2) + " --> P" + nf(m.p_arrive,2);
  text(traj, tx + colW[0]/2.0f, ry + ROW_H/2.0f);

  // ── Colonne 1 : Robot ──────────────────────────────────────
  int c1x       = tx + colW[0];
  boolean unasn = (m.id_livreur <= 0);

  if (doBlinkRed) {
    // Clignotement rouge (animation erreur sélection)
    fill(C_RED); noStroke();
    rect(c1x+1, ry+1, colW[1]-2, ROW_H-2);
  } else if (unasn) {
    // Case attribution (? bleu, contour bleu)
    if (awaiting_robot_click && selected_mission_row == mi) {
      // État sélectionné : fond bleu 75 %
      fill(color(30, 100, 220, 190)); noStroke();
      rect(c1x+1, ry+1, colW[1]-2, ROW_H-2);
    }
    noFill(); stroke(C_BLU); strokeWeight(2.5f);
    rect(c1x+2, ry+2, colW[1]-4, ROW_H-4);
    fill(C_BLU); noStroke();
    textFont(fBold); textSize(22);
    text("?", c1x + colW[1]/2.0f, ry + ROW_H/2.0f);
  } else {
    // Robot assigné : numéro NN
    textFont(fMono); textSize(15); fill(C_BLK); noStroke();
    text(nf(m.id_livreur, 2), c1x + colW[1]/2.0f, ry + ROW_H/2.0f);
  }

  // ── Colonne 2 : Code Livreur ───────────────────────────────
  int c2x = c1x + colW[1];
  char codeChar = (m.code_livreur >= 0 && m.code_livreur <= 3)
                  ? (char)('A' + m.code_livreur) : '?';
  textFont(fMono); textSize(16); fill(C_BLK); noStroke();
  text(String.valueOf(codeChar), c2x + colW[2]/2.0f, ry + ROW_H/2.0f);

  // ── Colonne 3 : Status ─────────────────────────────────────
  int c3x = c2x + colW[2];
  textFont(fBold); textSize(18);
  if (m.status == '?') {
    fill(C_BLK); noStroke();
    text("?", c3x + colW[3]/2.0f, ry + ROW_H/2.0f);
  } else {
    fill(stColor(m.status)); noStroke();
    text(String.valueOf(m.status), c3x + colW[3]/2.0f, ry + ROW_H/2.0f);
  }

  // ── Colonne 4 : Vitesse ────────────────────────────────────
  int c4x = c3x + colW[3];
  if (m.id_livreur > 0 && m.id_livreur <= nb_robot) {
    // Liste déroulante : valeur actuelle + flèche indicatrice
    int    v   = listeRobot[m.id_livreur].vitesse;
    String spd = (vToBars(v) * 20) + "%";
    // Boîte de la liste déroulante
    stroke(C_BLK); strokeWeight(1); noFill();
    rect(c4x+4, ry+4, colW[4]-8, ROW_H-8, 3);
    textFont(fMono); textSize(15); fill(C_BLK); noStroke();
    text(spd, c4x + (colW[4]-16)/2.0f, ry + ROW_H/2.0f);
    // Triangle indicateur de liste déroulante
    fill(C_BLK); noStroke();
    int ao = c4x + colW[4] - 14,  am = ry + ROW_H/2;
    triangle(ao, am-5, ao+10, am-5, ao+5, am+5);
  } else {
    textFont(fBold); textSize(18); fill(C_BLK); noStroke();
    text("?", c4x + colW[4]/2.0f, ry + ROW_H/2.0f);
  }

  popStyle();
}

// Dessine le menu déroulant de vitesse (superposé au tableau)
void drawDropdown() {
  int dh = DD_ITEMS.length * DD_IH;
  pushStyle();
  fill(C_WHT); stroke(C_BLK); strokeWeight(1.5f);
  rect(dd_x, dd_y, dd_w, dh, 3);

  for (int i = 0; i < DD_ITEMS.length; i++) {
    int iy = dd_y + i * DD_IH;
    // Surlignage au survol
    if (mouseX > dd_x && mouseX < dd_x+dd_w
        && mouseY > iy  && mouseY < iy+DD_IH) {
      fill(C_G25); noStroke();
      rect(dd_x+1, iy+1, dd_w-2, DD_IH-2);
    }
    fill(C_BLK); noStroke();
    textFont(fMono); textSize(16); textAlign(CENTER, CENTER);
    text(DD_ITEMS[i], dd_x + dd_w/2.0f, iy + DD_IH/2.0f);
    if (i < DD_ITEMS.length-1) {
      stroke(C_G25); strokeWeight(1);
      line(dd_x, iy+DD_IH, dd_x+dd_w, iy+DD_IH);
    }
  }
  // Redessine le contour
  noFill(); stroke(C_BLK); strokeWeight(1.5f);
  rect(dd_x, dd_y, dd_w, dh, 3);
  popStyle();
}

// ================================================================
//  SOUS-ÉCRAN 3 — LISTE ROBOTS
// ================================================================
void drawS3() {
  noStroke(); fill(248, 248, 252);
  rect(0, s3_y, W, s3_h);

  drawTab(0, s3_y, "LISTE ROBOTS");

  int sx = MG,  sy = s3_content_y;
  int cw = (W - 2*MG) / rob_cols;

  for (int i = 0; i < nb_robot; i++) {
    int rid = i + 1;
    int col = i % rob_cols,  row = i / rob_cols;
    int cx  = sx + col * cw,  cy = sy + row * rob_ch;

    pushStyle();
    Robot rob = listeRobot[rid];

    // État du robot → couleur de fond et label
    boolean timedOut = !rob.visible && robot_last_seen[rid] > 0;
    color bg;
    String lbl;

    if (timedOut) {
      // Timeout : rouge + conserve le numéro (spec : "rouge mais conserve numéro")
      bg  = color(210, 30, 30, 128);
      lbl = "R" + nf(rid, 2);
    } else if (!rob.dispo) {
      // Indisponible (en mission) : rouge + "----"
      bg  = color(210, 30, 30, 128);
      lbl = "----";
    } else {
      // Disponible : vert + numéro
      bg  = color(35, 180, 55, 128);
      lbl = "R" + nf(rid, 2);
    }

    fill(bg); stroke(C_BLK); strokeWeight(1);
    rect(cx+2, cy+2, cw-4, rob_ch-4, 5);
    fill(C_BLK); noStroke();
    textFont(fMono); textSize(16); textAlign(CENTER, CENTER);
    text(lbl, cx + cw/2.0f, cy + rob_ch/2.0f);

    popStyle();
  }
  // Si nb_robot est impair, la dernière case reste vide (coin inférieur droit)
}

// ── Onglet de sous-écran ──────────────────────────────────────
void drawTab(int x, int y, String lbl) {
  pushStyle();
  textFont(fBold); textSize(16);
  int tw = (int)textWidth(lbl) + 28;
  fill(C_G50); stroke(C_ORG); strokeWeight(2);
  rect(x, y, tw, TAB_H);
  fill(C_WHT); noStroke();
  textAlign(LEFT, CENTER);
  text(lbl, x + 10, y + TAB_H/2.0f);
  popStyle();
}

// ── Lignes de séparation entre sous-écrans ────────────────────
void drawSeparators() {
  pushStyle();
  textFont(fBold); textSize(16);
  stroke(C_BLK); strokeWeight(2);

  // Entre S1 et S2 : ligne pleine largeur
  line(0, s2_y, W, s2_y);

  // Entre S2 et S3 : depuis le bord droit de l'onglet S3 jusqu'au bord droit
  int tab3W = (int)textWidth("LISTE ROBOTS") + 28;
  line(tab3W, s3_y, W, s3_y);

  popStyle();
}

// ================================================================
//  GESTION DES INTERACTIONS SOURIS
// ================================================================
void mousePressed() {
  // Un clic ferme toujours le menu déroulant s'il est ouvert
  if (dd_open) {
    int dh = DD_ITEMS.length * DD_IH;
    if (mouseX >= dd_x && mouseX <= dd_x+dd_w
        && mouseY >= dd_y && mouseY <= dd_y+dh) {
      // Sélection d'une vitesse dans la liste
      int item = (mouseY - dd_y) / DD_IH;
      if (item >= 0 && item < DD_ITEMS.length
          && dd_mission_row >= 0
          && dd_mission_row < listeMission.size()) {
        num_robot_select = listeMission.get(dd_mission_row).id_livreur;
        if (num_robot_select > 0 && num_robot_select <= nb_robot) {
          // Routine d'interaction 2 — étapes 5-7
          listeRobot[num_robot_select].vitesse = DD_SPEEDS[item];
          sendMsg2(num_robot_select);
        }
      }
    }
    dd_open = false;
    return;
  }

  // Dispatch selon le sous-écran cliqué
  if      (mouseY >= s3_y)  clickS3();
  else if (mouseY >= s2_y)  clickS2();
  else {
    // Clic dans S1 : annule toute sélection en attente
    if (awaiting_robot_click) {
      awaiting_robot_click = false;
      selected_mission_row = -1;
    }
  }
}

// ── Clic dans le sous-écran 3 (grille robots) ─────────────────
void clickS3() {
  int sx = MG,  sy = s3_content_y;
  int cw = (W - 2*MG) / rob_cols;

  for (int i = 0; i < nb_robot; i++) {
    int rid = i+1, col = i%rob_cols, row = i/rob_cols;
    int cx  = sx+col*cw,  cy = sy+row*rob_ch;

    if (mouseX >= cx && mouseX < cx+cw
        && mouseY >= cy && mouseY < cy+rob_ch) {

      if (awaiting_robot_click) {
        // Routine d'interaction 1 — étape 3 : robot cliqué
        Robot r = listeRobot[rid];
        if (r.dispo) {
          // Robot disponible → affectation à la mission
          r.dispo = false;
          listeMission.get(selected_mission_row).id_livreur = rid;
          sendMsg1(selected_mission_row);
        } else {
          // Robot indisponible → animation erreur (3 clignotements)
          trigBlink(selected_mission_row);
        }
        awaiting_robot_click = false;
        selected_mission_row = -1;
      }
      return;
    }
  }
  // Clic en dehors de toute case robot → annule la sélection
  if (awaiting_robot_click) {
    awaiting_robot_click = false;
    selected_mission_row = -1;
  }
}

// ── Clic dans le sous-écran 2 (tableau missions) ──────────────
void clickS2() {
  int tx = MG,  ty = tableTopY,  tw = W - 2*MG;
  int tH = HDR_H + taille_tab_mission * ROW_H;

  // Zone des flèches de défilement (bord droit du tableau)
  if (mouseX > tx+tw+2 && mouseX < W) {
    if (mouseY < ty+HDR_H+42 && tabScrollOff > 0) {
      tabScrollOff--; return;
    }
    if (mouseY > ty+tH-42
        && tabScrollOff + taille_tab_mission < listeMission.size()) {
      tabScrollOff++; return;
    }
  }

  // Hors de la zone tableau → annule sélection
  if (mouseX < tx || mouseX > tx+tw || mouseY < ty+HDR_H) {
    if (awaiting_robot_click) {
      awaiting_robot_click = false; selected_mission_row = -1;
    }
    return;
  }

  // Numéro de ligne cliquée
  int row = (mouseY - (ty + HDR_H)) / ROW_H;
  if (row < 0 || row >= taille_tab_mission) {
    if (awaiting_robot_click) {
      awaiting_robot_click = false; selected_mission_row = -1;
    }
    return;
  }
  int mi = tabScrollOff + row;
  if (mi >= listeMission.size()) {
    if (awaiting_robot_click) {
      awaiting_robot_click = false; selected_mission_row = -1;
    }
    return;
  }

  // Colonne cliquée
  int cx2 = tx,  col = -1;
  for (int c = 0; c < 5; c++) {
    if (mouseX >= cx2 && mouseX < cx2 + colW[c]) { col = c; break; }
    cx2 += colW[c];
  }

  Mission m = listeMission.get(mi);

  if (col == 1 && m.id_livreur <= 0) {
    // Routine d'interaction 1 — étape 1&2 : clic sur case attribution
    awaiting_robot_click = true;
    selected_mission_row = mi;
    numero_mission_select = mi;

  } else if (col == 1 && awaiting_robot_click) {
    // Clic sur une autre case attribution → change la sélection (étape 2)
    selected_mission_row  = mi;
    numero_mission_select = mi;

  } else if (col == 4 && m.id_livreur > 0) {
    // Routine d'interaction 2 — étapes 1-4 : ouverture liste déroulante vitesse
    dd_mission_row = mi;
    numero_mission_select = mi;
    num_robot_select = m.id_livreur;
    int c4x = tx + colW[0]+colW[1]+colW[2]+colW[3];
    int ry  = ty + HDR_H + row * ROW_H;
    dd_x = c4x;  dd_y = ry + ROW_H;  dd_w = colW[4];
    dd_open = true;
    // Un clic sur la vitesse annule toute sélection de robot en attente
    if (awaiting_robot_click) {
      awaiting_robot_click = false; selected_mission_row = -1;
    }

  } else {
    // Clic ailleurs → annule l'attente
    if (awaiting_robot_click) {
      awaiting_robot_click = false; selected_mission_row = -1;
    }
  }
}

// Lance l'animation erreur (3 clignotements rouges brefs)
void trigBlink(int row) {
  blink_row   = row;
  blink_count = 6;          // 3 cycles ON/OFF = 6 toggles
  blink_next  = millis();
  blink_red   = true;
}

// Molette souris → défilement du tableau de mission
void mouseWheel(MouseEvent e) {
  if (mouseY >= s2_y && mouseY < s3_y) {
    int d = (int)e.getCount();
    tabScrollOff = constrain(tabScrollOff + d, 0,
                   max(0, listeMission.size() - taille_tab_mission));
  }
}

// ================================================================
//  GESTION DE LA LIAISON SÉRIE
// ================================================================

// Appelé automatiquement à chaque réception d'un '\n'
void serialEvent(Serial p) {
  try {
    String s = p.readStringUntil('\n');
    if (s != null) {
      s = trim(s);
      if (s.length() > 0 && fifo_messages.size() < 100)
        fifo_messages.append(s);
        logIncomingUART(s);
    }
  } catch (Exception e) { /* silencieux */ }
}

// Traite un message extrait de la FIFO
void processMsg(String msg) {
  if (msg == null || msg.length() < 4) return;

  // Détection du type de message par la présence des marqueurs
  boolean hasV = msg.indexOf('V') >= 0;
  boolean hasS = (msg.indexOf('S') >= 0);
  boolean hasR = msg.indexOf('R') >= 0;
  boolean hasC = msg.indexOf('C') >= 0;

  if (hasR && hasV && hasS) parseMsg1(msg);   // type 1 : mise à jour robot
  else if (hasC)             parseMsg2(msg);   // type 2 : nouvelle mission
}

// ── Parsing message type 1 : "P[pp]R[rr]V[vv]S[s]" ───────────
// Mise à jour de l'état d'un robot
void parseMsg1(String msg) {
  try {
    // Recherche ordonnée des marqueurs (robuste si ordre variable)
    int pi = msg.indexOf('P');
    int ri = msg.indexOf('R', max(0, pi));
    int vi = msg.indexOf('V', max(0, ri));
    int si = msg.indexOf('S', max(0, vi));
    if (pi<0 || ri<0 || vi<0 || si<0) return;
    if (ri<=pi || vi<=ri || si<=vi)    return;

    int   np  = Integer.parseInt(trim(msg.substring(pi+1, ri)));
    int   nr  = Integer.parseInt(trim(msg.substring(ri+1, vi)));
    int   nv  = Integer.parseInt(trim(msg.substring(vi+1, si)), 16);
    char  ns  = msg.charAt(si+1);

    if (nr < 1 || nr > nb_robot) return;
    if (np < 1 || np > nb_poste) return;

    Robot rob = listeRobot[nr];
    robot_last_seen[nr] = millis();
    rob.visible = true;

    // Mise à jour de l'indicateur « garé »
    int mx = mOfRobot(nr);
    if (mx >= 0) {
      Mission mq = listeMission.get(mx);
      if (rob.status == 'E' && np == mq.p_depart) rob.gare = true;
      if (rob.status == 'R' && np == mq.p_arrive) rob.gare = true;
    }
    if (rob.status == 'E' && ns == 'C') rob.gare = false;
    if (rob.status == 'R' && ns == 'L') { rob.gare = false; rob.dispo = true; }

    // Mise à jour des données du robot
    rob.id_last_poste = np;
    rob.vitesse       = nv;
    rob.status        = ns;

    // Activation de la flèche au poste (disparaît après 1 s)
    pFV[np]  = true;
    pFG[np]  = rob.gare;
    pFHA[np] = millis() + 1000;

    // Mise à jour du tableau de mission
    if (mx >= 0) listeMission.get(mx).status = ns;

  } catch (Exception e) {
    println("[SUPERVISION] Erreur parsing msg type1 : " + e.getMessage());
  }
}

// ── Parsing message type 2 : "P[pp]C[c]P[qq]" ────────────────
// Création d'une nouvelle mission
void parseMsg2(String msg) {
  try {
    int pi1 = msg.indexOf('P');
    int ci  = msg.indexOf('C', pi1+1);
    int pi2 = msg.indexOf('P', ci+1);
    if (pi1<0 || ci<0 || pi2<0) return;

    int  pe   = Integer.parseInt(trim(msg.substring(pi1+1, ci)));
    char code = msg.charAt(ci+1);
    int  pd   = Integer.parseInt(trim(msg.substring(pi2+1)));

    Mission m     = new Mission();
    m.p_depart    = pe;
    m.p_arrive    = pd;
    m.code_livreur = (code >= 'A' && code <= 'D') ? code - 'A' : 0;
    m.id_livreur  = 0;
    m.status      = '?';
    listeMission.add(m);

  } catch (Exception e) {
    println("[SUPERVISION] Erreur parsing msg type2 : " + e.getMessage());
  }
}

// Retourne l'indice dans listeMission de la mission d'un robot (-1 si aucune)
int mOfRobot(int rid) {
  for (int i = 0; i < listeMission.size(); i++)
    if (listeMission.get(i).id_livreur == rid) return i;
  return -1;
}

// ── Envoi message type 3 : "R[rr]P[pp]P[qq]\n\r" ─────────────
// Notifie la centrale : robot rr doit aller de pp à qq
void sendMsg1(int mi) {
  if (serialPort == null || mi < 0 || mi >= listeMission.size()) return;
  Mission m = listeMission.get(mi);
  if (m.id_livreur <= 0) return;
  String msgOut = "R" + nf(m.id_livreur,2)
                + "P" + nf(m.p_depart,2)
                + "P" + nf(m.p_arrive,2)
                + "\n\r";
                
  serialPort.write(msgOut);
  logOutgoingUART(msgOut);
}

// ── Envoi message type 4 : "R[rr]V[vv]\n\r" ──────────────────
// Notifie la centrale : consigne de vitesse pour le robot rr
void sendMsg2(int rid) {
  if (serialPort == null || rid <= 0 || rid > nb_robot) return;
  Robot r = listeRobot[rid];
  String msgOut = "R" + nf(r.id_robot,2)
                + "V" + hex(r.vitesse,2)
                + "\n\r";
                
  serialPort.write(msgOut);
  logOutgoingUART(msgOut);
}

// ================================================================
//  STRUCTURES DE DONNÉES
// ================================================================

class Robot {
  int     id_robot;
  int     id_last_poste = 0;    // id du dernier poste ayant signalé ce robot
  int     vitesse       = 0;    // vitesse courante (0x0–0xF)
  char    status        = '?';  // L | E | C | D | ?
  boolean gare          = false; // vrai si garé à côté d'un poste
  boolean dispo         = true;  // vrai si disponible pour une mission
  boolean visible       = false; // vrai si au moins un message reçu récemment

  Robot(int id) { id_robot = id; }
}

class Poste {
  int     id_poste;
  boolean robot_vu   = false;
  boolean robot_gare = false;

  Poste(int id) { id_poste = id; }
}

class Mission {
  int  p_depart     = 0;   // id poste de départ
  int  p_arrive     = 0;   // id poste d'arrivée
  int  id_livreur   = 0;   // id robot affecté (0 = non affecté)
  int  code_livreur = 0;   // 0=A, 1=B, 2=C, 3=D
  char status       = '?'; // statut courant (copie du statut du robot)
}

