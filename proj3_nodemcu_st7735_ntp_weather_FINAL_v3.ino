/*
//
// openweathermap/wassertipps  - alex pleiner, 2024, alex@zeitform.de
//
 **************************************************************************/

#include <Adafruit_GFX.h>    // core graphics library
#include <Adafruit_ST7735.h> // hardware-specific library for ST7735
#include <SPI.h>

#define TFT_CS         D8
#define TFT_RST        D6 // D4 is builtin led // -> D6                                           
#define TFT_DC         D3
   
//#define shortdelay    500

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// qrcode
//#include <qrcode_st7735.h>
//QRcode_ST7735 qrcode (&tft);
#include <qrcode.h>
// OLED display dimensions
#define OLED_WIDTH 128/2  // fixme
#define OLED_HEIGHT 160 // fixme

// pins
#define PHOTO_PIN     A0
#define SLEEP_PIN     D2
#define BUTTON_PIN    D1


// ntp
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//const char *ssid     = "nirvana.net";
//const char *password = "5882721161769666";
//char ssid[32];
//char possword[32];
//char *ssid = "nirvana.net";
//char *password = "5882721161769666";

//char ssid[32] = ""; //     = "nirvana.net";
//char password[32] = ""; // = "5882721161769666";
//String ssid = "nirvana.net";
//String password = "5882721161769666";

const long utcOffsetInSeconds = 3600;

String weekDays[7]={"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// openweathermap
#include <WiFiClientSecure.h>
#include <JSON_Decoder.h>
#include <OpenWeather.h>

#define MAX_HOURS 8 // edit /libraries/OpenWeather/User_Setup.h
#define MAX_DAYS 6 // edit /libraries/OpenWeather/User_Setup.h

#define ST77XX_LIGHTBLUE 0x52bf    // https://rgbcolorpicker.com/565

// OpenWeather API Details, replace x's with your API key
//String api_key = "4c9431d7972d708f48b29d6e893ea2d6"; // Obtain this from your OpenWeather account
//String api_key = "56eb1827cf56166afbae2f6e98743d07";
String eapi;
String elat;
String elng;


// Set both your longitude and latitude to at least 4 decimal places
//String latitude =  "49.878708"; // 90.0000 to -90.0000 negative for Southern hemisphere
//String longitude = "8.646927"; // 180.000 to -180.000 negative for West

String units = "metric";  // or "imperial"
String language = "de";   // See notes tab

OW_Weather ow; // Weather forecast library instance
OW_current *current = new OW_current;
OW_hourly *hourly = new OW_hourly;
OW_daily  *daily = new OW_daily;
OW_forecast  *forecast = new OW_forecast;

// v2
#include <ESP8266WebServer.h>
#include "index.h"
const char* ap_ssid = "OpenWeather";  // Enter SSID here
const char* ap_password = "";  //Enter Password here

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

#include <EEPROM.h>

#include <ArduinoJson.h>
JsonDocument doc;

/**********************************************************************************/

void setup(void) {

  //pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(D4, OUTPUT); // builtin led
  pinMode(PHOTO_PIN, INPUT);
  pinMode(SLEEP_PIN, OUTPUT);
  digitalWrite(SLEEP_PIN, HIGH);

  Serial.begin (115200);
  EEPROM.begin(512);

  delay(100);
  Serial.println();
  Serial.println();
  Serial.println("Startup");

  tft.initR (INITR_BLACKTAB); 
  tft.fillScreen (ST77XX_BLACK);

//setupAP(); // fixme

  tft.drawBitmap(0, 0, open_icon, 128, 59, ST7735_ORANGE);

  tft.setCursor (0, 70);
  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);

  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i) esid += char(EEPROM.read(i));
  Serial.print("SSID: ");
  //Serial.println(esid);
  Serial.println(esid.c_str());

  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i) epass += char(EEPROM.read(i));
  Serial.print("PASS: ");
  //Serial.println(epass);  
  Serial.println(epass.c_str()); 

  Serial.println("Reading EEPROM api");
  String eapi = "";
  for (int i = 96; i < 128; ++i) eapi += char(EEPROM.read(i));
  Serial.print("API: ");
  //Serial.println(eapi);  
  Serial.println(eapi.c_str()); 

  Serial.println("Reading EEPROM lat");
  String elat = "";
  for (int i = 128; i < 138; ++i) elat += char(EEPROM.read(i));
  Serial.print("LAT: ");
  //Serial.println(eapi);  
  Serial.println(elat.c_str()); 

  Serial.println("Reading EEPROM lng");
  String elng = "";
  for (int i = 138; i < 148; ++i) elng += char(EEPROM.read(i));
  Serial.print("LNG: ");
  //Serial.println(eapi);  
  Serial.println(elng.c_str()); 

  //const char *c = esid.c_str();

  if ( strlen(esid.c_str()) == 0 || strlen(eapi.c_str()) == 0 || strlen(elat.c_str()) == 0 || strlen(elng.c_str()) == 0 ) {  // fixme
  
    //delay(100);

    setupAP();
    //return;
  }

  tft.setTextWrap (true);
  tft.print ("Wifi: ");
  tft.println (esid.c_str());

  WiFi.begin(esid.c_str(), epass.c_str());
  //WiFi.begin("xxx", "yyy");

  //Serial.println(ssid);
  //Serial.println(password);

  //int i;
  unsigned long currentMillis = millis();
  while ( WiFi.status() != WL_CONNECTED ) {
  
    digitalWrite(LED_BUILTIN, LOW);
    delay ( 250 );
    digitalWrite(LED_BUILTIN, HIGH);
    delay ( 250 );

    if (digitalRead(BUTTON_PIN) == HIGH) { // button pressed
      
      //Serial.println("-->button low");

      //EEPROM.begin(512);
      //Serial.println("clearing eeprom");
      //for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      //EEPROM.commit();

      setupAP();
    }

    if (millis() - currentMillis > 1000 * 60) { // no wifi
      setupAP();
    }

    Serial.print ( "." );
    tft.print (".");
  }
  tft.setTextColor (ST77XX_GREEN);
  tft.print ("*");

  digitalWrite(LED_BUILTIN, HIGH);
  
  timeClient.begin();
  timeClient.update();

  // openweathermap
  ow.getForecast(current, hourly, daily, eapi.c_str(), elat.c_str(), elng.c_str(), units, language);
  //ow.getForecast(forecast, api_key, latitude, longitude, units, language);
  tft.print (")");

  // wassertipps
  wassertipps(elat.c_str(), elng.c_str());
  tft.print (")");
  //printCurrentWeather();
  //tftWeather();
  //tft.fillScreen (ST77XX_BLACK);
  
  tftWeatherMore();
  //printCurrentWeather();
  //tft.print (")");
  
  Serial.println("OK");
  // Serial.println(ssid);

  //tft.initR (INITR_BLACKTAB);
}

