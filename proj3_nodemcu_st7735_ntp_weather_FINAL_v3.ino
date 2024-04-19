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
//#include <NTPClient.h>
#include <ESP8266WiFi.h>
//#include <WiFiUdp.h>

//const long utcOffsetInSeconds = 3600 * 2;

String weekDays[7]={"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

// Define NTP Client to get time
//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// openweathermap
#include <WiFiClientSecure.h>
#include <JSON_Decoder.h>
#include <OpenWeather.h>

#define MAX_HOURS 8 // edit /libraries/OpenWeather/User_Setup.h
#define MAX_DAYS 6 // edit /libraries/OpenWeather/User_Setup.h

#define ST77XX_LIGHTBLUE 0x5571    // https://rgbcolorpicker.com/565
#define ST77XX_GREENBLUE 0xc7e9

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

#include <time.h>   
// #include <TZ.h>
// #define MYTZ TZ_Europe_Berlin
// configTime(MYTZ, "pool.ntp.org");
#define MY_NTP_SERVER "pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   
time_t now; // this are the seconds since Epoch (1970) - UTC
tm tm;      // the structure tm holds time information in a more convenient way


// Wi-Fi connection parameters.
// It will be read from the flash during setup.
struct WifiConf {
  char ssid[32];
  char password[64];
  char api[36];
  char lat[10];
  char lng[10];
 
  // Make sure that there is a 0 
  // that terminatnes the c string
  // if memory is not initalized yet.
  char cstr_terminator = 0; // makse sure
};
WifiConf wifiConf;

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
  Serial.println("Startup");

  tft.initR (INITR_BLACKTAB); 
  tft.fillScreen (ST77XX_BLACK);

  tft.drawBitmap(0, 0, open_icon, 128, 59, ST7735_ORANGE);

  tft.setCursor (0, 70);
  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);

  // read eeprom for ssid and pass
  readWifiConf();

  configTime(MY_TZ, MY_NTP_SERVER); // --> Here is the IMPORTANT ONE LINER needed in your sketch!

  if ( wifiConf.ssid == 0 || wifiConf.password == 0 || wifiConf.lat == 0 || wifiConf.lng == 0 ) {  // fixme
    Serial.println("-->ssid low");
    setupAP();
    //return;
  }

  tft.setTextWrap (true);
  tft.print ("Wifi: ");
  tft.println (wifiConf.ssid);

  WiFi.begin(wifiConf.ssid, wifiConf.password);
  Serial.println("-->wifi begin");

  unsigned long currentMillis = millis();
  while ( WiFi.status() != WL_CONNECTED ) {
  
    digitalWrite(LED_BUILTIN, LOW);
    delay ( 250 );
    digitalWrite(LED_BUILTIN, HIGH);
    delay ( 250 );

    if (digitalRead(BUTTON_PIN) == HIGH) { // button pressed
      Serial.println("-->button low");
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
  
  //timeClient.begin();
  //timeClient.update();

  // openweathermap
  ow.getForecast(current, hourly, daily, wifiConf.api, wifiConf.lat, wifiConf.lng, units, language);
  //ow.getForecast(forecast, api_key, latitude, longitude, units, language);
  tft.print (")");

  // wassertipps
  wassertipps(wifiConf.lat, wifiConf.lng);
  tft.print (")");
  
  tftWeatherMore();
  
  Serial.println("OK");
}

void readWifiConf() {
  // Read wifi conf from flash
  for (int i=0; i<sizeof(wifiConf); i++) {
    ((char *)(&wifiConf))[i] = char(EEPROM.read(i));
  }
  // Make sure that there is a 0 
  // that terminatnes the c string
  // if memory is not initalized yet.
  wifiConf.cstr_terminator = 0;
}

void writeWifiConf() {
  for (int i=0; i<sizeof(wifiConf); i++) {
    EEPROM.write(i, ((char *)(&wifiConf))[i]);
  }
  EEPROM.commit();
}

void wassertipps(String latitude, String longitude) {
  // wassertipps
  String url = "https://www.wassertipps.de/cgi-bin/server_api2.pl?json_data={\"lon\":" + longitude + ",\"lat\":" + latitude + "}";

  //Serial.println(url);


//https://randomnerdtutorials.com/esp32-http-get-post-arduino/
// String httpGETRequest(const char* url) {
//   HTTPClient http;

//   // Your IP address with path or Domain name with URL path 
//   http.begin(url);
//   http.begin(client, url);

//   // If you need Node-RED/server authentication, insert user and password below
//   //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");


//   // Send HTTP POST request
//   int httpResponseCode = http.GET();

//   String payload = "{}"; 

//   if (httpResponseCode>0) {
//     Serial.print("HTTP Response code: ");
//     Serial.println(httpResponseCode);
//     payload = http.getString();
//   }
//   else {
//     Serial.print("Error code: ");
//     Serial.println(httpResponseCode);
//   }
//   // Free resources
//   http.end();

//   return payload;
// }






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
  
  if (strlen(ap_password) != 0) {
     tft.print ("   PASS: ");
     tft.println (ap_password);
  }

  tft.setTextColor (ST77XX_YELLOW);
  tft.print ("2. http://"); http://
  tft.println (local_ip);

  tft.setTextColor (ST77XX_WHITE);
  tft.println ("3. Dein SSID/Passwort");
  
  //tft.println ("4. Reboot ");
  
  while (1) server.handleClient();
} 

void generateQRCode(String text, byte shiftX, byte shiftY, int color) {
  // Create a QR code object
  QRCode qrcode;
  
  // Define the size of the QR code (1-40, higher means bigger size)
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, text.c_str());

  // Calculate the scale factor
  int scale = min(OLED_WIDTH / qrcode.size, OLED_HEIGHT / qrcode.size);

  // Draw the QR code on the display
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(shiftX+x * scale, shiftY + y*scale, scale, scale, color);
      }
    }
  }
}

