/**
 * Auralius Manurung -- Universitas Telkom
 * auralius.manurung@ieee.org
 */

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;  // hard-wired for UNO shields anyway.
#include <TouchScreen.h>
#include "matrix.h" // the weight matrix is here

const int XP = 6, XM = A2, YP = A1, YM = 7;  //ID=0x9341
const int TS_LEFT = 907, TS_RT = 136, TS_TOP = 942, TS_BOT = 139;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

#define MINPRESSURE 100
#define MAXPRESSURE 1000

int16_t ROI[8];  //xmin, ymin, xmax, ymax, width, length, center x, center y

int16_t W2, W8, W815;  // W2: width/2, W8: width/8, W810: width/8/10
int16_t W16, W1615;    // W16: width/16, W1615: width/16/15
int16_t PENRADIUS = 1;

// The discretized drawing area: 8x8 grids, max value of each grid is 16
byte GRID[8][8];

// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

uint16_t GREYS[17];  // Grayscale colors in 17 steps: 0 to 16

// Grayscale of 17 steps from black (0x0000) to white(0xFFFF)
// #15 and #16 are both (0xFFFF) for simplification since 17 is an odd number
void create_greys() {
  for (byte k = 0; k < 16; k++)
    GREYS[k] = ((2 * k << 11) | (4 * k << 5) | 2 * k);
  GREYS[16] = 0xFFFF;
}


// This comes with the standard demo of the touchscreen
void show_tft(void) {
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.print(F("ID=0x"));
  tft.println(tft.readID(), HEX);
  tft.println("Screen is " + String(tft.width()) + "x" + String(tft.height()));
  tft.println("");
  tft.setTextSize(2);
  tft.println("");
  tft.setTextSize(1);
  tft.println("PORTRAIT Values:");
  tft.println("LEFT = " + String(TS_LEFT) + " RT  = " + String(TS_RT));
  tft.println("TOP  = " + String(TS_TOP) + " BOT = " + String(TS_BOT));
  tft.println("\nWiring is: ");
  tft.println("YP=" + String(YP) + " XM=" + String(XM));
  tft.println("YM=" + String(YM) + " XP=" + String(XP));
  tft.setTextSize(2);
  tft.setTextColor(RED);
  tft.setCursor((tft.width() - 48) / 2, (tft.height() * 2) / 4);
  tft.print("EXIT");
  tft.setTextColor(YELLOW, BLACK);
  tft.setCursor(0, (tft.height() * 6) / 8);
  tft.print("  Touch the screen!");
  while (1) {
    tp = ts.getPoint();
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (tp.z < MINPRESSURE || tp.z > MAXPRESSURE) continue;
    if (tp.x > 450 && tp.x < 570 && tp.y > 450 && tp.y < 570) break;
    tft.setCursor(0, (tft.height() * 3) / 4);
    tft.print("tp.x=" + String(tp.x) + " tp.y=" + String(tp.y) + "   ");
  }
}


// Clear the grid data, set all to 0
void reset_grid() {
  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++)
      GRID[i][j] = 0;

  // Reset ROI
  ROI[0] = 1000;  // xmin
  ROI[1] = 1000;  // ymin
  ROI[2] = 0;     // xmax
  ROI[3] = 0;     // ymax
  ROI[4] = 0;     
  ROI[5] = 0;     
  ROI[6] = 0;
  ROI[7] = 0;
}


// Normalize the numbers in the grids such that they range from 0 to 16
void normalize_grid() {
  float maxval = 0.0;
  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++)
      if (GRID[i][j] > maxval)
        maxval = GRID[i][j];

  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++) {
      GRID[i][j] = (float)GRID[i][j] / maxval * 16.0 + 0.5;  // round instead of floor!
      if (GRID[i][j] > 16)
        GRID[i][j] = 16;
    }
}


// Draw the grids in the screen
// The fill-color or the gray-level of each grid is set based on its value
void area_setup() {
  for (byte i = 0; i < 8; i++) {
    for (byte j = 0; j < 8; j++) {
      tft.fillRect(i * W16, j * W16, W16, W16, GREYS[GRID[i][j]]);
      tft.drawRect(i * W16, j * W16, W16, W16, CYAN);
    }
  }
}


// Track ROI, the specific area where the drawing occurs
void track_roi(uint16_t xpos, uint16_t ypos) {
  if (xpos - PENRADIUS < ROI[0])
    ROI[0] = xpos - PENRADIUS;
  if (xpos + PENRADIUS > ROI[2])
    ROI[2] = xpos + PENRADIUS;
  if (ypos - PENRADIUS < ROI[1])
    ROI[1] = ypos - PENRADIUS;
  if (ypos + PENRADIUS > ROI[3])
    ROI[3] = ypos + PENRADIUS;

  ROI[4] = ROI[2] - ROI[0];
  ROI[5] = ROI[3] - ROI[1];

  ROI[6] = ROI[0] + ROI[4] / 2;
  ROI[7] = ROI[1] + ROI[5] / 2;

}


// Draw the ROI as a red rectangle
void draw_roi() {
  tft.drawRect(ROI[0], ROI[1], ROI[4], ROI[5], RED);
  tft.drawCircle(ROI[6], ROI[7], 2,RED);
}

