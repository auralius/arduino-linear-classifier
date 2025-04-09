/**
 * Auralius Manurung -- Universitas Telkom
 * auralius.manurun@ieee.org
 */ 

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;  // hard-wired for UNO shields anyway.
#include <TouchScreen.h>

const int XP = 6, XM = A2, YP = A1, YM = 7;  //ID=0x9341
const int TS_LEFT = 907, TS_RT = 136, TS_TOP = 942, TS_BOT = 139;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

#define MINPRESSURE 100
#define MAXPRESSURE 1000

int16_t ROI[6];  //xmin, ymin, xmax, ymax, width, length

int16_t W2, W8, W86; // W2: width/2, W8: width/8, W86: width/8/6
int16_t PENRADIUS = 5;
uint16_t ID, oldcolor, currentcolor;

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

uint16_t GREYS[17];

// The W matrix computed by SVM of Softmax
float W[65 * 10] = { -7.5004e-04, 1.6986e-03, 2.2155e-04, 1.8328e-03, 7.7484e-04, -6.7503e-04, 1.3654e-03, 1.5934e-03, 3.1074e-04, 1.0143e-03,
                     -2.4115e-03, -1.2909e-02, 1.5399e-02, 2.0009e-02, -4.0672e-03, 2.3883e-03, -7.0563e-03, 1.4063e-02, -3.4012e-03, 1.3305e-03,
                     1.0382e-02, 6.5386e-03, 1.2305e-01, 1.0567e-01, 3.7379e-02, 1.3721e-01, -4.5026e-02, 9.4356e-02, -2.9856e-02, 7.4872e-02,
                     1.0608e-01, 6.8019e-02, 1.6955e-01, 1.4183e-01, -4.6786e-03, 1.5031e-01, 5.2095e-02, 1.1952e-01, 1.2193e-01, 1.6996e-01,
                     7.7283e-02, -1.0106e-02, 8.6136e-02, 1.8982e-01, 4.0474e-02, 2.2489e-01, 6.1945e-02, 1.8223e-01, 1.1131e-01, 1.3997e-01,
                     -6.3304e-03, 3.1118e-02, -1.0074e-02, 8.1167e-02, -7.8412e-02, 2.7084e-01, -1.3214e-02, 1.7137e-01, 3.5439e-02, 5.4369e-02,
                     -1.0029e-02, -1.4179e-02, 2.9188e-03, 4.1795e-03, -5.9551e-03, 1.1343e-01, -1.2091e-02, 8.4182e-02, -4.9487e-02, 2.0471e-02,
                     -1.0570e-03, -5.8446e-03, -2.0060e-03, -9.1694e-03, 2.1065e-02, 2.0615e-02, -1.9243e-04, 2.0684e-02, -6.6027e-03, -2.5421e-02,
                     -2.9443e-04, -5.0363e-03, -3.6746e-04, -1.3468e-03, -4.9913e-04, 9.4163e-04, 3.1177e-04, -4.5628e-04, 7.0753e-03, -1.7102e-05,
                     -9.2329e-03, -6.3973e-02, 7.9123e-02, 8.8163e-02, -5.7603e-03, 3.0492e-02, -2.8351e-02, 2.8387e-02, 2.7777e-02, 5.2245e-02,
                     1.1656e-01, -7.0426e-02, 2.1227e-01, 1.7958e-01, -3.0347e-02, 1.8949e-01, 2.7200e-02, 1.0191e-01, 1.2697e-01, 1.7029e-01,
                     1.4279e-01, 1.0894e-01, 1.4294e-01, 7.8010e-02, 6.5960e-02, 2.0802e-01, 1.1380e-01, 1.6221e-01, 3.6263e-02, 1.1746e-01,
                     1.5507e-01, 1.5085e-01, 1.4979e-01, 1.2886e-01, -2.8259e-02, 1.2279e-01, -5.6411e-03, 2.2332e-01, 2.8783e-02, 1.4737e-01,
                     1.2617e-01, 7.5959e-02, 4.6734e-02, 1.3387e-01, -5.9883e-02, 8.0915e-02, -4.7767e-02, 1.4810e-01, 1.4964e-01, 1.5582e-01,
                     -2.7485e-03, -3.7442e-03, 2.2531e-03, 4.3451e-02, 9.4766e-02, 9.1246e-03, -1.7007e-02, 5.3176e-02, -1.9298e-02, 5.1820e-02,
                     -2.6268e-03, -7.6660e-03, -2.0513e-03, 5.1587e-03, 1.9269e-02, 3.1768e-03, -1.1385e-03, 1.5892e-02, -5.0089e-03, -6.4275e-03,
                     -1.9488e-05, 1.1910e-03, -9.4955e-04, -1.2797e-04, -2.5237e-05, 2.9936e-04, -3.4061e-05, 4.1250e-04, -2.3005e-03, 6.1234e-04,
                     3.8930e-02, 1.2778e-02, 4.2873e-02, 4.8118e-03, 1.5873e-02, 4.0446e-02, -2.6402e-02, -2.6283e-02, 9.1889e-02, 4.6895e-02,
                     1.7384e-01, 9.7803e-02, 2.8704e-02, -1.0554e-01, 1.3129e-01, 2.2660e-01, 1.3901e-01, -9.1065e-02, 1.6910e-01, 1.8024e-01,
                     3.3405e-02, 3.1681e-01, -6.1303e-02, -7.2593e-02, 2.1269e-01, 1.2413e-01, 6.6028e-02, -8.4927e-02, 7.7297e-02, 9.2436e-02,
                     -7.6617e-03, 2.6470e-01, 1.1450e-01, 1.3526e-01, 6.0994e-02, -2.9134e-02, -6.7025e-02, 9.1396e-02, 4.5732e-02, 1.3642e-01,
                     1.8890e-01, 6.8303e-02, 8.0212e-02, 6.5625e-02, 8.0600e-02, -1.5655e-01, -9.0858e-02, 1.1202e-01, 1.7863e-01, 2.5434e-01,
                     5.4880e-02, -1.4729e-02, 1.5654e-02, 1.1468e-03, 5.9301e-02, -8.2760e-02, -1.8221e-02, 4.1243e-02, 3.7242e-02, 1.0240e-01,
                     -3.6858e-04, -2.1545e-03, -1.9706e-03, -1.0248e-04, 1.3274e-03, -1.8836e-03, -1.0202e-03, 4.4510e-03, 5.2727e-04, 6.8264e-03,
                     -1.8500e-03, 1.7270e-03, -1.4271e-03, -6.6949e-05, 1.3215e-05, -5.8098e-04, -4.6761e-04, -2.6137e-03, 1.7635e-03, -1.1756e-03,
                     8.9169e-02, 4.2016e-02, -7.1519e-03, -2.8659e-02, 7.9799e-02, 6.8587e-02, 1.8659e-02, -2.4875e-02, -2.6285e-03, 3.5944e-03,
                     1.6873e-01, 1.3301e-01, -9.0367e-02, -9.1392e-02, 2.4814e-01, 2.0296e-01, 1.6164e-01, -3.6889e-02, 9.7382e-02, 1.4993e-01,
                     -6.1775e-02, 1.9005e-01, -7.9954e-02, 1.1804e-01, 2.2365e-01, 1.3528e-01, 3.6948e-02, 4.1830e-02, 2.2728e-01, 1.2319e-01,
                     -9.7399e-02, 2.0513e-01, 1.0415e-01, 1.9765e-01, 6.9166e-02, 4.8573e-02, 1.0635e-02, 1.5837e-01, 1.6357e-01, 1.5856e-01,
                     1.0863e-01, 1.0608e-01, 4.5261e-02, -5.0107e-02, 1.7881e-01, 3.5218e-02, -5.4975e-02, 1.5621e-01, 5.9556e-02, 2.0765e-01,
                     1.2293e-01, -3.0643e-02, -1.6515e-02, -5.9453e-02, 1.5154e-01, -2.8425e-02, -3.1519e-02, 1.0525e-01, -1.5350e-02, 4.1478e-02,
                     -3.2035e-04, -1.1201e-03, 2.0767e-03, 1.2072e-04, 1.4736e-03, -7.8978e-04, 6.4131e-04, -9.6172e-04, 1.8192e-03, -4.7529e-04,
                     6.7789e-04, -1.0811e-03, -1.1235e-03, 9.7715e-05, 3.2782e-04, 9.1449e-04, 7.5524e-04, -1.4095e-03, 1.6724e-03, 2.2185e-04,
                     9.5977e-02, -5.4090e-03, -1.7952e-02, -1.6911e-02, 1.2175e-01, 1.7093e-02, 5.1968e-02, 2.1612e-02, -3.7023e-02, -1.1218e-02,
                     1.8334e-01, 5.6709e-02, -4.6088e-02, -3.0075e-02, 2.0873e-01, 3.3690e-02, 2.0234e-01, 1.2143e-01, 5.9990e-02, 3.5191e-02,
                     -5.9112e-02, 1.0385e-01, 2.4407e-02, 1.2361e-01, 8.6870e-02, 3.9713e-02, 1.7468e-01, 2.1021e-01, 2.0604e-01, 8.9849e-02,
                     -8.6329e-02, 1.6229e-01, 8.3392e-02, 1.9988e-01, 1.7609e-01, 7.9260e-03, 1.5458e-01, 2.1783e-01, 1.4612e-01, 7.1574e-02,
                     7.5709e-02, 6.5185e-02, -3.6773e-02, 1.3653e-01, 2.2664e-01, 6.2948e-02, 1.0292e-01, 2.1123e-01, -2.1665e-02, 1.4075e-01,
                     1.2383e-01, -4.8017e-02, -5.9672e-02, 1.2279e-02, 1.1954e-01, 6.4079e-02, 9.7995e-03, 1.1631e-01, -5.4540e-02, 9.4076e-03,
                     5.4472e-04, -1.6939e-04, -2.2481e-04, -3.8697e-04, 3.4479e-04, 2.2035e-05, -1.4647e-03, 1.2948e-03, -7.7834e-04, 6.2734e-04,
                     -8.0295e-04, -2.7104e-03, -1.5069e-03, -3.3103e-04, 7.3957e-03, 1.6855e-03, -1.7372e-03, 5.1816e-04, -4.6289e-04, -1.3502e-03,
                     4.5233e-02, -3.8640e-02, 1.3000e-03, -1.6672e-02, 1.6621e-01, -3.0127e-02, 5.9687e-03, 1.0376e-02, 2.1045e-02, -1.6420e-02,
                     2.3402e-01, 2.7983e-02, 9.7893e-02, -1.0347e-01, 1.1677e-01, -1.2801e-01, 2.6959e-01, 3.4779e-02, 2.4033e-01, -1.0512e-01,
                     1.0368e-03, 1.7410e-01, 1.7952e-01, -1.0213e-01, 2.1543e-01, -4.5585e-02, 2.0748e-01, 1.6951e-01, 8.7778e-02, -9.4464e-02,
                     -2.2234e-02, 1.7267e-01, 2.9403e-02, 4.8893e-02, 2.6449e-01, 2.2370e-02, 8.4672e-02, 1.2027e-01, 1.3295e-01, 1.1174e-02,
                     1.4603e-01, 2.3347e-02, -5.9284e-02, 2.0965e-01, 1.1726e-01, 9.0941e-02, 1.4157e-01, 2.2134e-02, 1.2138e-01, 9.4344e-02,
                     7.9044e-02, -3.4970e-02, -9.1528e-02, 1.2966e-01, 4.1358e-03, 7.4559e-02, 1.5525e-01, 2.6132e-03, 2.3495e-02, 1.7299e-02,
                     8.3887e-04, -5.1531e-04, -7.8073e-04, -3.7786e-05, 9.4373e-04, 2.4758e-03, 3.2156e-03, -2.0116e-04, -1.6876e-03, -1.3621e-03,
                     1.1513e-03, -1.2707e-04, 1.0521e-03, -6.0876e-04, 6.0042e-03, 1.6363e-03, -6.8412e-04, 1.8836e-04, -1.4702e-03, -2.5051e-03,
                     -7.0302e-03, -2.9368e-02, 5.1488e-02, -5.8117e-04, 1.0042e-01, -2.4517e-03, -4.4983e-02, -9.3515e-03, 1.6946e-02, 7.6951e-03,
                     1.6147e-01, 2.4034e-02, 1.6824e-01, 5.1552e-02, 4.6490e-02, 5.7130e-02, 1.2940e-01, -6.1301e-04, 1.6722e-01, 1.0846e-02,
                     1.3065e-01, 1.2143e-01, 2.4287e-01, 5.5102e-02, 1.3046e-01, 6.0846e-02, 1.7218e-01, 1.3486e-01, 7.9429e-03, -1.1525e-02,
                     1.4500e-01, 1.8238e-01, 1.9945e-01, 1.1441e-01, 1.3375e-01, 1.3624e-01, 3.4863e-02, -2.3978e-03, 7.5573e-02, 7.7719e-03,
                     1.7004e-01, 7.2393e-02, 1.5605e-01, 2.4403e-01, 1.2667e-02, 8.7535e-02, 1.5435e-01, -8.2979e-02, 1.3959e-01, -2.4853e-03,
                     -1.3872e-02, -1.9579e-03, 8.8209e-02, 6.3059e-02, 6.1466e-03, 2.7010e-02, 1.6725e-01, -2.3038e-02, 5.0799e-02, 2.0877e-02,
                     -1.5171e-03, 1.2349e-02, 4.8610e-03, -4.5449e-03, 2.4963e-03, -6.2524e-04, 1.5230e-02, -1.5695e-03, -2.4992e-03, -2.3988e-03,
                     1.9441e-03, -7.2051e-04, 7.4223e-04, -7.4912e-04, -1.1640e-03, 1.4304e-04, -9.1489e-04, 1.1930e-03, -2.1137e-03, -6.1142e-04,
                     -3.3863e-03, -1.7518e-03, 2.3232e-02, 2.1173e-02, 8.2790e-03, 9.4281e-05, -9.4525e-03, 8.0805e-03, -1.8509e-02, 7.6944e-04,
                     1.3707e-02, 6.3786e-03, 1.4540e-01, 1.5576e-01, 4.2908e-02, 1.4012e-01, -3.3096e-02, 9.1578e-02, -2.7572e-02, 6.2702e-02,
                     1.3655e-01, 4.9070e-02, 1.8345e-01, 1.9418e-01, -3.8112e-02, 2.1437e-01, 7.7090e-02, 4.6778e-02, 1.5791e-01, 1.5782e-01,
                     1.4064e-01, 1.0313e-01, 1.7267e-01, 1.5771e-01, 6.0882e-02, 1.1288e-01, 1.9056e-01, -8.6464e-02, 1.5434e-01, 1.3287e-01,
                     1.3145e-02, 1.6649e-01, 2.0584e-01, 5.3507e-02, 1.2650e-02, 1.2596e-02, 2.0186e-01, -5.0844e-02, 3.8738e-02, 1.2405e-02,
                     -3.1642e-02, 8.3718e-02, 1.7867e-01, -3.6387e-02, 3.8792e-03, -1.2758e-03, 6.6211e-02, -1.8004e-02, 2.2117e-03, -2.8570e-02,
                     -4.8152e-03, 4.4371e-02, 2.4252e-02, -1.5928e-02, 6.0644e-03, -7.0936e-03, -6.6536e-03, -2.0885e-03, -2.3916e-03, -1.2427e-02,
                     8.1631e-03, 2.5212e-03, 1.1335e-02, 1.0080e-02, 1.5500e-02, 1.2233e-02, 8.6945e-03, 1.0324e-02, 5.6079e-03, 8.4603e-03 };

