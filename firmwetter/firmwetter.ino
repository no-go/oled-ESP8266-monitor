char * wl_ssid = "klo.kla";
char * wl_passwd = "nummidummiblob";

char * wl_ssid2 = "klo.klatwo";
char * wl_passwd2 = "nummidummi-blob2";

#define REFRESH_RATE     45
#define MIDDLE_LENGTH    14

String wetter;
String dateHeader;

struct Line {
  String _name;
  String _dest;
  String _time;
};

struct Infoscreen {
  String _ort;
  String _stop;
  String _raw;
  Line _lines[5]; 
};

#define SCREENS   2

Infoscreen inf[SCREENS] = {
  { "Duesseldorf", "Uni-Mitte", "", {} },
  { "Duesseldorf", "Botanischer-Garten", "", {} }
};

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;

int seccount = 5;
int infoScreen = 0;

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include "SSD1306Spi.h"
#include "font.h"

// Initialize the OLED display using SPI
// GPIO 14 D5 -> CLK
// GPIO 13 D7 -> MOSI (DOUT)
// GPIO 16 D0 -> RES
// GPIO  4 D2 -> DC
// GPIO 15 D8 -> CS
SSD1306Spi        display(D0, D2, D8);

ESP8266WiFiMulti  WiFiMulti;
HTTPClient http;

void extractP(int id) {
  String cutting;
  int frompos;
  int totpos = 0;

  for (int i=0;i<5;++i) {
    cutting = inf[id]._raw.substring(totpos);
    frompos = cutting.indexOf("<div class=\"line\">")+18;
    totpos += frompos;
    cutting = cutting.substring(frompos);  
    frompos = cutting.indexOf("</div>");
    totpos += frompos;
    inf[id]._lines[i]._name = cutting.substring(0, frompos);
    inf[id]._lines[i]._name.trim();
    
    frompos = cutting.indexOf("<div class=\"dest\">")+18;
    totpos += frompos;
    cutting = cutting.substring(frompos);
    frompos = cutting.indexOf("</div>");
    totpos += frompos;
    inf[id]._lines[i]._dest = cutting.substring(0, frompos);
    inf[id]._lines[i]._dest.trim();
    if (inf[id]._lines[i]._dest.length() > MIDDLE_LENGTH) {
      inf[id]._lines[i]._dest = inf[id]._lines[i]._dest.substring(0,MIDDLE_LENGTH);
    }
    
    frompos = cutting.indexOf("<div class=\"time\">")+18;
    totpos += frompos;
    cutting = cutting.substring(frompos);
    frompos = cutting.indexOf("</div>");
    totpos += frompos;
    inf[id]._lines[i]._time = cutting.substring(0, frompos);
    inf[id]._lines[i]._time.trim();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  WiFiMulti.addAP(wl_ssid,  wl_passwd);
  WiFiMulti.addAP(wl_ssid2, wl_passwd2);
  delay(500);
  display.init();
  display.flipScreenVertically();
  const char* headerNames[] = { "Date", "Content-Type" };
  http.collectHeaders(headerNames, 2);
  http.setUserAgent("User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:12.0) Gecko/20100101 Firefox/12.0");
}


void loop() {  
  if (seccount < 1) {
    seccount = REFRESH_RATE;
    
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {
        for (int j = 0; j < SCREENS; ++j) {
          http.begin(
            String("https://vrrf.finalrewind.org/")+inf[j]._ort+String("/")+inf[j]._stop+String(".html?frontend=html"),
            "29:A8:9A:E8:23:A4:C3:9E:FF:D9:91:11:D8:53:56:FF:95:53:07:9B"
          );
          int httpCode = http.GET();
          // httpCode will be negative on error
          if(httpCode > 0) {
              if(httpCode == HTTP_CODE_OK) {
                  inf[j]._raw = http.getString();
                  extractP(j);
              }
          } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            display.clear();
            display.setTextAlignment(TEXT_ALIGN_LEFT);
            display.setFont(DejaVu_Sans_Condensed_8);
            display.drawStringMaxWidth(0, 3, 128, http.errorToString(httpCode).c_str());          
            display.display();
            delay(1000);
          }
        }

        http.begin("http://dummer.click/_p/wdrWetter/?text=true");
        int httpCode = http.GET();
        // httpCode will be negative on error
        if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
          dateHeader = http.header("Date");
          wetter = http.getString();
        }
        http.end();
    }
  }
  
  display.clear();
  
  if (infoScreen == SCREENS) {
    
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(DejaVu_Sans_Condensed_8);
    display.drawStringMaxWidth(0, 3, 128, wetter);
    
  } else if (infoScreen == (SCREENS+1)) {
    
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 4, dateHeader.substring(0,11));
    display.setFont(ArialMT_Plain_24);
    display.drawString(64, 36, dateHeader.substring(17,22) + String(" ") + dateHeader.substring(25));
    
  } else {
    
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(DejaVu_Sans_Condensed_8);
    display.drawString(0, 0, inf[infoScreen]._stop);
    
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 0, String(seccount));
    
    display.drawLine(0, 10, 128, 10);
  
    for (int i=0;i<5;++i) {
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0,  11+10*i, inf[infoScreen]._lines[i]._name);
      display.drawString(25, 11+10*i, inf[infoScreen]._lines[i]._dest);
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(128, 11+10*i, inf[infoScreen]._lines[i]._time);
    }
  }
  display.display();

  delay(1000);
  --seccount;
  if (seccount%5 == 0) infoScreen++;
  if (infoScreen > (SCREENS+1)) infoScreen=0;
}
