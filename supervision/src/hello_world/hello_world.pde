// Global variables to track button position and state
int btnX = 150;
int btnY = 200;
int btnWidth = 100;
int btnHeight = 40;
boolean buttonClicked = false;

void setup() {   
  size(400, 400); // Defines the window size (Width, Height)
  smooth();       // Enables anti-aliasing for smoother shapes
}

void draw() {
  // Clear the background each frame (R, G, B)
  background(240, 240, 245); 
  
  // 1. Draw a simple title
  fill(50); // Dark gray text color
  textSize(20);
  textAlign(CENTER, CENTER);
  text("Simple UI Demonstration", width/2, 50);
  
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
    
    // Toggle the boolean state
    buttonClicked = !buttonClicked; 
  }
}
