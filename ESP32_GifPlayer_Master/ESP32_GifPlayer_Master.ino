/*
// - Lien vidéo: https://youtu.be/XJcpp101V18
// REQUIRES the following Arduino libraries:
// - AnimatedGIF Library:  https://github.com/bitbank2/AnimatedGIF
*/
#include <SD.h>
#include <AnimatedGIF.h> 

const uint16_t ImageWidth = 128; 
const uint16_t ImageHeight = 64;

AnimatedGIF gif;
File f;
Stream* mySeriel;

#define DataFrame 0xda
#define NextFrame 0x9c
#define EndFrame 0xad

const uint16_t NUM_LEDS = ImageWidth * ImageHeight;
const uint16_t NUM_Palette = 255 * 2;
const uint16_t sizeofData = NUM_LEDS + NUM_Palette;
static uint8_t buff[sizeofData];


void setDriver(Stream* s) {
  mySeriel = s;
}

void updateScreenCallback() {
  mySeriel->write(DataFrame);
  mySeriel->write((uint8_t *)buff, sizeofData);
  mySeriel->write(EndFrame);
  delay(1);
  memset(buff, 0x0, NUM_LEDS);
  while (mySeriel->read() != NextFrame){}
}

// Draw a line of image directly on the LED Matrix
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  int x, y, xy, iWidth;

  iWidth = pDraw->iWidth;
  y = pDraw->iY + pDraw->y; // current line
  x = pDraw->iX;
  
  if (iWidth + x > ImageWidth)
    iWidth = ImageWidth - x;
  if (y >= ImageHeight || x >= ImageWidth || iWidth < 1)
       return; 
  
  memcpy((buff+NUM_LEDS), (uint8_t *)pDraw->pPalette, NUM_Palette);
  
  s = pDraw->pPixels;
  for (int i=0; i<iWidth; i++)
    s[i] =(s[i] != 0xFF) ? (s[i] + 1) : 0;
    
  xy = (ImageWidth * y) + x;
  memcpy((buff+xy), s, iWidth);
} /* GIFDraw() */

void * GIFOpenFile(const char *fname, int32_t *pSize)
{
  //Serial.print("Playing gif: ");
  //Serial.println(fname);
  f = SD.open(fname);
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


void ShowGIF(char *name)
{
  long lTime;
  if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
      while (gif.playFrame(false, NULL)) { 
        updateScreenCallback();
      }
    gif.close();
  }
} /* ShowGIF() */


void setup() {
  setDriver(&Serial2);
  Serial2.begin(1500000, SERIAL_8E2);
  delay(5000);
  SD.begin(5);
  gif.begin(LITTLE_ENDIAN_PIXELS);
  //gif.begin(LITTLE_ENDIAN_PIXELS);
  //gif.begin(LITTLE_ENDIAN_PIXELS);
  //gif.begin(BIG_ENDIAN_PIXELS, GIF_PALETTE_RGB888);
}


void loop() 
{  
String gifDir = "/gifs"; // play all GIFs in this directory on the SD card
char filePath[256] = { 0 };
File root, gifFile;
      root = SD.open(gifDir);
      if (root)
      {
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
