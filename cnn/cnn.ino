/**
 * Auralius Manurung -- Universitas Telkom
 * auralius.manurung@ieee.org
 */
#include <SD.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;  // hard-wiTFT_RED for UNO shields anyway.
#include <TouchScreen.h>

const int XP = 8, YP = A3, XM = A2, YM = 9;  //most common configuration
const int TS_LEFT = 153, TS_RT = 864, TS_TOP = 895, TS_BOT = 114;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

#define SD_CS 10
#define MINPRESSURE 200
#define MAXPRESSURE 1000

const int16_t PENRADIUS = 1;

const int16_t L = 128;
const int16_t L8 = 16;
const int16_t L16 = 8;

const int16_t W8 = 240 / 8;
const int16_t W16 = 240 / 16;
const int16_t W2 = 240 / 2;

int16_t ROI[8];  //xmin, ymin, xmax, ymax, width, length, center x, center y

// The discretized drawing area: 8x8 grids, max value of each grid is 255
int16_t PADDED_GRID[18][18];
int16_t GRID[16][16];
byte POOLED_GRID[8][8];

// Grayscale colors in 17 steps: 0 to 16
uint16_t GREYS[17];

// Number of neurons in the hidden layer
const uint16_t HIDDEN_SIZE = 50;

// Filter kernel
const int16_t kernel[3][3] = {{1, 2, 1}, {2, 4, 2}, {1, 2, 2}};


// Grayscale of 16 steps from TFT_BLACK (0x0000) to TFT_WHITE(0xFFFF)
void create_greys() {
  for (int16_t k = 0; k < 16; k++)
    GREYS[k] = ((2 * k << 11) | (4 * k << 5) | (2 * k));
  GREYS[16] = 0xFFFF;
}


// Clear the grid data, set all to 0
void reset_grid() {
  for (int16_t i = 0; i < 16; i++)
    for (int16_t j = 0; j < 16; j++)
      GRID[i][j] = 0;

  for (int16_t i = 0; i < 18; i++)
    for (int16_t j = 0; j < 18; j++)
      PADDED_GRID[i][j] = 0;

  // Reset ROI
  for (int16_t i = 0; i < 8; i++)
    ROI[i] = 0;

  ROI[0] = 1000;  // xmin
  ROI[1] = 1000;  // ymin
}


void do_convolution() {
  // Padding
  for (int16_t i = 1; i < 17; i++)
    for (int16_t j = 1; j < 17; j++)
        PADDED_GRID[i][j] = GRID[i-1][j-1];

  // Convolution
  for (int16_t i = 0; i < 16; i++){
    for (int16_t j = 0; j < 16; j++) {
      int16_t v = 0;
      for (int16_t k = 0; k < 3; k++)
        for (int16_t l = 0; l < 3; l++)
          v = v + kernel[k][l] * PADDED_GRID[i + k][j + l];
      
      if (v < 0) v = 0;
      GRID[i][j] = v / 16;
    }
  }

  // Max Pooling
  for (int16_t i = 0; i < 8; i++){
    for (int16_t j = 0; j < 8; j++) {
      int16_t v = 0;
      for (int16_t k = 0; k < 2; k++)
        for (int16_t l = 0; l < 2; l++)
          if (GRID[i*2 + k][j*2 + l] > v)
            v =  GRID[i*2 + k][j*2 + l];
      
      POOLED_GRID[i][j] = v;
    }
  }
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
  tft.fillRect(0, 0, L, 320 - W8 * 2, TFT_BLACK);

  for (int16_t i = 0; i < 16; i++) {
    for (int16_t j = 0; j < 16; j++) {
      tft.fillRect(i * L16, j * L16, L16, L16, GREYS[GRID[i][j] / 16]);
    }
  }

  tft.drawRect(0, 0, L, L, TFT_GREEN);
}


void area_setup2() {
  for (int16_t i = 0; i < 8; i++) {
    for (int16_t j = 0; j < 8; j++) {
      tft.fillRect(i * 4, j * 4, 4, 4, GREYS[POOLED_GRID[i][j] / 16]);
    }
  }
  
  tft.drawRect(0, 0, 32, 32, TFT_RED);
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
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 320 - W8 * 2);
  tft.print(F("Convolutional Neural Network\nIntelligent Biomedical Systems\nUniversitas Telkom, Bandung"));

  // Add 2 buttons in the bottom: CLEAR and PREDICT
  tft.fillRect(0, 320 - W8, W2, W2, TFT_GREEN);
  tft.fillRect(W2, 320 - W8, W2, W2, TFT_RED);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(6, 320 - W8 + 6);
  tft.print(label1);
  tft.setCursor(W2 + 6, 320 - W8 + 6);
  tft.print(label2);
}


// Put text at the bottom of the screen, right above the two buttons
void print_label(byte label, uint16_t color = TFT_RED) {
  tft.fillRect(L, 0, 240 - L, 320 - W8 * 2, TFT_BLACK);
  tft.setCursor(W16 * 10, W16 * 2);
  tft.setTextSize(1);
  tft.setTextColor(color);

  if (label == 255) {
    tft.print(F("Please wait!"));
  } else if (label == 254) {
    tft.print(F("Go ahead!"));
  } else {
    tft.print(F("Prediction:"));

    tft.setCursor(W16 * 11, W16 * 4);
    tft.setTextSize(4);

    char label_txt[8];
    itoa(label, label_txt, 10);
    tft.print(label);
  }
}