// Grayscale of 17 steps from blak (0x0000) to white(0xFFFF)
// #15 and #16 are both (0xFFFF) for simplification reason since 17 is an odd number
void create_greys() {
  for (byte k = 0; k < 16; k++) {
    GREYS[k] = ((2 * k << 11) | (4 * k << 5) | 2 * k);
  }
  GREYS[16] = 0xFFFF;
}

// This comes with the standard demo of the touchscreen
void show_Serial(void) {
  Serial.println(F("Most Touch Screens use pins 6, 7, A1, A2"));
  Serial.println(F("But they can be in ANY order"));
  Serial.println(F("e.g. right to left or bottom to top"));
  Serial.println(F("or wrong direction"));
  Serial.println(F("Edit name and calibration statements\n"));
  Serial.println("");
  Serial.print(F("ID=0x"));
  Serial.println(ID, HEX);
  Serial.println("Screen is " + String(tft.width()) + "x" + String(tft.height()));
  Serial.println("Calibration is: ");
  Serial.println("LEFT = " + String(TS_LEFT) + " RT  = " + String(TS_RT));
  Serial.println("TOP  = " + String(TS_TOP) + " BOT = " + String(TS_BOT));
  Serial.println("Wiring is always PORTRAIT");
  Serial.println("YP=" + String(YP) + " XM=" + String(XM));
  Serial.println("YM=" + String(YM) + " XP=" + String(XP));
}