void wassertipps(String latitude, String longitude) {
  // wassertipps
  String url = "https://www.wassertipps.de/cgi-bin/server_api2.pl?json_data={\"lon\":" + longitude + ",\"lat\":" + latitude + "}";

  //Serial.println(url);

  //uint32_t dt = millis();
  
  //Serial.println("\n\nThe connection to server is secure (https). Certificate not checked.\n");
  WiFiClientSecure client;
  client.setInsecure(); // Certificate not checked

  const char*  host = "www.wassertipps.de";
  int port = 443;

  if (!client.connect(host, port))
  {
    Serial.println("Connection failed.\n");
    //return false;
    return;
  }

  //JSON_Decoder parser;
  //parser.setListener(this);
  //JsonDocument doc;
  
  uint32_t timeout = millis();
  char c = 0;

  //bool parseOK = false;

  // Serial.println();
  // Serial.print("Sending GET request to "); 
  // Serial.print(host); 
  // Serial.print(" port ");
  // Serial.print(port); 
  // Serial.println();

  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

  String currentLine = "";

  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Header end found");
      break;
    }
    if ((millis() - timeout) > 5000UL)
    {
      Serial.println("HTTP header timeout");
      client.stop();
      //return false;
      return;
    }
  }
  
  Serial.println("\nParsing JSON");
  while ( client.available() > 0 || client.connected())
  {
    bool body = false;
    while(client.available() > 0)
    {
      c = client.read();
      //Serial.print("c: ");
      Serial.print(c);
      if (c == '\n' && ! body) {  
        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0) {
            body = true;     // fixme
        } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
        } 

      } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
          //Serial.println("currentLine: ");
          //Serial.println(currentLine);
      }    
          // 

      //Serial.println(currentLine);
      if (currentLine.endsWith("[{")) { 
        currentLine = "[{";
      }

      if (currentLine.endsWith("}]")) { // fixme

          //JsonDocument doc;
          //Serial.println("currentLine: ");
          //Serial.println(currentLine);

          // Parse JSON object
          DeserializationError error = deserializeJson(doc, currentLine);
          if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            client.stop();
            return;
          }

          //Serial.println("currentLine: ");
          //Serial.println(currentLine);
          // Serial.print("zip: ");
          // Serial.println(doc[0]["zip"].as<const char*>());
          // Serial.print("city: ");
          // Serial.println(doc[0]["city"].as<const char*>());
          // Serial.print("wasserhaerte: ");
          // Serial.print(doc[0]["wasserhaerte_min"].as<const char*>());
          // Serial.print(" / ");
          // Serial.print(doc[0]["wasserhaerte_avg"].as<const char*>());
          // Serial.print(" / ");
          // Serial.println(doc[0]["wasserhaerte_max"].as<const char*>());
      }
    
    }

    if ((millis() - timeout) > 8000UL)
    {
      Serial.println("Client timeout during JSON parse");
      //parser.reset();
      client.stop();
      //return false;
      return;
    }
    // yield();
  }

  // A message has been parsed, but the data-point correctness is unknown
  //return parseOK;
  return;

  Serial.println("--->OK");

}


void setupAP () {
  
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println();
  Serial.println("wifi->ap");

  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(1000);
  
      //   IPAddress IP = WiFi.softAPIP();
      //   Serial.print("AP IP address: ");
      //   Serial.println(IP);
      //   Serial.println(WiFi.localIP());
      // Serial.println(WiFi.status());

    //  Serial.println(ap_ssid);
    //  Serial.println(ap_password);

  server.begin();

  server.on("/", handle_OnConnect);
  server.on("/wifi", handle_OnWifi);
  server.onNotFound(handle_NotFound);

  //delay(100);  // niemals!!
  tft.fillScreen (ST77XX_BLACK);

  tft.setCursor (64, 12);
  tft.setTextColor (ST77XX_MAGENTA);
  tft.print ("Kein Wifi!");

  tft.drawBitmap(0, 0, open_icon, 128, 59, ST7735_ORANGE);

  // tft.setCursor (0, 66);
  // tft.setTextColor (ST77XX_MAGENTA);
  // tft.setTextSize (1);

  // tft.println ("Kein Wifi!");
  // tft.println  ("");

  tft.setCursor (0, 59);
  //tft.setTextColor (ST77XX_BLACK);
  //qrcode.init();
  //String qr = "WIFI:S:OpenWeather;H:false;;";
  // qrcode.create(qr);
  String t = "WIFI:S:";
  t += ap_ssid;
  t += ";H:false;P:";
  t += ap_password;
  t += ";";
  generateQRCode(t, 0, 100, ST77XX_GREEN);

  t = "http://";
  t += WiFi.softAPIP().toString();
  generateQRCode(t, 68, 100, ST77XX_YELLOW);

  tft.setTextColor (ST77XX_GREEN);

  tft.print ("1. SSID: ");
  tft.println (ap_ssid);
  // tft.println  ("");

  if (strlen(ap_password) != 0) {
    //tft.println  ("");
     tft.print ("   PASS: ");
     tft.println (ap_password);
     //   tft.println  ("");
  }

  tft.setTextColor (ST77XX_YELLOW);


  // tft.println  ("");
  tft.print ("2. http://"); http://
  tft.println (local_ip);
  // tft.println  ("");

  tft.setTextColor (ST77XX_WHITE);

  // tft.println  ("");
  tft.println ("3. Dein SSID/Passwort");
  //tft.print ();
  // tft.println  ("");

  // tft.println  ("");
  tft.println ("4. Reboot ");
  // //tft.print ();
  // tft.println  ("");



  while (1) server.handleClient();
} 