// Arduino setup function
void setup(void) {
  //Serial.begin(9600);
  //while ( !Serial ) delay(2);

  create_greys();

  tft.reset();
  tft.begin(tft.readID());
  tft.fillScreen(TFT_BLACK);

  tft.setCursor(0, 10);
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);

  if (!SD.begin(SD_CS)) {
    tft.println(F("cannot start SD"));
    while (1)
      ;
  }

  reset_grid();
  area_setup();
  print_label(254);
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
    xpos = map(tp.x, TS_LEFT, TS_RT, 0, tft.width());
    ypos = map(tp.y, TS_BOT, TS_TOP, 0, tft.height());

    // are we in drawing area ?
    if (((ypos - PENRADIUS) > 0) && ((ypos + PENRADIUS) < L) && ((xpos - PENRADIUS) > 0) && ((xpos + PENRADIUS) < L)) {
      tft.fillCircle(xpos, ypos, PENRADIUS, TFT_BLUE);
      track_roi(xpos, ypos);
    }

    // CLEAR?
    if ((ypos > tft.height() - W8) && (xpos < W2)) {
      reset_grid();
      area_setup();
      print_label(254);
    }

    // PREDICT?
    if ((ypos > tft.height() - W8) && (xpos > W2)) {
      print_label(255);
      draw_roi();

      for (int16_t i = 0; i < 16; i++) {
        for (int16_t j = 0; j < 16; j++) {
          for (int16_t k = 0; k < 8; k++) {
            for (int16_t l = 0; l < 8; l++) {
              int16_t x = i * L16 + k;
              int16_t y = j * L16 + l;

              uint16_t pixel = tft.readPixel(x, y);

              if (pixel == TFT_BLUE) {
                float a = (float)L;
                float s = a / (float)ROI[5];
                int16_t x_ = s * (float)(x - ROI[0]) + 0.5 * ( a - s * (float)ROI[4]);  // Align to center (60,60)
                int16_t y_ = s * (float)(y - ROI[1]) + 0.5 * ( a - s * (float)ROI[5]);

                if ((x_ >= 0) && (x_ < L) && (y_ >= 0) && (y_ < L)) {
                  tft.fillCircle(x_, y_, 1, TFT_WHITE);
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
      do_convolution();
      normalize_grid();
      area_setup();
      area_setup2();
      predict();
      reset_grid();
    }
  }
}


// Convert to float, normalize (0.0 to 1.0), flatten the grids,
// add 1.0 at the end (for the bias tricks)
float get_x(int16_t i) {
  if (i < 8 * 8)
    return (float)POOLED_GRID[i % 8][i / 8];
  else
    return 1.0;
}


int16_t update_progress(int16_t p) {
  p = p + 1;
  int16_t e = float(p) / (float)(HIDDEN_SIZE + 10) * L;
  tft.drawLine(0, L - 1, e, L - 1, TFT_RED);
  return p;
}


float read_float(File &f) {
  char s[16];
  f.readBytesUntil('\n', (char *)s, sizeof(s));
  return atof(s);
}


// Do the PREDICTION
void predict() {
  int16_t progress = 0;
  float y[HIDDEN_SIZE];  // hidden_size
  float c[10];           // output_size
  float w;
  char s[16];
  File f;

  unsigned long st = micros();  // timer starts

  // Compute the scores: y = x * W (matrix multiplication)
  f = SD.open("W1_CNN.txt");
  for (int16_t i = 0; i < HIDDEN_SIZE; i++) {
    y[i] = 0;
    for (int16_t j = 0; j < (8 * 8 + 1); j++) {
      w = read_float(f);
      y[i] = y[i] + (get_x(j) * w);
    }
    if (y[i] < 0.0)  //ReLU
      y[i] = 0.0;

    progress = update_progress(progress);
  }
  f.close();

  f = SD.open("W2_CNN.txt", FILE_READ);
  for (int16_t i = 0; i < 10; i++) {
    c[i] = 0;
    for (int16_t j = 0; j < HIDDEN_SIZE; j++) {
      w = read_float(f);
      c[i] = c[i] + (y[j] * w);
    }
    w = read_float(f);
    c[i] = c[i] + w;

    progress = update_progress(progress);
  }
  f.close();

  // Find the largest scores
  float max_c = 0;
  int16_t label = 0;
  for (int16_t i = 0; i < 10; i++) {
    if (c[i] > max_c) {
      max_c = c[i];
      label = i;
    }
  }

  float et = (float)(micros() - st) * 1e-6;  // timer stops

  // Display the results
  print_label(label);

  // Display the scores
  tft.setCursor(0, W16 * 9);
  tft.setTextSize(1);
  tft.println("Scores:\n");
  for (int16_t i = 0; i < 10; i++) {
    if (i == label)
      tft.setTextColor(TFT_RED);
    else
      tft.setTextColor(TFT_BLUE);

    tft.print(i);
    tft.print(" : ");
    tft.println(c[i], 4);
  }
  tft.setTextColor(TFT_GREEN);
  tft.print("Time (s) : ");
  tft.println(et, 2);
}
