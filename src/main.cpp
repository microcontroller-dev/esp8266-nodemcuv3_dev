#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>




const int chipSelect = D8; 
File root;
File myWebfile;




const char* ssid = "blackzap";
const char* password = "0123456789";
uint8_t max_connections=8;//Maximum Connection Limit for AP
int current_stations=0, new_stations=0;
IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);
unsigned long previousMillis = 0;
const long interval = 10000; 




#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#define NUMFLAKES     10
#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

#define XPOS   0 
#define YPOS   1
#define DELTAY 2


AsyncWebServer server(80);



void printDirectory(File dir, int numTabs) {
int colcnt =0;
while(true) {
  File entry =  dir.openNextFile();
  if (! entry) {
    break;
   }
   if (numTabs > 0) {
     for (uint8_t i=0; i<=numTabs; i++) {
       Serial.print('\t');
     }
   }
   Serial.print(entry.name());
   if (entry.isDirectory()) {
     Serial.println("/");
     printDirectory(entry, numTabs+1);
   } else
   {
     Serial.print("\t");
     Serial.println(entry.size(), DEC);
   }
   entry.close();
  }
}


void testscrolltext(void) {
  display.clearDisplay();
  display.setTextSize(2); 
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      
  delay(100);
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}



void testdrawbitmap(void) {
  display.clearDisplay();
  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}




void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h) {

  int8_t f, icons[NUMFLAKES][3];
  for(f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
    icons[f][YPOS]   = -LOGO_HEIGHT;
    icons[f][DELTAY] = random(1, 6);
    Serial.print(F("x: "));
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(F(" y: "));
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(F(" dy: "));
    Serial.println(icons[f][DELTAY], DEC);
  }
  for(;;) { 
    display.clearDisplay(); 
    for(f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
    }
    display.display();
    delay(200);        
    for(f=0; f< NUMFLAKES; f++) {
      icons[f][YPOS] += icons[f][DELTAY];
      if (icons[f][YPOS] >= display.height()) {
        icons[f][XPOS]   = random(1 - LOGO_WIDTH, display.width());
        icons[f][YPOS]   = -LOGO_HEIGHT;
        icons[f][DELTAY] = random(1, 6);
      }
    }
  }
}




void setup() {
  Serial.begin(115200);
  Serial.println("Console > Started !....");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(400);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Console > SSD1306 allocation failed!..."));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Console > ");
  display.display(); 
  display.setCursor(0, 20);
  display.println(" Started!");
  display.display(); 


  testdrawbitmap();
  delay(2000); 
  testanimate(logo_bmp, 16, 16);


  Serial.print("\r\nWaiting for SD card to initialise...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Initialising failed!");
    return;
  }
  Serial.println("Initialisation completed");



  if(WiFi.softAP(ssid,password,1,false,max_connections)==true)
  {
    Serial.print("Access Point is Created with SSID: ");
    Serial.println(ssid);
    Serial.print("Max Connections Allowed: ");
    Serial.println(max_connections);
    Serial.print("Access Point IP: ");
    Serial.println(WiFi.softAPIP());
  }
  else
  {
    Serial.println("Unable to Create Access Point");
  }
  Serial.println(WiFi.localIP());






  myWebfile = SD.open("index.htm", FILE_READ);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", myWebfile.readString());
  });

  server.on("/result", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "result.htm");
  });


server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
  String json = "[";
  int n = WiFi.scanComplete();
  if(n == -2){
    WiFi.scanNetworks(true);
  } else if(n){
    for (int i = 0; i < n; ++i){
      if(i) json += ",";
      json += "{";
      json += "\"rssi\":"+String(WiFi.RSSI(i));
      json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
      json += ",\"channel\":"+String(WiFi.channel(i));
      json += ",\"secure\":"+String(WiFi.encryptionType(i));
      json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
      json += "}";
    }
    WiFi.scanDelete();
    if(WiFi.scanComplete() == -2){
      WiFi.scanNetworks(true);
    }
  }
  json += "]";
  request->send(200, "application/json", json);
  json = String();
});


server.begin();



}