void generateQRCode(String text, byte shiftX, byte shiftY, int color) {
  // Create a QR code object
  QRCode qrcode;
  
  // Define the size of the QR code (1-40, higher means bigger size)
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, text.c_str());

  // Clear the display
  //display.clearDisplay();

  // Calculate the scale factor
  int scale = min(OLED_WIDTH / qrcode.size, OLED_HEIGHT / qrcode.size);
  
  // Calculate horizontal shift
  //int shiftX = XXX; // + (OLED_WIDTH - qrcode.size*scale)/2;
  
  // Calculate horizontal shift
  //int shiftY = YYY; //(OLED_HEIGHT - qrcode.size*scale)/2;

  // Draw the QR code on the display
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(shiftX+x * scale, shiftY + y*scale, scale, scale, color);
      }
    }
  }

  // Update the display
  //display.display();
}

///////////////////////////////////////////////////////////

void handle_OnConnect() {

  String s = MAIN_page;
  server.send(200, "text/html", s); 

}

void handle_OnWifi() {

  String myssid =  server.arg("ssid");
  String mypassword = server.arg("password");

  String myapi =  server.arg("api"); 
  String mylat =  server.arg("lat"); 
  String mylng =  server.arg("lng"); 

  EEPROM.begin(512);

  Serial.println("clearing eeprom");
  for (int i = 0; i < 148; ++i) { EEPROM.write(i, 0); }

  Serial.println("writing eeprom ssid:");
  for (int i = 0; i < myssid.length(); ++i)
  {
    EEPROM.write(i, myssid[i]);
    Serial.print("Wrote: ");
    Serial.println(myssid[i]); 
  }
  Serial.println("writing eeprom pass:"); 
  for (int i = 0; i < mypassword.length(); ++i)
  {
    EEPROM.write(32+i, mypassword[i]);
    Serial.print("Wrote: ");
    Serial.println(mypassword[i]); 
  }
  Serial.println("writing eeprom api:");
  for (int i = 0; i < myapi.length(); ++i)
  {
    EEPROM.write(96+i, myapi[i]);
    Serial.print("Wrote: ");
    Serial.println(myapi[i]); 
  }
  Serial.println("writing eeprom lat:");
  for (int i = 0; i < mylat.length(); ++i)
  {
    EEPROM.write(128+i, mylat[i]);
    Serial.print("Wrote: ");
    Serial.println(mylat[i]); 
  }
  Serial.println("writing eeprom lng:");
  for (int i = 0; i < mylng.length(); ++i)
  {
    EEPROM.write(138+i, mylng[i]);
    Serial.print("Wrote: ");
    Serial.println(mylng[i]); 
  }
  EEPROM.commit();

  String s = MAIN_page;

  Serial.println("Wifi: ");
  Serial.println(myssid);
  Serial.println(mypassword);

  server.send(200, "text/plain", "Success"); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

////////////////////////////////////////////////////////
unsigned long currentMillis;
bool tft_hourly = true;
byte tft_stage = 1; // daily?

void loop() {

  int light = analogRead(PHOTO_PIN);
  //Serial.println("light: " + (String) light);

  if (light < 100) { // fixme
    digitalWrite(SLEEP_PIN, LOW); // 8 LEDA/BLK --> 3V3 oder D2 HIGH/LOW (display abschalten)
    //delay(200);
    return;
  } else {
    digitalWrite(SLEEP_PIN, HIGH);
  }

  if (digitalRead(BUTTON_PIN) == HIGH) {
    //delay(200);
    //Serial.print("global: ");
    //Serial.println(tft_hourly);// == true : "true" : "false")
    //Serial.println(tft_stage);

    if (tft_stage == 0 || tft_stage == 1) {

      Serial.print("hourly: ");
      //Serial.println(tft_hourly);// == true : "true" : "false")
      Serial.println(tft_stage);
      //if (tft_hourly == true) { tftWeatherMore(); }
      //else { tftWeatherMoreDaily(); }
      tft_hourly = ! tft_hourly;
      tftWeatherMore();
      //tft_hourly = ! tft_hourly;

    } else if (tft_stage == 2) {
      
      // wassertipps
      Serial.print("wassertipps: ");
      tftWassertipps();
      //tft_hourly = true;
      //Serial.println(tft_hourly);// == true : "true" : "false")
      Serial.println(tft_stage);
    }
    tft_stage += 1;
    if (tft_stage > 2) tft_stage = 0;

  }

  //unsigned long currentMillis;
  if (millis() - currentMillis > 1000 * 60 * 15) {
    
    //ow.getForecast(current, hourly, daily, api_key, latitude, longitude, units, language);
    ow.getForecast(current, hourly, daily, eapi.c_str(), elat.c_str(), elng.c_str(), units, language);
    // Serial.print ( "millis: " );
    // Serial.print (millis());
    // Serial.print ( "currentMillis: " );
    // Serial.println (currentMillis);
    
    //if (tft_hourly == true) { tftWeatherMore(); }
    //else { tftWeatherMoreDaily(); }
    tft_hourly = true;
    tftWeatherMore();

    currentMillis = millis();
  }


  // if (millis() / 5000 % 2) {
  //   state = 1;
  //   if (state != prevState) tft.fillScreen (ST77XX_BLACK);
  //   prevState = state;
  //   tftWeather();
  // } else {
  //   state = 2;
  //   if (state != prevState) tft.fillScreen (ST77XX_BLACK);
  //   prevState = state;
  //   tftWeatherMore(); 
  // }
  
  //tft.fillScreen (ST77XX_BLACK);
  timeClient.update();
  
  tftDateTime();
  //tftWeatherMore(); 
  //tftWeather(); 

  delay (1000);

}

//byte hour;

void tftDateTime ()
{
  // ntp -> date/time
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;

  //hour = ptm->tm_hour;  // ganz oben 

  String formattedDate = (monthDay < 10 ? "0" : "") + String(monthDay) + "." 
                       + (currentMonth < 10 ? "0" : "") + String(currentMonth) + "." 
                       + String(currentYear - 2000);
  String formattedTime = timeClient.getFormattedTime();
  String weekDay = weekDays[timeClient.getDay()];
  //Serial.println(weekDay);

  tft.setCursor (0, 0);
  //tft.fillScreen (ST77XX_BLACK);
  tft.fillRect(0, 0, 59, 8, ST77XX_BLACK); // tip!
  if (ptm->tm_hour == 0 && ptm->tm_min == 0 && ptm->tm_sec == 0) tft.fillRect(60, 0, 128, 8, ST77XX_BLACK); // tip!


  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);
   
  tft.print (formattedTime);
  tft.setCursor (60, 0);
  tft.print (weekDay);
  tft.print (",");
  tft.setCursor (80, 0);
  tft.println (formattedDate);
}

