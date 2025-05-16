/**
 * Auralius Manurung -- Universitas Telkom
 * auralius.manurung@ieee.org
 */

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;  // hard-wired for UNO shields anyway.
#include <TouchScreen.h>
#include "matrix.h"  // the weight matrix is here

const int XP = 6, YP = A1, XM = A2, YM = 7;  //most common configuration
const int TS_LEFT = 174, TS_RT = 902, TS_TOP = 215, TS_BOT = 957;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

#define MINPRESSURE 100
#define MAXPRESSURE 1000
# define PENRADIUS  1

const int16_t W8 = 240 / 8;
const int16_t W16 = 240 / 16;
const int16_t W2 = 240 / 2;

int16_t ROI[8];  //xmin, ymin, xmax, ymax, width, length, center x, center y

// The discretized drawing area: 8x8 grids, max value of each grid is 16
byte GRID[8][8];

// Grayscale colors in 17 steps: 0 to 16
uint16_t GREYS[17];  

// Grayscale of 17 steps from TFT_BLACK (0x0000) to white(0xFFFF)
// #15 and #16 are both (0xFFFF) for simplification since 17 is an odd number
void create_greys() {
  for (int16_t k = 0; k < 16; k++)
    GREYS[k] = ((2 * k << 11) | (4 * k << 5) | 2 * k);
  GREYS[16] = 0xFFFF;
}


// Clear the grid data, set all to 0
void reset_grid() {
  for (int16_t i = 0; i < 8; i++)
    for (int16_t j = 0; j < 8; j++)
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
  // find the maximum
  float maxval = 0.0;
  for (int16_t i = 0; i < 8; i++)
    for (int16_t j = 0; j < 8; j++)
      if (GRID[i][j] > maxval)
        maxval = GRID[i][j];

  // normalize such that the maximum is 16.0
  for (int16_t i = 0; i < 8; i++)
    for (int16_t j = 0; j < 8; j++) {
      GRID[i][j] = (float)GRID[i][j] / maxval * 16.0 + 0.5;  // round instead of floor!
      if (GRID[i][j] > 16)
        GRID[i][j] = 16;
    }
}


// Draw the grids in the screen
// The fill-color or the gray-level of each grid is set based on its value
void area_setup() {
  tft.fillRect(0, 0, 240, 320 - W8, TFT_BLACK);

  for (int16_t i = 0; i < 8; i++) {
    for (int16_t j = 0; j < 8; j++) {
      tft.fillRect(i * W16, j * W16, W16, W16, GREYS[GRID[i][j]]);
    }
  }

  tft.drawRect(0, 0, W16 * 8, W16 * 8, TFT_GREEN);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 320 - W8 * 2);
  tft.print(F("Linear Classifier Demo\nIntelligent Biomedical Systems,\nUniversitas Telkom, Bandung, 2025"));
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


// Draw the ROI as a TFT_RED rectangle
void draw_roi() {
  tft.drawRect(ROI[0], ROI[1], ROI[4], ROI[5], TFT_RED);
  tft.drawCircle(ROI[6], ROI[7], 2, TFT_RED);
}


// Draw the two button at the bottom of the screen
void draw_buttons(char *label1, char *label2) {
  // Add 2 buttons in the bottom: CLEAR and PREDICT
  tft.fillRect(0, 320 - W8, W2, W2, TFT_BLUE);
  tft.fillRect(W2, 320 - W8, W2, W2, TFT_RED);
  tft.setTextSize(2);
  tft.setCursor(6, 320 - W8 + 6);
  tft.print(label1);
  tft.setCursor(W2 + 6, 320 - W8 + 6);
  tft.print(label2);
}


// Put text at the bottom of the screen, right above the two buttons
void print_label(byte label) {
  tft.setCursor(W16 * 10, W16 * 2);
  tft.setTextSize(1);

  if (label == 255) {
    tft.print(F("Please wait!"));
  } else {
    tft.print(F("Prediction:"));

    tft.setCursor(W16 * 11, W16 * 4);
    tft.setTextSize(4);

    char label_txt[3];
    itoa(label, label_txt, 10);
    tft.print(label);
  }
}


// Arduino setup function
void setup(void) {
  //Serial.begin(9600);

  create_greys();

  tft.reset();
  tft.begin(tft.readID());
  tft.fillScreen(TFT_BLACK);

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
    xpos = map(tp.x, TS_LEFT, TS_RT, 0, 240);
    ypos = map(tp.y, TS_BOT, TS_TOP, 0, 320);

    // are we in drawing area ?
    if (((ypos - PENRADIUS) > 0) && ((ypos + PENRADIUS) < W16 * 8) && ((xpos - PENRADIUS) > 0) && ((xpos + PENRADIUS) < W16 * 8)) {
      tft.fillCircle(xpos, ypos, PENRADIUS, TFT_BLUE);
      track_roi(xpos, ypos);
    }

    // CLEAR?
    if ((ypos > 320 - W8) && (xpos < W2)) {
      reset_grid();
      area_setup();
    }

    // PREDICT?
    if ((ypos > 320 - W8) && (xpos > W2)) {
      print_label(255);
      draw_roi();

      for (int16_t i = 0; i < 8; i++) {
        for (int16_t j = 0; j < 8; j++) {
          for (int16_t k = 0; k < 15; k++) {
            for (int16_t l = 0; l < 15; l++) {
              int16_t x = i * W16 + k;
              int16_t y = j * W16 + l;

              uint16_t pixel = tft.readPixel(x, y);

              if (pixel == TFT_BLUE) {
                float s = 120.0 / (float)ROI[5];
                int16_t x_ = s * (float)(x - ROI[0]) + 60.0 - s * 0.5 * (float)ROI[4];  // Align to center (60,60)
                int16_t y_ = s * (float)(y - ROI[1]);

                if ((x_ >= 0) && (x_ < 120) && (y_ >= 0) && (y_ < 120)) {
                  tft.fillCircle(x_, y_, 1, TFT_RED);
                  GRID[x_ / W16][y_ / W16] = GRID[x_ / W16][y_ / W16] + 1;
                  //GRID[x/W16][y/W16] = GRID[x/W16][y/W16] + 1;
                }
              }
            }
          }
        }
      }
      //delay(1000);
      normalize_grid();
      area_setup();
      predict();
      reset_grid();
    }
  }
}


// Convert to float, flatten the grids, add 1 at the end (for the bias tricks)
float get_x(int16_t i) {
  if (i < 64)
    return (float)GRID[i % 8][i / 8];
  else
    return 1.0;
}


// Do the prediction
void predict() {
  float max_y = 0;
  int16_t label;

  // Compute the scores: y = x * W (matrix multiplication)
  float y[10];
  for (int16_t i = 0; i < 10; i++) {
    y[i] = 0;
    for (int16_t j = 0; j < 65; j++) {
      float w = pgm_read_float(&W[i + j * 10]);  // Read from PROGMEM one element only!
      y[i] = y[i] + (get_x(j) * w);
    }
    
    // Keep the largest!
    if (y[i] > max_y) {
      max_y = y[i];
      label = i;
    }
  }

  // Display the scores
  tft.setCursor(0, W16 * 9);
  tft.setTextSize(1);
  tft.println("Scores:\n");

  for (int16_t i = 0; i < 10; i++) {
    if (i == label)
      tft.setTextColor(TFT_RED);
    else
      tft.setTextColor(TFT_WHITE);

    tft.print(String(i));
    tft.print(" : ");
    tft.println(String(y[i], 4));
  }

  // Display the results
  print_label(label);
}
