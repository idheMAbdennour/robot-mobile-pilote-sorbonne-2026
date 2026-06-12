// ================================================================
//  TERMINAL D'AFFICHAGE DES MESSAGES UART — term.pde
//  Entièrement compatible avec supervision.pde (Processing 2.1.2)
// ================================================================

import java.util.ArrayList;

// Liste partagée et synchronisée pour stocker l'historique des messages UART
// Format d'un élément : "Date_Heure | [PROVENANCE] -> Message"
static ArrayList<String> uartLogHistory = new ArrayList<String>();
static final int MAX_LOG_LINES = 100; // Limite pour éviter la saturation mémoire

/**
 * Intercepte et formate un message entrant (reçu de la Centrale)
 */
void logIncomingUART(String msg) {
  String timestamp = nf(hour(), 2) + ":" + nf(minute(), 2) + ":" + nf(second(), 2);
  String logLine = timestamp + " | [CENTRALE -> SUPERVISION] : " + msg;
  synchronized(uartLogHistory) {
    uartLogHistory.add(logLine);
    if (uartLogHistory.size() > MAX_LOG_LINES) {
      uartLogHistory.remove(0);
    }
  }
}

/**
 * Intercepte et formate un message sortant (envoyé par la Supervision)
 */
void logOutgoingUART(String msg) {
  String timestamp = nf(hour(), 2) + ":" + nf(minute(), 2) + ":" + nf(second(), 2);
  // Nettoyage des sauts de ligne invisibles (\n\r) pour un affichage propre dans le terminal
  String cleanMsg = msg.replace("\n", "\\n").replace("\r", "\\r");
  String logLine = timestamp + " | [SUPERVISION -> CENTRALE] : " + cleanMsg;
  synchronized(uartLogHistory) {
    uartLogHistory.add(logLine);
    if (uartLogHistory.size() > MAX_LOG_LINES) {
      uartLogHistory.remove(0);
    }
  }
}

// ── EXTENSION DU PROGRAMME POUR LA SECONDE FENÊTRE ────────────────
// Au démarrage de supervision.pde, Processing appelle automatiquement cette fonction globale
void initTerminal() {
  String[] args = {"Terminal UART (LPCXpresso1769)"};
  TerminalApplet termWindow = new TerminalApplet();
  PApplet.runSketch(args, termWindow);
}
/**
 * Classe représentant la seconde fenêtre dédiée au terminal
 */
public class TerminalApplet extends PApplet {
  PFont fConsole;
  int padding = 15;
  int lineHeight = 22;

  public void setup() {
    // Configuration de la taille de la fenêtre du terminal
    size(700, 450);
    frameRate(15); // Une fréquence basse suffit amplement pour de l'affichage de texte
    
    // Utilisation d'une police à chasse fixe (Monospaced)
    fConsole = createFont("Courier New", 13, true);
  }

  public void draw() {
    background(15, 15, 20); // Fond sombre de type terminal
    
    // Titre du Terminal
    pushStyle();
    fill(40, 45, 55);
    noStroke();
    rect(0, 0, width, 35);
    fill(180, 190, 200);
    textFont(fConsole);
    textSize(12);
    textAlign(LEFT, CENTER);
    text(" MONITEUR DE LIAISON SÉRIE (UART) — 115200 bauds", padding, 17);
    stroke(60, 70, 80);
    line(0, 35, width, 35);
    popStyle();

    // Affichage des messages
    textFont(fConsole);
    textSize(13);
    textAlign(LEFT, TOP);
    
    int currentY = 45;
    
    synchronized(uartLogHistory) {
      // On calcule combien de lignes maximum peuvent entrer dans l'écran
      int maxVisibleLines = (height - currentY - padding) / lineHeight;
      int startIndex = max(0, uartLogHistory.size() - maxVisibleLines);
      
      for (int i = startIndex; i < uartLogHistory.size(); i++) {
        String lineText = uartLogHistory.get(i);
        
        // Coloration syntaxique ergonomique selon la provenance
        if (lineText.contains("[CENTRALE -> SUPERVISION]")) {
          fill(50, 180, 255); // Bleu clair pour les messages reçus de la LPCXpresso
        } else if (lineText.contains("[SUPERVISION -> CENTRALE]")) {
          fill(230, 140, 10); // Orange pour les messages émis par la supervision
        } else {
          fill(200);
        }
        
        text(lineText, padding, currentY);
        currentY += lineHeight;
      }
    }
    
    // Petit indicateur clignotant de bon fonctionnement en bas à droite
    if (frameCount % 30 < 15) {
      fill(35, 180, 55);
      noStroke();
      ellipse(width - 20, height - 20, 8, 8);
    }
  }
}