void tftWassertipps () 
{ 

  byte row = 14;
  tft.fillScreen (ST77XX_BLACK);
  tft.setTextColor (ST77XX_ORANGE);
  
  tft.setCursor (0, row);
  tft.println (doc[0]["zip"].as<const char*>());

  tft.setTextColor (ST77XX_WHITE);
  tft.setCursor (0, row+10);
  tft.println (doc[0]["city"].as<const char*>());


  row = 32;
  tft.setCursor (0, row);
  tft.setTextColor (ST77XX_YELLOW);
  tft.println();
  tft.println("Wasserh\x84rte:");

// https://cdn-learn.adafruit.com/downloads/pdf/adafruit-gfx-graphics-library.pdf   
// --> Seite 16
//
// \x84 = ä
// \x94 = ö
// \x81 = ü
// \x8E = Ä 
// \x99 = Ö
// \x9A = Ü 
// \xF7 = grad


  float wasserhaerte_min = doc[0]["wasserhaerte_min"];
  float wasserhaerte_avg = doc[0]["wasserhaerte_avg"];
  float wasserhaerte_max = doc[0]["wasserhaerte_max"];
   
  

  //tft.drawFastHLine(0 + i * COL, row + 30 - 20 * ((hourly->temp[i] - imin) / (imax - imin)), COL, ST77XX_GREEN);
  if (wasserhaerte_min > 0) tft.drawFastVLine(wasserhaerte_min * 128 / 22, row+20, 7, ST77XX_YELLOW);
  if (wasserhaerte_max > 0) tft.drawFastVLine(wasserhaerte_max * 128 / 22, row+20, 7, ST77XX_YELLOW);
  if (wasserhaerte_avg > 0) tft.drawFastVLine(wasserhaerte_avg * 128 / 22, row+20, 7, ST77XX_YELLOW);

  if (wasserhaerte_min > 0 && wasserhaerte_max > 0) {
    tft.drawFastHLine(wasserhaerte_min * 128 / 22 + 1, row+20 + 2, (wasserhaerte_max - wasserhaerte_min) * 128 / 22, ST77XX_YELLOW);
    tft.drawFastHLine(wasserhaerte_min * 128 / 22 + 1, row+20 + 3, (wasserhaerte_max - wasserhaerte_min) * 128 / 22, ST77XX_YELLOW);
    tft.drawFastHLine(wasserhaerte_min * 128 / 22 + 1, row+20 + 4, (wasserhaerte_max - wasserhaerte_min) * 128 / 22, ST77XX_YELLOW);
  }

  tft.drawFastHLine(0, row+28, (8.4) * 128 / 22, ST77XX_GREEN);
  tft.drawFastHLine((8.4) * 128 / 22 + 1, row+28, (14 - 8.4) * 128 / 22 - 1, ST77XX_YELLOW);
  tft.drawFastHLine((14) * 128 / 22, row+28, 128, ST77XX_RED);

  tft.setCursor (0, row+36);
  if (wasserhaerte_min > 0) tft.print(wasserhaerte_min, 1); 
  tft.print(" < ");
  if (wasserhaerte_avg > 0) tft.print(wasserhaerte_avg, 1);
  tft.print(" < ");
  if (wasserhaerte_max > 0) tft.print(wasserhaerte_max, 1);
  tft.print(" ");
  tft.write (247);  // grad
  tft.print("dH");
  
// Wasserhärte:
//  0 - 8,4 °dH (< 1,5 mmol/l - weich)
//  8,5 - 14 °dH (1,5 - 2,5 mmol/l - mittel)
//  über 14 °dH (> 2,5 mmol/l - hart)

// Nitratwerte:
//  0 - 10 mg/l
//  10 - 25 mg/l
//  25 - 50 mg/l


row = 80;
  tft.setCursor (0, row);
  tft.setTextColor (ST77XX_GREEN);
  tft.println();
  tft.println("Nitrat:");

  float nitrat_min = doc[0]["nitrat_min"];
  float nitrat_avg = doc[0]["nitrat_avg"];
  float nitrat_max = doc[0]["nitrat_max"];

  if (nitrat_min > 0) tft.drawFastVLine(nitrat_min * 128 / 50, row+20, 7, ST77XX_GREEN);
  if (nitrat_max > 0) tft.drawFastVLine(nitrat_max * 128 / 50, row+20, 7, ST77XX_GREEN);
  if (nitrat_avg > 0) tft.drawFastVLine(nitrat_avg * 128 / 50, row+20, 7, ST77XX_GREEN);

  if (nitrat_min > 0 && nitrat_max > 0) {
    tft.drawFastHLine(nitrat_min * 128 / 50 + 1, row+20 + 2, (nitrat_max - nitrat_min) * 128 / 50, ST77XX_GREEN);
    tft.drawFastHLine(nitrat_min * 128 / 50 + 1, row+20 + 3, (nitrat_max - nitrat_min) * 128 / 50, ST77XX_GREEN);
    tft.drawFastHLine(nitrat_min * 128 / 50 + 1, row+20 + 4, (nitrat_max - nitrat_min) * 128 / 50, ST77XX_GREEN);
  }

// Nitratwerte:
//  0 - 10 mg/l
//  10 - 25 mg/l
//  25 - 50 mg/l

  tft.drawFastHLine(0, row+28, (10) * 128 / 50, ST77XX_GREEN);
  tft.drawFastHLine((10) * 128 / 50 + 1, row+28, (25 - 10) * 128 / 50 - 1, ST77XX_YELLOW);
  tft.drawFastHLine((25) * 128 / 50, row+28, 128, ST77XX_RED);

  tft.setCursor (0, row+36);
  if (nitrat_min > 0) tft.print(nitrat_min, 1); 
  tft.print(" < ");
  if (nitrat_avg > 0) tft.print(nitrat_avg, 1);
  tft.print(" < ");
  if (nitrat_max > 0) tft.print(nitrat_max, 1);
  // tft.print(" ");
  // tft.write (247);  // grad
  tft.print(" mg/l");
  
}

