/*
// - Lien vidéo: https://youtu.be/AD6JaqEASAE
// REQUIRES the following Arduino libraries:
// - AnimatedGIF Library:  https://github.com/bitbank2/AnimatedGIF
*/
 
#include "MatrixCommon.h"
#include <SD.h>
#include <GifDecoder.h>
#include "FilenameFunctions.h"

const uint16_t kMatrixWidth = 128;
const uint16_t kMatrixHeight = 64; 
#define DISPLAY_TIME_SECONDS 60000

#define UpHeader 0x01
#define DataFrame 0x02
#define endHeader 0x03

#define SD_CS 5
#define GIF_DIRECTORY "/gifs"
int num_files;
Stream* mySeriel;
unsigned long displayStartTime_millis;
int nextGIF = 1;   
int inde = 0;

GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;
const uint16_t NUM_LEDS = kMatrixWidth * kMatrixHeight + 1;
rgb24 rgb[NUM_LEDS];


// translates from x, y into an index into the LED array
uint16_t XY(uint8_t x, uint8_t y) {
  return (y * kMatrixWidth) + x;
}

void setDriver(Stream* s) {
  mySeriel = s;
}

void screenClearCallback(void) {
  delay(6000);
}

void updateScreenCallback(void) {
  mySeriel->write(UpHeader);
  mySeriel->write(DataFrame);
  mySeriel->write((char *)rgb, NUM_LEDS*3);
  mySeriel->write(endHeader);
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  rgb[XY(x, y)] = rgb24 {red, green, blue};
}

// Setup method runs once, when the sketch starts
void setup() {
    Serial.begin(1350000);
    delay(5000);
    setDriver(&Serial);
    decoder.setScreenClearCallback(screenClearCallback);
    decoder.setUpdateScreenCallback(updateScreenCallback);
    decoder.setDrawPixelCallback(drawPixelCallback);

    decoder.setFileSeekCallback(fileSeekCallback);
    decoder.setFilePositionCallback(filePositionCallback);
    decoder.setFileReadCallback(fileReadCallback);
    decoder.setFileReadBlockCallback(fileReadBlockCallback);
    
    // NOTE: new callback function required after we moved to using the external AnimatedGIF library to decode GIFs
    decoder.setFileSizeCallback(fileSizeCallback);
    
    if(initFileSystem(SD_CS) < 0) {
        Serial.println("No SD card");
        while(1);
    }

    // Determine how many animated GIF files exist
    num_files = enumerateGIFFiles(GIF_DIRECTORY, true);

    if(num_files < 0) {
        Serial.println("No gifs directory");
        while(1);
    }

    if(!num_files) {
        Serial.println("Empty gifs directory");
        while(1);
    }

}

void loop() {
    
    if((millis() - displayStartTime_millis) > DISPLAY_TIME_SECONDS)
        nextGIF = 1;
        
    if(nextGIF) {
        nextGIF = 0;
        if (openGifFilenameByIndex(GIF_DIRECTORY, inde) >= 0) {
            // Can clear screen for new animation here, but this might cause flicker with short animations
            // matrix.fillScreen(COLOR_BLACK);
            // matrix.swapBuffers();

            // start decoding, skipping to the next GIF if there's an error
            if(decoder.startDecoding() < 0) {
                nextGIF = 1;
                return;
            }

            // Calculate time in the future to terminate animation
            displayStartTime_millis = millis();
        }

        // get the index for the next pass through
        if (++inde >= num_files) {
            inde = 0;
        }

    }

    if(decoder.decodeFrame() < 0) {
        // There's an error with this GIF, go to the next one
        nextGIF = 1;
    }
}
