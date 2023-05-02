#include "RoachUsbMsd.h"

#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"

FatFile root;
FatFile file;

void setup()
{
  RoachUsbMsd_begin();
}

void loop()
{
  RoachUsbMsd_task();
  if (RoachUsbMsd_hasChange(true))
  {
    Serial.println("Opening root");

    if ( !root.open("/") )
    {
      Serial.println("open root failed");
      return;
    }

    Serial.println("Flash contents:");

    while ( file.openNext(&root, O_RDONLY) )
    {
      file.printFileSize(&Serial);
      Serial.write(' ');
      file.printName(&Serial);
      if ( file.isDir() )
      {
        // Indicate a directory.
        Serial.write('/');
      }
      Serial.println();
      file.close();
    }

    root.close();

    Serial.println();
  }
}