///////////////////////////////////////////////////////////

void handle_OnConnect() {

  String s = MAIN_page;
  server.send(200, "text/html", s); 

}

void handle_OnWifi() {
  bool save = false;

  if (server.hasArg("ssid") && server.hasArg("password")) {
    server.arg("ssid").toCharArray(wifiConf.ssid, sizeof(wifiConf.ssid));
    server.arg("password").toCharArray(wifiConf.password, sizeof(wifiConf.password));
    server.arg("api").toCharArray(wifiConf.api, sizeof(wifiConf.api));
    server.arg("lat").toCharArray(wifiConf.lat, sizeof(wifiConf.lat));
    server.arg("lng").toCharArray(wifiConf.lng, sizeof(wifiConf.lng));
           
    writeWifiConf();
    save = true;
  }

  String s = MAIN_page;

  // Serial.println("Wifi: ");
  // Serial.println(wifiConf.ssid);
  // Serial.println(wifiConf.password);
  //Serial.println(wifiConf.api);
  
  server.send(200, "text/plain", "Success"); 
  if (save) {
    Serial.println("Wi-Fi conf saved. Rebooting...");
    delay(1000);
    ESP.restart();
  }
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

////////////////////////////////////////////////////////
unsigned long currentMillis;
bool tft_hourly = true;
byte tft_stage = 1; // daily?

void loop() {

  //int light = analogRead(PHOTO_PIN);
  //Serial.println("light: " + (String) light);

  if (analogRead(PHOTO_PIN) < 120) { // fixme
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
    
    ow.getForecast(current, hourly, daily, wifiConf.api, wifiConf.lat, wifiConf.lng, units, language);
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
  //timeClient.update();
  
  tftDateTime();

  delay (250);

}

void tftDateTime ()
{
  time(&now);
  localtime_r(&now, &tm); 
  // ntp -> date/time
  // time_t epochTime = timeClient.getEpochTime();
  // struct tm *ptm = gmtime ((time_t *)&epochTime);

  tft.setCursor (0, 0);
  //tft.fillScreen (ST77XX_BLACK);
  tft.fillRect(0, 0, 59, 8, ST77XX_BLACK); // tip!
  if (tm.tm_hour == 0 && tm.tm_min == 0 && tm.tm_sec == 0) tft.fillRect(60, 0, 128, 8, ST77XX_BLACK); // tip!

  tft.setTextColor (ST77XX_WHITE);
  tft.setTextSize (1);

  // int monthDay = tm.tm_mday;
  // int currentMonth = tm.tm_mon+1;
  // int currentYear = tm.tm_year+1900;

  // String formattedDate = (tm.tm_mday < 10 ? "0" : "") + String(tm.tm_mday) + "." 
  //                      + (tm.tm_mon+1 < 10 ? "0" : "") + String(tm.tm_mon+1) + "." 
  //                      + String(tm.tm_year+1900 - 2000);
  //String formattedTime = timeClient.getFormattedTime();
  // int hh = tm.tm_hour;
  // int mm = tm.tm_min;
  // int ss = tm.tm_sec;

  // String formattedTime = (tm.tm_hour < 10 ? "0" : "") + String(tm.tm_hour) + ":" 
  //                      + (tm.tm_min < 10 ? "0" : "") + String(tm.tm_min) + ":" 
  //                      + (tm.tm_sec < 10 ? "0" : "") + String(tm.tm_sec);
  
  //String weekDay = weekDays[tm.tm_wday];

  if (tm.tm_hour < 10) tft.print ("0");
  tft.print (tm.tm_hour);
  tft.print (":");
  if (tm.tm_min < 10) tft.print ("0");
  tft.print (tm.tm_min);
  tft.print (":");
  if (tm.tm_sec < 10) tft.print ("0");
  tft.print (tm.tm_sec);

  //tft.print (formattedTime);
  tft.setCursor (60, 0);
  tft.print (weekDays[tm.tm_wday]);
  tft.print (",");
  tft.setCursor (80, 0);
  //tft.println (formattedDate);

  if (tm.tm_mday < 10) tft.print ("0");
  tft.print (tm.tm_mday);
  tft.print (".");
  if (tm.tm_mon+1 < 10) tft.print ("0");
  tft.print (tm.tm_mon+1);
  tft.print (".");
  tft.print (tm.tm_year+1900 - 2000);

}

void tftWassertipps () 
{ 
  // PLZ und Ort
  byte row = 14;
  tft.fillScreen (ST77XX_BLACK);
  tft.setTextColor (ST77XX_ORANGE);
  
  tft.setCursor (0, row);
  tft.println (doc[0]["zip"].as<const char*>());

  tft.setTextColor (ST77XX_WHITE);
  tft.setCursor (0, row+10);
  tft.println (doc[0]["city"].as<const char*>());

  // Wasserhaerte
  row = 32;
  tft.setCursor (0, row);
  tft.setTextColor (ST77XX_LIGHTBLUE);
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
   
  if (wasserhaerte_min > 0) tft.drawFastVLine(wasserhaerte_min * 128 / 22, row+20, 7, ST77XX_LIGHTBLUE);
  if (wasserhaerte_max > 0) tft.drawFastVLine(wasserhaerte_max * 128 / 22, row+20, 7, ST77XX_LIGHTBLUE);
  if (wasserhaerte_avg > 0) tft.drawFastVLine(wasserhaerte_avg * 128 / 22, row+20, 7, ST77XX_LIGHTBLUE);

  if (wasserhaerte_min > 0 && wasserhaerte_max > 0) {
    tft.drawFastHLine(wasserhaerte_min * 128 / 22 + 1, row+20 + 2, (wasserhaerte_max - wasserhaerte_min) * 128 / 22, ST77XX_LIGHTBLUE);
    tft.drawFastHLine(wasserhaerte_min * 128 / 22 + 1, row+20 + 3, (wasserhaerte_max - wasserhaerte_min) * 128 / 22, ST77XX_LIGHTBLUE);
    tft.drawFastHLine(wasserhaerte_min * 128 / 22 + 1, row+20 + 4, (wasserhaerte_max - wasserhaerte_min) * 128 / 22, ST77XX_LIGHTBLUE);
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

  // Nitrat
  row = 80;
  tft.setCursor (0, row);
  tft.setTextColor (ST77XX_GREENBLUE);
  tft.println();
  tft.println("Nitrat:");

  float nitrat_min = doc[0]["nitrat_min"];
  float nitrat_avg = doc[0]["nitrat_avg"];
  float nitrat_max = doc[0]["nitrat_max"];

  if (nitrat_min > 0) tft.drawFastVLine(nitrat_min * 128 / 50, row+20, 7, ST77XX_GREENBLUE);
  if (nitrat_max > 0) tft.drawFastVLine(nitrat_max * 128 / 50, row+20, 7, ST77XX_GREENBLUE);
  if (nitrat_avg > 0) tft.drawFastVLine(nitrat_avg * 128 / 50, row+20, 7, ST77XX_GREENBLUE);

  if (nitrat_min > 0 && nitrat_max > 0) {
    tft.drawFastHLine(nitrat_min * 128 / 50 + 1, row+20 + 2, (nitrat_max - nitrat_min) * 128 / 50, ST77XX_GREENBLUE);
    tft.drawFastHLine(nitrat_min * 128 / 50 + 1, row+20 + 3, (nitrat_max - nitrat_min) * 128 / 50, ST77XX_GREENBLUE);
    tft.drawFastHLine(nitrat_min * 128 / 50 + 1, row+20 + 4, (nitrat_max - nitrat_min) * 128 / 50, ST77XX_GREENBLUE);
  }

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

  // Nitratwerte:
  //  0 - 10 mg/l
  //  10 - 25 mg/l
  //  25 - 50 mg/l
  
}

void tftWeatherMore () 
{ 
  time(&now);
  localtime_r(&now, &tm); 

  // ntp -> date/time
  // time_t epochTime = timeClient.getEpochTime();
  // struct tm *ptm = gmtime ((time_t *)&epochTime);
  // int monthDay = tm.tm_mday;
  // int currentMonth = tm.tm_mon+1;
  // int currentYear = tm.tm_year+1900;

  // String formattedDate = (monthDay < 10 ? "0" : "") + String(monthDay) + "." 
  //                      + (currentMonth < 10 ? "0" : "") + String(currentMonth) + "." 
  //                      + String(currentYear - 2000);
  //String formattedTime = timeClient.getFormattedTime();
  // int hh = tm.tm_hour;
  // int mm = tm.tm_min;
  // int ss = tm.tm_sec;

  tft.fillScreen (ST77XX_BLACK);

  byte MAX; byte COL;
  if (tft_hourly == true) { 
    Serial.println("HOURLY----------"); 
    tft_stage = 0;
    MAX = 8;
    COL = 16;
  } 
  else { 
    Serial.println("DIALY----------"); 
    tft_stage = 1;
    MAX = 6;
    COL = 22;
  }

  byte row;
  row = 152;
  tft.fillRect(0, row, 128, 10, ST77XX_BLACK);
  
  tft.setTextColor (ST77XX_WHITE);

  if (tft_hourly == true) {
    // hour
    for (int i = 0; i < 8; i++) {
      //tft.setTextColor (ST77XX_WHITE);
      tft.setCursor (2 + i * COL, row);
      if ((tm.tm_hour + i) % 24 < 10) tft.print (" ");
      tft.print ((tm.tm_hour + i) % 24);
    }
  } else { 
    // daily
    for (int i = 0; i < 6; i++) {
      //tft.setTextColor (ST77XX_WHITE);
      tft.setCursor (4 + i * COL, row);
      //if ((timeClient.getDay() + i) % 7 < 10) tft.print (" ");
      tft.print (weekDays[(tm.tm_wday + i) % 7]);
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
