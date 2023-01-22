/*
// - Lien vidéo: https://youtu.be/AD6JaqEASAE
// REQUIRES the following Arduino libraries:
// - AnimatedGIF Library:  https://github.com/bitbank2/AnimatedGIF
*/

#define FILESYSTEM SD
#include <SD.h>
#include <AnimatedGIF.h>

AnimatedGIF gif;
File f;
Stream* mySeriel;

#define UpHeader 0x01
#define DataFrame 0x02
#define endHeader 0x03

const uint16_t DISPLAY_WIDTH = 128;
const uint16_t kMatrixHeight = 64; 
const uint16_t NUM_LEDS = DISPLAY_WIDTH * kMatrixHeight + 1;

typedef struct rgb24 {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} rgb24;

rgb24 buff[NUM_LEDS];


void setDriver(Stream* s) {
  mySeriel = s;
}

void updateScreenCallback() {
  mySeriel->write(UpHeader);
  mySeriel->write(DataFrame);
  mySeriel->write((char *)buff, NUM_LEDS*3);
  mySeriel->write(endHeader);
}

uint16_t XY(uint8_t x, uint8_t y) {
  return (y * DISPLAY_WIDTH) + x;
}

void DrawPixelRow(int startX, int y, int numPixels, rgb24 * color) {
  for(int i=0; i<numPixels; i++)
  {
    buff[XY(startX + i, y)] = color[i];
  }
}

// Draw a line of image directly on the LED Matrix
void GIFDraw(GIFDRAW *pDraw){
  uint8_t *s;
  //uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;
  rgb24 *d, *usPalette, usTemp[128];

  iWidth = pDraw->iWidth;
  if (iWidth > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH;
  usPalette = (rgb24*)pDraw->pPalette;

  y = pDraw->iY + pDraw->y; // current line
  
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x=0; x<iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }
  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while(x < iWidth)
    {
      c = ucTransparent-1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        DrawPixelRow(pDraw->iX+x, y, iCount, (rgb24 *)usTemp);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--; 
      }
      if (iCount)
      {
        x += iCount; // skip these
        iCount = 0;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x=0; x<iWidth; x++)
      usTemp[x] = usPalette[*s++];
      DrawPixelRow(pDraw->iX, y, iWidth, (rgb24 *)usTemp);
  }
} /* GIFDraw() */


void * GIFOpenFile(const char *fname, int32_t *pSize)
{
  //Serial.print("Playing gif: ");
  //Serial.println(fname);
  f = FILESYSTEM.open(fname);
  if (f)
  {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
} /* GIFOpenFile() */

void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
     f->close();
} /* GIFCloseFile() */

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0)
       return 0;
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    return iBytesRead;
} /* GIFReadFile() */

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{ 
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
//  Serial.printf("Seek time = %d us\n", i);
  return pFile->iPos;
} /* GIFSeekFile() */

unsigned long start_tick = 0;

void ShowGIF(char *name)
{
  start_tick = millis();
   
  if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    while (gif.playFrame(true, NULL))
    {
      if(gif.getLastError() == GIF_SUCCESS) {
        updateScreenCallback();
      }  
      else if ( (millis() - start_tick) > 60000) { // we'll get bored after about 8 seconds of the same looping gif
        break;
      }
    }
    gif.close();
  }

} /* ShowGIF() */


void setup() {
  Serial.begin(1250000);
  delay(5000);
  setDriver(&Serial);
  Serial.println("Starting AnimatedGIFs Sketch");

  // Start filesystem
  Serial.println(" * Loading SD");
  if(!SD.begin(5)){
        Serial.println("SD Mount Failed");
  }
  gif.begin(BIG_ENDIAN_PIXELS, GIF_PALETTE_RGB888);
}

String gifDir = "/gifs"; // play all GIFs in this directory on the SD card
char filePath[256] = { 0 };
File root, gifFile;

void loop() 
{  
      root = FILESYSTEM.open(gifDir);
      if (root) {
        gifFile = root.openNextFile();
        while (gifFile) {
          memset(filePath, 0x0, sizeof(filePath));                
          strcpy(filePath, gifFile.name());
          ShowGIF(filePath);
          gifFile.close();
          gifFile = root.openNextFile();
          }
         root.close();
      } // root
      
      delay(4000); // pause before restarting
      
}