// Draw the two button at the bottom of the screen
void draw_buttons(char *label1, char *label2) {
  // Add 2 buttons in the bottom: CLEAR and IDENTIFY
  tft.fillRect(0, tft.height() - W8, W2, W2, GREEN);
  tft.fillRect(W2, tft.height() - W8, W2, W2, RED);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(6, tft.height() - W8 + 6);
  tft.print(label1);
  tft.setCursor(W2 + 6, tft.height() - W8 + 6);
  tft.print(label2);
}


// Put text at the bottom of the screen, right above the two buttons
void set_lower_text(char *label) {
  tft.fillRect(0, tft.height() - 2 * W8 - 8, tft.width(), W8 + 8, BLACK);
  tft.setTextColor(BLUE);
  tft.setTextSize(5);
  tft.setCursor(W2 - 4, tft.height() - 2 * W8 - 8);
  tft.print(label);
}

// Arduino setup function
void setup(void) {
  create_greys();

  tft.reset();
  tft.begin(tft.readID());
  Serial.begin(9600);
  tft.setRotation(0);
  tft.fillScreen(BLACK);
  show_tft();

  tft.fillScreen(BLACK);

  W8 = tft.width() / 8;
  W16 = tft.width() / 16;
  W1615 = tft.width() / 16 / 15;
  W2 = tft.width() / 2;
  W815 = W8 / 15;

  reset_grid();
  area_setup();

  draw_buttons("CLEAR", "IDENTIFY");

  delay(1000);
}

void loop() {
  int16_t xpos, ypos;  //screen coordinates
  tp = ts.getPoint();  //tp.x, tp.y are ADC values

  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // we have some minimum pressure we consider 'valid'
  if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE) {
    /// Map to your current pixel orientation
    xpos = map(tp.x, TS_RT, TS_LEFT, 0, tft.width());
    ypos = map(tp.y, TS_TOP, TS_BOT, 0, tft.height());

    // are we in drawing area ?
    if (((ypos - PENRADIUS) > 0) && ((ypos + PENRADIUS) < W16 * 8) && ((xpos - PENRADIUS) > 0) && ((xpos + PENRADIUS) < W16 * 8)) {
      tft.fillCircle(xpos, ypos, PENRADIUS, BLUE);
      track_roi(xpos, ypos);
    }

    // CLEAR?
    if ((ypos > tft.height() - W8) && (xpos < W2)) {
      tft.fillRect(0, 0, tft.width(), W8 * 8, BLACK);
      tft.fillRect(0, tft.height() - 2 * W8 - 8, tft.width(), W8 + 8, BLACK);
      reset_grid();
      area_setup();
    }

    // IDENTIFY?
    if ((ypos > tft.height() - W8) && (xpos > W2)) {
      draw_buttons("WAIT...", "WAIT...");
      draw_roi();

      for (byte i = 0; i < 8; i++) {
        for (byte j = 0; j < 8; j++) {
          
          for (byte k = 0; k < 15; k++) {
            for (byte l = 0; l < 15; l++) {
              uint16_t x = i * W16 + k * W1615;
              uint16_t y = j * W16 + l * W1615;

              uint16_t pixel = tft.readPixel(x, y);
              if (pixel == BLUE) {
                //uint16_t x_ = x + (60-ROI[6]); // Align to center (120,120)
                //uint16_t y_ = y + (60-ROI[7]); 
                float s = 120.0/(float)ROI[5];
                float s2 = s*0.5;
                uint16_t x_ = s*(x - ROI[0])+60-ROI[4]*s2; // Align to center (120,120)
                uint16_t y_ = s*(y - ROI[1]); 
                if ((x_>=0) && (x_<120) && (y_>=0) && (y_<120))
                  tft.fillCircle(x_, y_, 1, RED);
                
                GRID[x_/W16][y_/W16] = GRID[x_/W16][y_/W16] + 1;
              }
            }
          }
        }
      }
      //delay(1000);
      normalize_grid();
      area_setup();
      draw_buttons("CLEAR", "IDENTIFY");
      predict();
    }
  }
}

// Do the prediction
void predict() {
  // Flatten the grids, add 1 at the end
  float x[65];
  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++)
      x[i + j * 8] = GRID[i][j];
  x[64] = 1;

  // Compute the score values
  // y = x * W (matrix multiplication)
  float y[10];
  for (uint16_t i = 0; i < 10; i++) {
    y[i] = 0;
    for (uint16_t j = 0; j < 65; j++) {
      float w = pgm_read_float(&W[i + j * 10]); // Read from PROGMEM one element only!
      y[i] = y[i] + (x[j] * w);
    }
  }

  // Find the largest scores
  float max_y = 0;
  int16_t label;
  for (byte i = 0; i < 10; i++) {
    if (y[i] > max_y) {
      max_y = y[i];
      label = i;
    }
  }

  // Display the results
  char label_txt[2];
  itoa(label, label_txt, 10);
  set_lower_text(label_txt);

  //Serial.println(">>> x:");
  //for (uint16_t i=0; i<65; i++)
  //  Serial.println(x[i]);
  //Serial.println(">>> y:");
  //for (uint16_t i = 0; i < 10; i++)
  //  Serial.println(y[i]);
}