void show_tft(void) {
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.print(F("ID=0x"));
  tft.println(ID, HEX);
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
}

// Normalize the numbers in the grids such that they range from 0 to 16
void normalize_grid() {
  float maxval = 0.0;
  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++)
      if (GRID[i][j] > maxval)
        maxval = GRID[i][j];

  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++){
      GRID[i][j] = (float)GRID[i][j] / maxval * 16.0 + 0.5; // round instead of floor!
      if (GRID[i][j] > 16)
        GRID[i][j] = 16;
    }
}

// Draw the grids in the screen
// The fill-color or the gray-level of each grid is set based on its value
void area_setup() {
  for (byte i = 0; i < 8; i++) {
    for (byte j = 0; j < 8; j++) {
      tft.fillRect(i * W8, j * W8, W8, W8, GREYS[GRID[i][j]]);
      tft.drawRect(i * W8, j * W8, W8, W8, CYAN);
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
}

// Draw the ROI as a red rectangle
void draw_roi() {
  tft.drawRect(ROI[0], ROI[1], ROI[4], ROI[5], RED);
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

// Put text at the bottom of the scrre, right above the two buttons
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
  ID = tft.readID();
  tft.begin(ID);
  Serial.begin(9600);
  show_Serial();
  tft.setRotation(0);
  tft.fillScreen(BLACK);
  show_tft();

  tft.fillScreen(BLACK);

  W8 = tft.width() / 8;
  W2 = tft.width() / 2;
  W86 = W8 / 6;

  reset_grid();
  area_setup();

  draw_buttons("CLEAR", "IDENTIFY");

  currentcolor = BLUE;
  delay(1000);

  //for (byte i=0; i<8; i++)
  //  for (byte j=0; j<8; j++)
  //    GRID[i][j] = NINE[i+j*8];
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
    if (((ypos - PENRADIUS) > 0) && ((ypos + PENRADIUS) < W8 * 8)) {
      tft.fillCircle(xpos, ypos, PENRADIUS, currentcolor);
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
      for (byte i = 0; i < 8; i++) {
        for (byte j = 0; j < 8; j++) {
          byte counter = 0;
          for (byte k = 0; k < 6; k++) {
            for (byte l = 0; l < 6; l++) {
              uint16_t x = i * W8 + k * W86;
              uint16_t y = j * W8 + l * W86;

              uint16_t pixel = tft.readPixel(x, y);
              if (pixel == BLUE){
                counter++;
                tft.drawCircle(x, y, 1, RED);
              }
            }
          }
          GRID[i][j] = counter;
        }
      }
      delay(1000);
      normalize_grid();
      area_setup();
      draw_roi();
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
      y[i] = y[i] + (x[j] * W[i + j * 10]);
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