void tftWeatherMore () 
{ 
  tft.fillScreen (ST77XX_BLACK);

  byte MAX; byte COL;
  if (tft_hourly == true) { 
    Serial.println("HOURLY----------"); 
    MAX = 8;
    COL = 16;
  } 
  else { 
    Serial.println("DIALY----------"); 
    MAX = 6;
    COL = 22;
  }

  byte row;
  row = 152;
  tft.fillRect(0, row, 128, 10, ST77XX_BLACK);

  // time_t epochTime = timeClient.getEpochTime();
  // struct tm *ptm = gmtime ((time_t *)&epochTime);
  // int hour = ptm->tm_hour;

  //String weekDay = weekDays[timeClient.getDay()];
  //int currentMonth = ptm->tm_mon+1;
  //int currentYear = ptm->tm_year+1900;

  //hour = ptm->tm_hour;  // ganz oben 
  
  tft.setTextColor (ST77XX_WHITE);

  if (tft_hourly == true) {
    // hour
    for (int i = 0; i < 8; i++) {
      //tft.setTextColor (ST77XX_WHITE);
      tft.setCursor (2 + i * COL, row);
      if ((timeClient.getHours() + i) % 24 < 10) tft.print (" ");
      tft.print ((timeClient.getHours() + i) % 24);
    }
  } else { 
    // daily
    for (int i = 0; i < 6; i++) {
      //tft.setTextColor (ST77XX_WHITE);
      tft.setCursor (4 + i * COL, row);
      //if ((timeClient.getDay() + i) % 7 < 10) tft.print (" ");
      tft.print (weekDays[(timeClient.getDay() + i) % 7]);
    }
  }
  //Serial.println("1");
  float max; float min; int imin; int imax; 
  
  // temperatur
  if (tft_hourly == true) {
     max = 0; min = hourly->temp[0];
     for (int i = 0; i < MAX; i++) {
       if (max < hourly->temp[i]) max = hourly->temp[i];
       if (min > hourly->temp[i]) min = hourly->temp[i];
     }
  } else {
     max = 0; min = daily->temp_max[0];
     for (int i = 0; i < MAX; i++) {
       if (max < daily->temp_max[i]) max = daily->temp_max[i];
       if (min > daily->temp_max[i]) min = daily->temp_max[i];
     }  
  }
  //Serial.println("2");
  //Serial.println(daily->temp_max[0]);

  imin = min - 0.5; imax = max + 1;

  if (imin == imax) { imax += 1; }
  
  tft.setTextColor (ST77XX_GREEN);
  row = 11;
  tft.setCursor (0, row);
  tft.print ("Temperatur: ");
  tft.print (current->temp, 1);
  tft.write (247);  // grad
  tft.println ("C");

  tft.setCursor (imax < 10 ? 122 : 116, row);
  tft.print (imax);
  tft.setCursor (imin < 10 ? 122 : 116, row + 24);
  tft.print (imin);

  for (int i = 0; i < MAX; i++) {
    if (tft_hourly == true) {
      tft.drawFastHLine(0 + i * COL, row + 30 - 20 * ((hourly->temp[i] - imin) / (imax - imin)), COL, ST77XX_GREEN);
      //tft.drawFastHLine(0 + i * 16, row + 30 - 20 * (i % 2 ? 1 : 0), 16, ST77XX_GREEN);
    } else {
      tft.drawFastHLine(0 + i * COL, row + 30 - 20 * ((daily->temp_max[i] - imin) / (imax - imin)), COL, ST77XX_GREEN);
    }
  }
  
  // wind
  if (tft_hourly == true) {
    max = 0; min = hourly->wind_speed[0];
    for (int i = 0; i < MAX; i++) {
      if (max < hourly->wind_speed[i]) max = hourly->wind_speed[i];
      if (min > hourly->wind_speed[i]) min = hourly->wind_speed[i];
    }
  } else {
     max = 0; min = daily->wind_speed[0];
     for (int i = 0; i < MAX; i++) {
       if (max < daily->wind_speed[i]) max = daily->wind_speed[i];
       if (min > daily->wind_speed[i]) min = daily->wind_speed[i];
     }  
  }

  imin = min - 0.5; imax = max + 1;

  if (imin == imax) { imax += 1; }

  tft.setTextColor (ST77XX_ORANGE);
  row = 46;
  tft.setCursor (0, row);
  tft.print ("Wind: ");
  //tft.print (deg2ono(current->wind_deg));
  tft.print (current->wind_speed, 1);
  tft.print ("m/s ");
  tft.println (deg2ono(current->wind_deg));
  //tft.print (current->wind_speed, 1);
  //tft.println ("m/s ");

  tft.setCursor (imax < 10 ? 122 : 116, row);
  tft.print (imax);
  tft.setCursor (imin < 10 ? 122 : 116, row + 24);
  tft.print (imin);

  for (int i = 0; i < MAX; i++) {
    if (tft_hourly == true) {
      tft.drawFastHLine(0 + i * COL, row + 30 - 20 * ((hourly->wind_speed[i] - imin) / (imax - imin)), COL, ST77XX_ORANGE);
    } else {
      tft.drawFastHLine(0 + i * COL, row + 30 - 20 * ((daily->wind_speed[i] - imin) / (imax - imin)), COL, ST77XX_ORANGE);
    }
  }

  // regen/schnee
  max = 0; min = 0; // min=0;
  byte snow;
  if (tft_hourly == true) {
    for (int i = 0; i < MAX; i++) {
      if (max < hourly->rain1h[i]) { 
        max = hourly->rain1h[i];
        snow = 0;
      }
      if (max < hourly->snow[i]) {
        max = hourly->snow[i];
        snow = 1;
      }
    }
  } else {
  for (int i = 0; i < MAX; i++) {
      if (max < daily->rain[i]) { 
        max = daily->rain[i];
        snow = 0;
      }
      if (max < daily->snow[i]) {
        max = daily->snow[i];
        snow = 1;
      }
    }
  }
   
  imin = min; imax = max + 1;

  if (imin == imax) { imax += 1; }
  
  tft.setTextColor (ST77XX_LIGHTBLUE);
  row = 81;
  tft.setCursor (0, row);
  tft.print ("Regen");
  tft.setTextColor (ST77XX_WHITE);
  tft.print ("/Schnee");
  
  tft.setTextColor (snow > 0 ? ST77XX_WHITE : ST77XX_LIGHTBLUE);
  tft.setCursor (imax < 10 ? 122 : 116, row);
  tft.print (imax);
  //tft.setCursor (imin < 10 ? 122 : 116, row + 24);
  //tft.print (imin);

  //float aaa[8] = { 0.4, 0.5, 1, 0.3, 0.2, 0.1, 0.05, 0.95 };

  for (int i = 0; i < MAX; i++) {
    if (tft_hourly == true) {
      tft.fillRect(4 + i * COL, 
                  row + 30 + 1 - 20 * ((hourly->rain1h[i] - imin) / (imax - imin)), 
                  7, 
                  20 * ((hourly->rain1h[i] - imin) / (imax - imin)), 
                  ST77XX_LIGHTBLUE); 
      tft.fillRect(11 + i * COL, 
                  row + 30 + 1 - 20 * ((hourly->snow[i] - imin) / (imax - imin)), 
                  2, 
                  20 * ((hourly->snow[i] - imin) / (imax - imin)), 
                  ST77XX_WHITE); 
      // tft.fillRect(11 + i * COL, 
      //             row + 30 + 1 - 20 * aaa[i], 
      //             2, 
      //             20 * aaa[i], 
      //             ST77XX_WHITE);            

    } else {
      tft.fillRect(3 + i * COL, 
                  row + 30 + 1 - 20 * ((daily->rain[i] - imin) / (imax - imin)), 
                  12, 
                  20 * ((daily->rain[i] - imin) / (imax - imin)), 
                  ST77XX_LIGHTBLUE); 
      tft.fillRect(15 + i * COL, 
                  row + 30 + 1 - 20 * ((daily->snow[i] - imin) / (imax - imin)), 
                  2, 
                  20 * ((daily->snow[i] - imin) / (imax - imin)), 
                  ST77XX_WHITE); 
      // tft.fillRect(15 + i * COL, 
      //              row + 30 + 1 - 20 * aaa[i], 
      //              2, 
      //              20 * aaa[i], 
      //              ST77XX_WHITE); 
    }          
  }

  // druck
  if (tft_hourly == true) {
    max = 0; min = hourly->pressure[0];
    for (int i = 0; i < MAX; i++) {
      if (max < hourly->pressure[i]) max = hourly->pressure[i];
      if (min > hourly->pressure[i]) min = hourly->pressure[i];
    }
  } else {
    max = 0; min = hourly->pressure[0];
    for (int i = 0; i < MAX; i++) {
      if (max < daily->pressure[i]) max = daily->pressure[i];
      if (min > daily->pressure[i]) min = daily->pressure[i];
    }
  }
  imin = min; imax = max; // + 1;

  if (imin == imax) { imax += 1; }

  tft.setTextColor (ST77XX_MAGENTA);
  row = 116;
  tft.setCursor (0, row);
  tft.print ("Druck: ");
  tft.print (current->pressure, 0);
  tft.println ("hPa");

  tft.setCursor (imax < 1000 ? 110 : 104, row);
  tft.print (imax);
  tft.setCursor (imin < 1000 ? 110 : 104, row + 24);
  tft.print (imin);

  for (int i = 0; i < MAX_HOURS; i++) {
    if (tft_hourly == true) {
      tft.drawFastHLine(0 + i * COL, row + 30 - 20 * ((hourly->pressure[i] - imin) / (imax - imin)), COL, ST77XX_MAGENTA);
      // Serial.println("pressure");
      // Serial.println(i);
      // Serial.println(hourly->pressure[i]);
      // Serial.println(row + 5 - 20 * ((hourly->pressure[i] - imin) / (imax - imin)));
      //tft.drawFastHLine(0 + i * COL, row + 5 - 20 * ((hourly->pressure[i] - imin) / (imax - imin)), COL, ST77XX_MAGENTA);
      //tft.drawFastHLine(0 + i * 16, row + 30 - 20 * (i % 2 ? 1 : 0), 16, ST77XX_GREEN);
    } else {
      tft.drawFastHLine(0 + i * COL, row + 30 - 20 * ((daily->pressure[i] - imin) / (imax - imin)), COL, ST77XX_MAGENTA);
    }
  }
}
 
