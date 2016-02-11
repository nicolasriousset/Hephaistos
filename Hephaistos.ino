#include "Wire.h"
#include <DHT.h>
#include <SPI.h>
#include "U8glib.h"

// #define USE_SD 1
//#if defined(USE_SD)
#include <SD.h>
//#endif
#include <elapsedMillis.h>

  DHT dht;

struct
{
  int8_t dhtPort;
  DHT::DHT_MODEL_t dhtType;
  float humidity;
  float temperature;  
} sensorsData[] = {
  { 2, DHT::DHT22, 0, 0 },
  { 3, DHT::DHT11, 0, 0 }
};

struct 
{
  byte second;
  byte minute;
  byte hour;
  byte dayOfWeek;
  byte dayOfMonth; 
  byte month; 
  byte year;
} lastReadTime;

char lastReadTimeStr[20] = { 0 };

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);	// I2C / TWI 

#define DS3231_I2C_ADDRESS 0x68
#define SENSOR_POLLING_INTERVAL 500

// set up variables using the SD utility library functions:
//#if defined(USE_SD)
// Sd2Card card;
// SdVolume volume;
// SdFile root;
File dataFile;
//s#endif

String rcvdBuf;
elapsedMillis timeElapsed;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
const int chipSelect = 10;

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void Log(const char* csvLine) {
//#if defined(USE_SD)

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(csvLine);
    dataFile.flush();
    // dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("Log file not open");
  }
//#endif
}

void setupLCD() {
  Serial.println("Setting up LCD");
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }  
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);

  // set the initial time here:
  // DS3231 seconds, minutes, hours, day, date, month, year
  // setDS3231time(00,7,22,1,28,12,15);

//#if defined(USE_SD)  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
  // if (!card.init(SPI_HALF_SPEED, chipSelect)) {
   Serial.println("SD Card failure");
  }

/*  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
  }*/
  
  // processDirCommand();
  
//#endif
  Serial.println("Read date time");
  readDateTime();
  char filename[15] = {0};  
  sprintf(filename, "%04d%02d%02d.csv", (int)lastReadTime.year + 2000, (int)lastReadTime.month, (int)lastReadTime.dayOfMonth);
  Serial.print("Logging to ");
  Serial.println( filename);
  dataFile = SD.open(filename, FILE_WRITE); // ouvrir et ferme le fichier de log à chaque appel à log() cause un crash de l'application...
                                            // Passer à SDFat pourrait résoudre ces problèmes et diminuer la consommation de mémoire

  Log("\r\n\r\n");
  
  Serial.println("Setup LCD");
  setupLCD();
      
  Serial.println("Setup complete");
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void readDateTime()
{
  // retrieve data from DS3231
  readDS3231time(&lastReadTime.second, &lastReadTime.minute, &lastReadTime.hour, &lastReadTime.dayOfWeek, &lastReadTime.dayOfMonth, &lastReadTime.month,
  &lastReadTime.year);  
  sprintf(lastReadTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", (int)lastReadTime.year+2000, (int)lastReadTime.month, (int)lastReadTime.dayOfMonth, (int)lastReadTime.hour, (int)lastReadTime.minute, (int)lastReadTime.second);
}

void refreshScreen() {
    //u8g.setFont(u8g_font_unifont);
    u8g.setFontPosTop();
    //u8g.setFont(u8g_font_osb21);
    u8g.drawStr( 0, 0, lastReadTimeStr);
    int nextTop = u8g.getFontLineSpacing() + 3;
    for (int8_t i = 0; i < (sizeof(sensorsData) / sizeof(sensorsData[0])); ++i) {
      u8g.drawStr( 0, nextTop, (String(sensorsData[i].dhtPort) + ":").c_str());
      u8g.drawStr( 20, nextTop, (String(sensorsData[i].humidity, 1) + "%").c_str());
      u8g.drawStr( 70, nextTop, (String(sensorsData[i].temperature, 1) + "C").c_str());
      nextTop += u8g.getFontLineSpacing() + 3;
    }
}

void logSensorData(int index, bool fileOnly) {
  char csvLine[50] = {0};
  sprintf(csvLine, "%s;%d;%d;%d", lastReadTimeStr, (int)sensorsData[index].dhtPort, (int)sensorsData[index].humidity, (int)sensorsData[index].temperature);
  if (!fileOnly)
    Serial.println(csvLine);
  Log(csvLine);    
}

void logSensorsData(bool fileOnly) {
  for (int i = 0; i < (sizeof(sensorsData) / sizeof(sensorsData[0])); ++i) {
    logSensorData(i, fileOnly);
  }  
}

void readSensor(int index) {
    readDateTime();

    dht.setup(sensorsData[index].dhtPort, sensorsData[index].dhtType);
    dht.resetTimer();
    sensorsData[index].humidity = dht.getHumidity();
    sensorsData[index].temperature = dht.getTemperature();
}

void readSensors() {
  for (int i = 0; i < (sizeof(sensorsData) / sizeof(sensorsData[0])); ++i) {
    readSensor(i);
  }  
}

//#if defined(USE_SD)
void printDirectory(File dir, int numTabs) {  
  if (! dir) {
    Serial.println("Invalid directory");
    return;
  }
  dir.rewindDirectory();
  
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}
//#endif

/*void processDelCommand(const String& filename ){
//#if defined(USE_SD)
  Serial.println("Deleting file " + filename);
  SD.remove((char*)filename.c_str());
//#endif
}*/

void processDirCommand(){
//#if defined(USE_SD)
  Serial.println("Listing files :");
  // File root = SD.open("/");
/*  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
  
  // printDirectory(root, 0);
  root.close();*/
//#endif
}

void processGetCommand(const String& filename ){
//#if defined(USE_SD)
  Serial.println("Getting file " + filename);
  File file = SD.open(filename.c_str(), FILE_READ);
  while (file.available()) {
    Serial.print((char)file.read());
  }
  file.close();
  Serial.println("");
//#endif
}

void processReadCommand() {
    readSensors();
    logSensorsData(false);
}

void processTimeCommand() {
  readDateTime();
  Serial.println(lastReadTimeStr);
}

void processCommand(const String& command ){
  if (command.startsWith("settime ")) {
    String newtime = command.c_str() + strlen("settime ");
    Serial.println("Unsupported for now");
    // DS3231 seconds, minutes, hours, day, date, month, year
    // setDS3231time(00,03,23,7,21,11,15);
  } else if (command.startsWith("get ")) {
    String filename = command.c_str() + strlen("get ");
    processGetCommand(filename);
  } else if (command.startsWith("more ")) {
    String filename = command.c_str() + strlen("more ");
    processGetCommand(filename);
//  } else if (command.startsWith("del ")) {
//    String filename = command.c_str() + strlen("del ");
//    processDelCommand(filename);
  } else if (command == "dir") {
    processDirCommand();
  } else if (command == "read") {
    processReadCommand();
  } else if (command == "time") {
    processTimeCommand();
  } else {
    Serial.println("Invalid command : " + command);
  }
}

void loop()
{
  while (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte == '\n' || incomingByte == '\r'){
      if (rcvdBuf != "") {
        processCommand(rcvdBuf);
        rcvdBuf = "";           
      }
    } else {
      rcvdBuf += incomingByte;
    }
  }

  if (timeElapsed > (5000)) {  
    readSensors();
  }
  
  if (timeElapsed > (SENSOR_POLLING_INTERVAL * 1000)) {  
    logSensorsData(true);
    timeElapsed = 0;
  }

  u8g.firstPage();  
  do {
    refreshScreen();
  } while( u8g.nextPage() );
  
  delay(1000);
}
