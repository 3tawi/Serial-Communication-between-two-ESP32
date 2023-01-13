/*
// - Lien vidéo: https://youtu.be/iPlEjbkxA2w
// REQUIRES the following Arduino libraries:
// - AnimatedGIF Library:  https://github.com/bitbank2/AnimatedGIF
*/
 
#include <SD.h>
#include <GifDecoder.h>
#include "FilenameFunctions.h"

#define kMatrixWidth  128
#define kMatrixHeight  64
#define DISPLAY_TIME_SECONDS 60000
#define DataFrame 0x02
#define ClearHeader 0xaa
#define UpHeader 0x08
#define endHeader 0x04
#define SD_CS 5
#define GIF_DIRECTORY "/gifs"
Stream* mySeriel;
uint8_t buff[5];

GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;

void setDriver(Stream* s) {
  mySeriel = s;
}

void sendData(uint8_t* data)
{
  mySeriel->write(DataFrame);
  mySeriel->write(data, 5);
  mySeriel->write(endHeader);
}


int num_files;

void screenClearCallback(void) {
  mySeriel->write(ClearHeader);
  mySeriel->write(endHeader);
}

void updateScreenCallback(void) {
  mySeriel->write(UpHeader);
  mySeriel->write(endHeader);
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  buff[0] = x;
  buff[1] = y;
  buff[2] = red;
  buff[3] = green;
  buff[4] = blue;
  sendData(buff);
}

// Setup method runs once, when the sketch starts
void setup() {
    Serial.begin(1250000);
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
    static unsigned long displayStartTime_millis;
    static int nextGIF = 1;     // we haven't loaded a GIF yet on first pass through, make sure we do that

    unsigned long now = millis();

    static int index = 0;
    // default behavior is to play the gif for DISPLAY_TIME_SECONDS or for NUMBER_FULL_CYCLES, whichever comes first
    if((millis() - displayStartTime_millis) > DISPLAY_TIME_SECONDS)
        nextGIF = 1;

    if(nextGIF)
    {
        delay(3000);
        nextGIF = 0;

        if (openGifFilenameByIndex(GIF_DIRECTORY, index) >= 0) {
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
        if (++index >= num_files) {
            index = 0;
        }

    }

    if(decoder.decodeFrame() < 0) {
        // There's an error with this GIF, go to the next one
        nextGIF = 1;
    }
  //delay(3000);
}
