// Global variables to track button position and state
int btnX = 150;
int btnY = 200;
int btnWidth = 100;
int btnHeight = 40;
boolean buttonClicked = false;

// --- New Animation Variables ---
String message = "Hello, world !";
float[] letterYOffsets;      // Stores the vertical offset for each letter
int activeLetterFreezer = -1; // Which letter is currently the center of attention
float animationTimer = 0;    // Tracks the progress of the current letter's jump
float jumpSpeed = 0.2;       // How fast the letter jumps
float jumpHeight = 30;       // How high the letter jumps
float rainbowHue = 0;        // Global hue tracker for the rainbow effect

void setup() {   
  size(400, 400); // Defines the window size (Width, Height)
  smooth();       // Enables anti-aliasing for smoother shapes
  
  // Initialize the array to keep track of individual letter offsets
  letterYOffsets = new float[message.length()];
}

void draw() {
  // Clear the background each frame (R, G, B)
  background(240, 240, 245); 
  
  // Update rainbow hue cycle
  rainbowHue = (rainbowHue + 2) % 360;
  
  // 1. Draw a simple title
  fill(50); // Dark gray text color
  textSize(20);
  textAlign(CENTER, CENTER);
  text("Simple UI Demonstration", width/2, 50);
  
  // --- Handle Letter Animations ---
  if (buttonClicked && activeLetterFreezer != -1) {
    // Progress the jump animation using a sine wave
    animationTimer += jumpSpeed;
    
    // Calculate the jump offset (negative because up is negative Y in Processing)
    letterYOffsets[activeLetterFreezer] = -abs(sin(animationTimer) * jumpHeight);
    
    // Once the sine wave completes one full arch (PI)
    if (animationTimer >= PI) {
      // Reset current letter's position
      letterYOffsets[activeLetterFreezer] = 0;
      // Reset timer for the next letter
      animationTimer = 0;
      // Move to the next letter
      activeLetterFreezer++;
      
      // If we reached the end of the string, stop animating
      if (activeLetterFreezer >= message.length()) {
        activeLetterFreezer = -1;
        buttonClicked = false; // Reset button state
      }
    }
  }

  // --- Draw the Animated Text ---
  textSize(24);
  textAlign(CENTER, CENTER);
  
  // Calculate total width to center the individual characters properly
  float totalTextWidth = textWidth(message);
  float startX = (width - totalTextWidth) / 2;
  float currentX = startX;
  float textBaselineY = 130; // Center of the screen

  for (int i = 0; i < message.length(); i++) {
    char c = message.charAt(i);
    float charWidth = textWidth(c);
    
    // Check if this letter or its neighbors are active
    boolean isCurrent = (i == activeLetterFreezer);
    boolean isNeighbor = (activeLetterFreezer != -1 && abs(i - activeLetterFreezer) == 1);
    
    if (isCurrent || isNeighbor) {
      // Switch to HSB to easily get a vibrant rainbow color
      colorMode(HSB, 360, 100, 100);
      fill(rainbowHue, 80, 90);
      colorMode(RGB, 255, 255, 255); // Switch back to default RGB safely
    } else {
      fill(120); // Default grey text
    }
    
    // Draw the character with its specific Y offset
    text(c, currentX + charWidth/2, textBaselineY + letterYOffsets[i]);
    
    // Move X cursor forward for the next letter
    currentX += charWidth;
  }
  
  // 2. Check if the mouse is hovering over the button
  if (mouseX >= btnX && mouseX <= btnX + btnWidth &&
      mouseY >= btnY && mouseY <= btnY + btnHeight) {
    fill(100, 150, 250); // Lighter blue on hover
  } else {
    fill(50, 100, 200);  // Default blue
  }
  
  // 3. Draw the button shape
  noStroke();
  rect(btnX, btnY, btnWidth, btnHeight, 7); // 7 is the corner radius
  
  // 4. Draw the button label
  fill(255); // White text
  textSize(14);
  textAlign(CENTER, CENTER);
  text("Click Me", btnX + (btnWidth/2), btnY + (btnHeight/2) - 2);
  
  // 5. Display dynamic UI response based on interaction
  if (buttonClicked) {
    fill(46, 204, 113); // Green color
    textSize(16);
    text("Status: Button Activated!", width/2, 300);
    
    // Draw a small visual indicator
    stroke(46, 204, 113);
    strokeWeight(3);
    noFill();
    ellipse(width/2, 340, 20, 20);
  } else {
    fill(150); // Gray color
    textSize(16);
    text("Status: Awaiting Input", width/2, 300);
  }
}

// Built-in event function that triggers automatically on a mouse click
void mousePressed() {
  // Check if the click happened inside the button boundaries
  if (mouseX >= btnX && mouseX <= btnX + btnWidth &&
      mouseY >= btnY && mouseY <= btnY + btnHeight) {
    
    // Only trigger if an animation isn't already running
    if (activeLetterFreezer == -1) {
      buttonClicked = true; 
      activeLetterFreezer = 0; // Start animation sequence at the first letter
      animationTimer = 0;
    }
  }
}