void tftWeather () 
{ 
  tft.setCursor (0, 10);
  //tft.fillScreen (ST77XX_BLACK);
  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);
  tft.println ("Temperatur:");
  tft.setCursor (0, 20);
  tft.setTextSize (2);
  tft.setTextColor (ST77XX_GREEN);
  if (current->temp > 0) tft.print ("+");
  tft.print (current->temp, 1);

  //tft.write (247);  // grad
  //tft.println ("C");

  // tft.setTextSize (1);
  // tft.print (" O");
  // tft.setTextSize (2);
  // tft.println ("C");

  tft.setCursor (0, 40);
  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);
  tft.println ("Luftfeuchtigkeit:");
  tft.setCursor (0, 50);
  tft.setTextSize (2);
  tft.setTextColor (ST77XX_YELLOW);
  tft.print (current->humidity);
  tft.println ("%");

  tft.setCursor (0, 70);
  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);
  tft.println ("Druck:");
  tft.setTextSize (2);
  tft.setCursor (0, 80);
  tft.setTextColor (ST77XX_ORANGE);
  tft.print (current->pressure, 1);
  tft.println (" hPa");

  tft.setCursor (0, 100);
  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);
  tft.println ("Windgeschwindigkeit:");
  tft.setTextSize (2);
  tft.setCursor (0, 110);
  tft.setTextColor (ST77XX_BLUE);
  //if (current->dew_point > 0) tft.print ("+");
  tft.print (current->wind_speed, 1);
  tft.print ("m/s ");
  tft.print (deg2ono(current->wind_deg));

