/*
// - Lien vidéo: https://youtu.be/AD6JaqEASAE
// REQUIRES the following Arduino libraries:
// - SmartMatrix Library: https://github.com/pixelmatix/SmartMatrix
*/
 
#include <MatrixHardware_ESP32_V0.h>                // This file contains multiple ESP32 hardware configurations, edit the file to define GPIOPINOUT (or add #define GPIOPINOUT with a hardcoded number before this #include)
#include <SmartMatrix.h>

#define COLOR_DEPTH 24                  // Choose the color depth used for storing pixels in the layers: 24 or 48 (24 is good for most sketches - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24)
const uint16_t kMatrixWidth = 128;       // Set to the width of your display, must be a multiple of 8
const uint16_t kMatrixHeight = 64;      // Set to the height of your display
const uint8_t kRefreshDepth = 24;       // Tradeoff of color quality vs refresh rate, max brightness, and RAM usage.  36 is typically good, drop down to 24 if you need to.  On Teensy, multiples of 3, up to 48: 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48.  On ESP32: 24, 36, 48
const uint8_t kDmaBufferRows = 2;       // known working: 2-4, use 2 to save RAM, more to keep from dropping frames and automatically lowering refresh rate.  (This isn't used on ESP32, leave as default)
const uint8_t kPanelType = SM_PANELTYPE_HUB75_64ROW_MOD32SCAN;   // Choose the configuration that matches your panels.  See more details in MatrixCommonHub75.h and the docs: https://github.com/pixelmatix/SmartMatrix/wiki
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_NONE);        // see docs for options: https://github.com/pixelmatix/SmartMatrix/wiki
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);


#define UpHeader 0x01
#define DataFrame 0x02
#define endHeader 0x03

uint32_t lastData = 1000;
uint8_t buff[3];

    void drawFrame() {
      if (Serial.read() != UpHeader)
            return;
      if (Serial.read() != DataFrame)
            return;
      int bufferSize = (kMatrixHeight * kMatrixWidth) + 1;
      rgb24 *buffer = backgroundLayer.backBuffer();
      for (int i = 0; i < bufferSize; i++) {
        Serial.readBytes(buff, 3);
        buffer[i] = rgb24 {buff[0], buff[1], buff[2]};
      }
      if (Serial.read() != endHeader)
            return;
      backgroundLayer.swapBuffers();
    }

void setup() {
  Serial.begin(1250000);
  delay(3000);
  matrix.addLayer(&backgroundLayer); 
  matrix.begin();
  backgroundLayer.setBrightness(255);
  //backgroundLayer.enableColorCorrection(true);
}

void loop() {
  if (Serial.available() > 0) {
    switch (Serial.peek()) {
      case UpHeader:
         drawFrame();
         lastData = millis();
         break;
      default:
         Serial.read();
         break;
    }
  }
  else if (millis() - lastData > 3000) {
      backgroundLayer.fillScreen({ 0, 0, 0 });
      backgroundLayer.setFont(font3x5);
      backgroundLayer.drawString(3, 24, { 255, 0, 255 }, "Waiting");
      backgroundLayer.swapBuffers(true);
      lastData = millis();
   }
}