void loop() {

  digitalWrite(LED_BUILTIN, LOW);
  delay(400);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);

  root = SD.open("/");
  root.rewindDirectory();
  printDirectory(root, 0); 
  root.close();
  Serial.flush(); Serial.begin(9600); while(!Serial.available());

  Serial.println("Open a new file called 'testfile.txt' and write 1 to 5 to it");
  File testfile = SD.open("testdata.txt", FILE_WRITE); 
  if (testfile) {
    for (int entry = 0; entry <= 5; entry = entry + 1) {
      testfile.print(entry);
      Serial.print(entry); 
    }
  }
  Serial.println("\r\nCompleted writing data 1 to 5\r\n");
  testfile.close();
  Serial.flush(); Serial.begin(9600); while(!Serial.available());
  Serial.println("Open a file called 'testfile.txt' and read the data from it");
  testfile = SD.open("testdata.txt", FILE_READ);
  while (testfile.available()) {
    Serial.write(testfile.read());
  }
  Serial.println("\r\nCompleted reading data from file\r\n");
  testfile.close();
  Serial.flush(); Serial.begin(9600); while(!Serial.available());
  //-------------------------------------------------------------------------------
  Serial.println("Open a file called 'testfile.txt' and append 5 downto 1 to it");
  testfile = SD.open("testdata.txt", FILE_WRITE);
  for (int entry = 5; entry >= 0; entry = entry - 1) {
    testfile.print(entry);
    Serial.print(entry);
  }
  Serial.println("\r\nCompleted writing data 5 to 1\r\n");
  testfile.close();
  Serial.flush(); Serial.begin(9600); while(!Serial.available());
  //-------------------------------------------------------------------------------
  Serial.println("Open a file called 'testfile.txt' and read it");
  testfile = SD.open("testdata.txt", FILE_READ); 
  while (testfile.available()) {
    Serial.write(testfile.read());
  }
  Serial.println("\r\nCompleted reading from the file\r\n");
  testfile.close();
  Serial.flush(); Serial.begin(9600); while(!Serial.available());
  Serial.println("\r\nOpen a file called 'testfile.txt' and move to position 8 in the file then print the data (should be 3)");
  testfile = SD.open("testdata.txt", FILE_READ); 
  Serial.print("Data at file location (8): ");
  testfile.seek(8);
  Serial.write(testfile.read());
  Serial.flush(); Serial.begin(9600); while(!Serial.available());
  Serial.println("\r\nOpen a file called 'testfile.txt' and move to position 6 in the file then print the data (should be 5)");
  Serial.print("Data at file location (6): ");
  testfile.seek(6);
  Serial.write(testfile.read());
  Serial.print("\r\nFile pointer is at file location: ");
  Serial.println(testfile.position());
  Serial.print("\r\nData at current file location (should be 4): ");
  Serial.println(char(testfile.peek()));
  testfile.close();
  Serial.flush(); Serial.begin(9600); while(!Serial.available());
  Serial.println("\r\nOpen a file called 'testfile.txt' and write some data records");
  testfile = SD.open("testdata.txt", FILE_WRITE); 
  int hours = 10;
  int mins  = 00;
  String names = "Mr Another";
  testfile.print("\r\n"+String(hours));
  testfile.print(":");
  testfile.print(String(mins,2));
  testfile.println(" "+names);
  Serial.print(hours);
  Serial.print(":");
  Serial.print(mins);
  Serial.println(names);
  names = "Mr And Another";
  testfile.print(String(hours+1));
  testfile.print(":");
  testfile.print(String(mins+1));
  testfile.println(" "+names+"\r\n");
  Serial.print(hours);
  Serial.print(":");
  Serial.print(mins,2);
  Serial.print(names);
  Serial.println("\r\nCompleted writing data records to the file\r\n");
  testfile.close();
  Serial.flush(); Serial.begin(9600); while(!Serial.available());
  Serial.println("\r\nOpen a file called 'testfile.txt' and move to position 8 in the file then print the data (should be 4)");
  testfile = SD.open("testdata.txt", FILE_READ); // FILE_WRITE opens file for writing and moves to the end of the file
  while (testfile.available()) {
    Serial.write(testfile.read());
  }
  Serial.println("\r\nCompleted reading records from the file\r\n");
  Serial.flush(); Serial.begin(9600); while(!Serial.available());
  Serial.println("\r\nStarting again");
}