//Serial.print("wind deg: ");
//Serial.print(current->wind_deg);

  tft.setCursor (0, 130);
  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);
  tft.println ("Regenmenge:");
  tft.setTextSize (2);
  tft.setCursor (0, 140);
  tft.setTextColor (ST77XX_MAGENTA);
  tft.print (current->rain, 2);
  tft.println ("mm");
}

String deg2ono(float d) {

  //Serial.print("deg: ");
  //Serial.println(d);
  if (d < 11.25 || d >= 348.75) return "N";
  if (d < 33.75 && d >= 11.25) return "NNO";
  if (d < 56.25 && d >= 33.75) return "NO";
  if (d < 78.75 && d >= 56.25) return "ONO";
  if (d < 101.25 && d >= 78.75) return "O";
  if (d < 123.75 && d >= 101.25) return "OSO";
  if (d < 146.25 && d >= 123.75) return "SO";
  if (d < 168.75 && d >= 146.25) return "SSO";
  if (d < 191.25 && d >= 168.75) return "S";
  if (d < 213.75 && d >= 191.25) return "SSW";
  if (d < 236.25 && d >= 213.75) return "SW";
  if (d < 258.75 && d >= 236.25) return "WSW";
  if (d < 281.25 && d >= 258.75) return "W";
  if (d < 303.75 && d >= 281.25) return "WNW";
  if (d < 326.25 && d >= 303.75) return "NW";
  if (d < 348.75 && d >= 326.25) return "NNW";
  return "XX";
  
}


