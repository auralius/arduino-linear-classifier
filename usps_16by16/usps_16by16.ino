/**
 * Auralius Manurung -- Universitas Telkom
 * auralius.manurung@ieee.org
 */

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;  // hard-wired for UNO shields anyway.
#include <TouchScreen.h>
#include "matrix.h"  // the weight matrix is here

const int XP = 8, YP = A3, XM = A2, YM = 9;  //most common configuration
const int TS_LEFT = 80, TS_RT = 904, TS_TOP = 901, TS_BOT = 111;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

#define MINPRESSURE 200
#define MAXPRESSURE 1000

const int16_t PENRADIUS = 1;

const int16_t L   = 128;
const int16_t L16 = 8;

const int16_t W8  = 240 / 8;
const int16_t W16 = 240 / 16;
const int16_t W2  = 240 / 2;

int16_t ROI[8];  //xmin, ymin, xmax, ymax, width, length, center x, center y

// The discretized drawing area: 8x8 grids, max value of each grid is 16
byte GRID[16][16];

// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define WHITE 0xFFFF

uint16_t GREYS[32];  // Grayscale colors in 32 steps: 0 to 255

// Grayscale of 32 steps from black (0x0000) to white(0xFFFF)
void create_greys() {
  for (int16_t k = 0; k < 32; k++)
    GREYS[k] = ((k << 11) | (2 * k << 5) | k);
}

// Clear the grid data, set all to 0
void reset_grid() {
  for (int16_t i = 0; i < 16; i++)
    for (int16_t j = 0; j < 16; j++)
      GRID[i][j] = 0;

  // Reset ROI
  for (int16_t i = 0; i<8; i++)
    ROI[i] = 0;

  ROI[0] = 1000;  // xmin
  ROI[1] = 1000;  // ymin
}


// Normalize the numbers in the grids such that they range from 0 to 16
void normalize_grid() {
  // find the maximum
  int16_t maxval = 0;
  for (int16_t i = 0; i < 16; i++)
    for (int16_t j = 0; j < 16; j++)
      if (GRID[i][j] > maxval)
        maxval = GRID[i][j];

  // normalize such that the maximum is 255.0
  for (int16_t i = 0; i < 16; i++)
    for (int16_t j = 0; j < 16; j++) 
      GRID[i][j] = round((float)GRID[i][j] / (float)maxval * 255.0);  // round instead of floor!
}


// Draw the grids in the screen
// The fill-color or the gray-level of each grid is set based on its value
void area_setup() {
  tft.fillRect(0, 0, 240, 320 - W8, BLACK);

  for (int16_t i = 0; i < 16; i++) {
    for (int16_t j = 0; j < 16; j++) {
      tft.fillRect(i * L16, j * L16, L16, L16, GREYS[GRID[i][j] / 8]);
    }
  }

  tft.drawRect(0, 0, L, L, GREEN);
}


// Track ROI, the specific area where the drawing occurs
void track_roi(int16_t xpos, int16_t ypos) {
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
/*
void draw_roi() {
  tft.drawRect(ROI[0], ROI[1], ROI[4], ROI[5], RED);
  tft.drawCircle(ROI[6], ROI[7], 2, RED);
}
*/

// Draw the two button at the bottom of the screen
void draw_buttons(char *label1, char *label2) {
  // Add 2 buttons in the bottom: CLEAR and PREDICT
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
void print_label(byte label) {
  char label_txt[3];
  itoa(label, label_txt, 10);

  tft.setTextColor(RED);
  
  tft.setCursor(W16 * 9, W16 * 2);
  tft.setTextSize(1);
  tft.print("Prediction:");

  tft.setCursor(W16 * 10, W16 * 3);
  tft.setTextSize(4);
  tft.print(label);
}


// Arduino setup function
void setup(void) {
  //Serial.begin(9600);
  create_greys();

  tft.reset();
  tft.begin(tft.readID());
  //tft.setRotation(0);

  reset_grid();
  area_setup();

  draw_buttons("CLEAR", "PREDICT");
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
    xpos = map(tp.x, TS_LEFT, TS_RT,  0, tft.width());
    ypos = map(tp.y, TS_BOT, TS_TOP, 0, tft.height());

    // are we in drawing area ?
    if (((ypos - PENRADIUS) > 0) && ((ypos + PENRADIUS) < L) && ((xpos - PENRADIUS) > 0) && ((xpos + PENRADIUS) < L)) {
      tft.fillCircle(xpos, ypos, PENRADIUS, BLUE);
      track_roi(xpos, ypos);
    }

    // CLEAR?
    if ((ypos > tft.height() - W8) && (xpos < W2)) {
      reset_grid();
      area_setup();
    }

    // PREDICT?
    if ((ypos > tft.height() - W8) && (xpos > W2)) {
      //draw_roi();

      for (int16_t i = 0; i < 16; i++) {
        for (int16_t j = 0; j < 16; j++) {
          for (int16_t k = 0; k < 8; k++) {
            for (int16_t l = 0; l < 8; l++) {
              int16_t x = i * L16 + k;
              int16_t y = j * L16 + l;

              uint16_t pixel = tft.readPixel(x, y);

              if (pixel == BLUE) {
                float s = 128.0 / (float)ROI[5];
                int16_t x_ = s * (float)(x - ROI[0]) + 64.0 - s * 0.5 * (float)ROI[4];  // Align to center (60,60)
                int16_t y_ = s * (float)(y - ROI[1]);

                if ((x_ >= 0) && (x_ < L) && (y_ >= 0) && (y_ < L)){
                  tft.fillCircle(x_, y_, 1, RED);
                  GRID[x_ / L16][y_ / L16] = GRID[x_ / L16][y_ / L16] + 1;
                  //GRID[x/L16][y/L16] = GRID[x/L16][y/L16] + 1;
                }
              }
            }
          }
        }
      }
      //delay(1000);
      normalize_grid();
      area_setup();
      draw_buttons("CLEAR", "PREDICT");
      predict();
    }
  }
}


// Convert to float, normalize (0.0 to 1.0), flatten the grids, 
// add 1.0 at the end (for the bias tricks)
float get_x(int16_t i) {
  if (i < 16*16)
    return (float)GRID[i % 16][i / 16];
  else
    return 1.0;
}


// Do the prediction
void predict() {
  // Compute the scores: y = x * W (matrix multiplication)
  float y[10];
  for (int16_t i = 0; i < 10; i++) {
    y[i] = 0;
    for (int16_t j = 0; j < (16*16+1); j++) {
      float w = pgm_read_float(&W[i + j * 10]);  // Read from PROGMEM one element only!
      y[i] = y[i] + (get_x(j) * w);
    }
  }

  // Find the largest scores
  float max_y = 0;
  int16_t label;
  for (int16_t i = 0; i < 10; i++) {
    if (y[i] > max_y) {
      max_y = y[i];
      label = i;
    }
  }

  // Display the results
  print_label(label);

  // Display the scores
  tft.setCursor(0, W16 * 9);
  tft.setTextSize(1);
  tft.println("Scores:\n");
  for (int16_t i = 0; i < 10; i++){
    if (i == label)
      tft.setTextColor(RED);
    else
      tft.setTextColor(BLUE);

    tft.print(i);
    tft.print(" : ");
    tft.println(y[i], 4);
  }
}
