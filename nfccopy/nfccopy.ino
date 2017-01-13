#include <SD.h>
#include <Adafruit_PN532.h>

Adafruit_PN532 nfc(2, 3);
File myfile;
char filename[8] = "test";
char path[16] = "amiibo";
uint8_t success;
uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
uint8_t dataBuffer[32];
char rorw;

void setup(void) {
  Serial.begin(9600);
  //Serial.println("set baud=115200");
  //Serial.end();
  //Serial.begin(115200);

  nfc.begin();

  // configure NFC and SD reader
  if (!nfc.getFirmwareVersion()) {
    Serial.println("NFC Fail");
    while (1); // halt
  }
  nfc.SAMConfig();
  if (!SD.begin(4)) {
    Serial.println("SD Fail");
    while (1); // halt
  }
  // Make amiibo directory
  if (!SD.exists(path)) {
    SD.mkdir(path);
  }
  
  Serial.println("Ready");
}

void loop(void) {
  
  Serial.println("r or w?");
  Serial.flush();
  while (!Serial.available());
  while (Serial.available()) {
    rorw = Serial.read();
  }
  // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (success) {
    Serial.print("UID: ");
    nfc.PrintHex(uid, uidLength);
    
    if (uidLength == 7)
    {
      // NTAG2xx cards have 39*4 bytes of user pages (156 user bytes),
      // starting at page 4 ... larger cards just add pages to the end of
      // this range:
      
      // See: http://www.nxp.com/documents/short_data_sheet/NTAG203_SDS.pdf
  
      // TAG Type       PAGES   USER START    USER STOP
      // --------       -----   ----------    ---------
      // NTAG 203       42      4             39
      // NTAG 213       45      4             39
      // NTAG 215       135     4             129
      // NTAG 216       231     4             225      

      if (rorw == 'w')
      {
        // TODO:  Make it work with the path.
        myfile = SD.open(filename, FILE_READ);
        myfile.seek(0);
        Serial.print("Reading from ");
        Serial.println(filename);
        for (uint8_t i = 0; i < 135; i++) 
        {
          //Read page from file
          myfile.read(dataBuffer, 4);
          //Write page to tag
          //First 4 pages are write protected.  Need special function.
          
          success = nfc.ntag2xx_WritePage(i, dataBuffer);
          if (success) {
            //nfc.PrintHexChar(dataBuffer, 4);
          }
          else 
          {
            Serial.println("Write Fail");
          }
        }//EndFor
        
        myfile.close();
      } //EndWrite
      else if (rorw == 'r')
      {
        // TODO:  Make it work with the path.
        SD.remove(filename);
        myfile = SD.open(filename, FILE_WRITE);
        myfile.seek(0);
        Serial.print("Saving to ");
        Serial.println(filename);
        // 
        nfc.ntag2xx_ReadPage(0, dataBuffer); // Sometimes, the first read is too fast :)
        for (uint8_t i = 0; i < 135; i++) 
        {
          success = nfc.ntag2xx_ReadPage(i, dataBuffer);
          if (success) 
          {
            // Dump the page data
            //nfc.PrintHexChar(dataBuffer, 4);
            myfile.write(dataBuffer, 4);
          }
          else
          {
            Serial.println("Read Fail");
          }
        }//EndFor
    
        //Print file contents for confirmation
        //myfile.flush();
        //myfile.seek(0);
        //Serial.println("");
        //while(myfile.available()){
        //  myfile.read(dataBuffer, 4);
        //  nfc.PrintHexChar(dataBuffer, 4);
        //}
        //EndPrintFile
        
        myfile.close();
      } //EndRead
    }
    // Wait a bit before trying again
    Serial.println("Complete!\n");
  }
  else
  {
    Serial.println("Tag Fail");
  }
}