void printCurrentWeather()
{
  // Create the structures that hold the retrieved weather
  //OW_current *current = new OW_current;
  //OW_hourly *hourly = new OW_hourly;
  //OW_daily  *daily = new OW_daily;

  Serial.println("\nRequesting weather information from OpenWeather... ");

  //On the ESP8266 (only) the library by default uses BearSSL, another option is to use AXTLS
  //For problems with ESP8266 stability, use AXTLS by adding a false parameter thus       vvvvv
  //ow.getForecast(current, hourly, daily, api_key, latitude, longitude, units, language, false);

  //ow.getForecast(current, hourly, daily, api_key, latitude, longitude, units, language);
  Serial.println("");
  Serial.println("Weather from Open Weather\n");

  // Position as reported by Open Weather
  Serial.print("Latitude            : "); Serial.println(ow.lat);
  Serial.print("Longitude           : "); Serial.println(ow.lon);
  // We can use the timezone to set the offset eventually...
  Serial.print("Timezone            : "); Serial.println(ow.timezone);
  Serial.println();

  if (current)
  {
    Serial.println("############### Current weather ###############\n");
    //Serial.print("dt (time)        : "); Serial.println(strTime(current->dt));
    //Serial.print("sunrise          : "); Serial.println(strTime(current->sunrise));
    //Serial.print("sunset           : "); Serial.println(strTime(current->sunset));
    Serial.print("temp             : "); Serial.println(current->temp);
    Serial.print("feels_like       : "); Serial.println(current->feels_like);
    Serial.print("pressure         : "); Serial.println(current->pressure);
    Serial.print("humidity         : "); Serial.println(current->humidity);
    Serial.print("dew_point        : "); Serial.println(current->dew_point);
    Serial.print("uvi              : "); Serial.println(current->uvi);
    Serial.print("clouds           : "); Serial.println(current->clouds);
    Serial.print("visibility       : "); Serial.println(current->visibility);
    Serial.print("wind_speed       : "); Serial.println(current->wind_speed);
    Serial.print("wind_gust        : "); Serial.println(current->wind_gust);
    Serial.print("wind_deg         : "); Serial.println(current->wind_deg);
    Serial.print("rain             : "); Serial.println(current->rain);
    //Serial.print("rain             : "); Serial.println(current->rain1h);
    Serial.print("snow             : "); Serial.println(current->snow);
    Serial.println();
    Serial.print("id               : "); Serial.println(current->id);
    Serial.print("main             : "); Serial.println(current->main);
    Serial.print("description      : "); Serial.println(current->description);
    Serial.print("icon             : "); Serial.println(current->icon);

    Serial.println();
  }

//  if (forecast)
//   {
//     Serial.println("###############  Forecast weather  ###############\n");
//     for (int i = 0; i < (MAX_DAYS * 8); i++)
//     {
//       Serial.print("3 hourly forecast   "); if (i < 10) Serial.print(" "); Serial.print(i);
//       Serial.println();
//       //Serial.print("dt (time)        : "); Serial.print(strTime(forecast->dt[i]));

//       Serial.print("temp             : "); Serial.println(forecast->temp[i]);
//       Serial.print("temp.min         : "); Serial.println(forecast->temp_min[i]);
//       Serial.print("temp.max         : "); Serial.println(forecast->temp_max[i]);

//       Serial.print("pressure         : "); Serial.println(forecast->pressure[i]);
//       Serial.print("sea_level        : "); Serial.println(forecast->sea_level[i]);
//       Serial.print("grnd_level       : "); Serial.println(forecast->grnd_level[i]);
//       Serial.print("humidity         : "); Serial.println(forecast->humidity[i]);

//       Serial.print("clouds           : "); Serial.println(forecast->clouds_all[i]);
//       Serial.print("wind_speed       : "); Serial.println(forecast->wind_speed[i]);
//       Serial.print("wind_deg         : "); Serial.println(forecast->wind_deg[i]);
//       Serial.print("wind_gust        : "); Serial.println(forecast->wind_gust[i]);

//       Serial.print("visibility       : "); Serial.println(forecast->visibility[i]);
//       Serial.print("pop              : "); Serial.println(forecast->pop[i]);
//       Serial.println();

//       Serial.print("dt_txt           : "); Serial.println(forecast->dt_txt[i]);
//       Serial.print("id               : "); Serial.println(forecast->id[i]);
//       Serial.print("main             : "); Serial.println(forecast->main[i]);
//       Serial.print("description      : "); Serial.println(forecast->description[i]);
//       Serial.print("icon             : "); Serial.println(forecast->icon[i]);

//       Serial.println();
//     }
//   }

  if (hourly)
  {
    Serial.println("############### Hourly weather  ###############\n");
    for (int i = 0; i < MAX_HOURS; i++)
    {
      Serial.print("Hourly summary  "); if (i < 10) Serial.print(" "); Serial.print(i);
      Serial.println();
      //Serial.print("dt (time)        : "); Serial.println(strTime(hourly->dt[i]));
      Serial.print("temp             : "); Serial.println(hourly->temp[i]);
      Serial.print("feels_like       : "); Serial.println(hourly->feels_like[i]);
      Serial.print("pressure         : "); Serial.println(hourly->pressure[i]);
      Serial.print("humidity         : "); Serial.println(hourly->humidity[i]);
      Serial.print("dew_point        : "); Serial.println(hourly->dew_point[i]);
      Serial.print("clouds           : "); Serial.println(hourly->clouds[i]);
      Serial.print("wind_speed       : "); Serial.println(hourly->wind_speed[i]);
      Serial.print("wind_gust        : "); Serial.println(hourly->wind_gust[i]);
      Serial.print("wind_deg         : "); Serial.println(hourly->wind_deg[i]);
      //Serial.print("rain             : "); Serial.println(hourly->rain[i]);
      Serial.print("rain             : "); Serial.println(hourly->rain1h[i]);
      Serial.print("snow             : "); Serial.println(hourly->snow[i]);
      Serial.println();
      Serial.print("id               : "); Serial.println(hourly->id[i]);
      Serial.print("main             : "); Serial.println(hourly->main[i]);
      Serial.print("description      : "); Serial.println(hourly->description[i]);
      Serial.print("icon             : "); Serial.println(hourly->icon[i]);
      Serial.print("pop              : "); Serial.println(hourly->pop[i]);

      Serial.println();
    }
  }

  // if (daily)
  // {
  //   Serial.println("###############  Daily weather  ###############\n");
  //   for (int i = 0; i < MAX_DAYS; i++)
  //   {
  //     Serial.print("Daily summary   "); if (i < 10) Serial.print(" "); Serial.print(i);
  //     Serial.println();
  //     // Serial.print("dt (time)        : "); Serial.println(strTime(daily->dt[i]));
  //     // Serial.print("sunrise          : "); Serial.println(strTime(daily->sunrise[i]));
  //     // Serial.print("sunset           : "); Serial.println(strTime(daily->sunset[i]));

  //     Serial.print("temp.morn        : "); Serial.println(daily->temp_morn[i]);
  //     Serial.print("temp.day         : "); Serial.println(daily->temp_day[i]);
  //     Serial.print("temp.eve         : "); Serial.println(daily->temp_eve[i]);
  //     Serial.print("temp.night       : "); Serial.println(daily->temp_night[i]);
  //     Serial.print("temp.min         : "); Serial.println(daily->temp_min[i]);
  //     Serial.print("temp.max         : "); Serial.println(daily->temp_max[i]);

  //     Serial.print("feels_like.morn  : "); Serial.println(daily->feels_like_morn[i]);
  //     Serial.print("feels_like.day   : "); Serial.println(daily->feels_like_day[i]);
  //     Serial.print("feels_like.eve   : "); Serial.println(daily->feels_like_eve[i]);
  //     Serial.print("feels_like.night : "); Serial.println(daily->feels_like_night[i]);

  //     Serial.print("pressure         : "); Serial.println(daily->pressure[i]);
  //     Serial.print("humidity         : "); Serial.println(daily->humidity[i]);
  //     Serial.print("dew_point        : "); Serial.println(daily->dew_point[i]);
  //     Serial.print("uvi              : "); Serial.println(daily->uvi[i]);
  //     Serial.print("clouds           : "); Serial.println(daily->clouds[i]);
  //     Serial.print("visibility       : "); Serial.println(daily->visibility[i]);
  //     Serial.print("wind_speed       : "); Serial.println(daily->wind_speed[i]);
  //     Serial.print("wind_gust        : "); Serial.println(daily->wind_gust[i]);
  //     Serial.print("wind_deg         : "); Serial.println(daily->wind_deg[i]);
  //     Serial.print("rain             : "); Serial.println(daily->rain[i]);
  //     //Serial.print("rain             : "); Serial.println(daily->rain1h[i]);
  //     Serial.print("snow             : "); Serial.println(daily->snow[i]);
  //     Serial.println();
  //     Serial.print("id               : "); Serial.println(daily->id[i]);
  //     Serial.print("main             : "); Serial.println(daily->main[i]);
  //     Serial.print("description      : "); Serial.println(daily->description[i]);
  //     Serial.print("icon             : "); Serial.println(daily->icon[i]);
  //     Serial.print("pop              : "); Serial.println(daily->pop[i]);

  //     Serial.println();
  //   }
  //}

  // Delete to free up space and prevent fragmentation as strings change in length
  //delete current;
  //delete hourly;
  //delete daily;
}
