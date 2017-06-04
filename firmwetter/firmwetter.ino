char * wl_ssid = "klo.kla";
char * wl_passwd = "nummidummiblob";

char * wl_ssid2 = "klo.klatwo";
char * wl_passwd2 = "nummidummi-blob2";

#define REFRESH_RATE     45
#define MIDDLE_LENGTH    14

String wetter;

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

#define SCREENS   4

Infoscreen inf[SCREENS] = {
  { "krefeld", "Zwingenbergstr", "", {} },
  { "krefeld", "Heyenbaumstr", "", {} },
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
  delay(500);
  WiFiMulti.addAP(wl_ssid,  wl_passwd);
  WiFiMulti.addAP(wl_ssid2, wl_passwd2);
  delay(500);
  display.init();
  display.flipScreenVertically();
  display.setFont(DejaVu_Sans_Condensed_8);
}

void loop() {  
  if (seccount < 1) {
    seccount = REFRESH_RATE;
    
    // wait for WiFi connection
    if((WiFiMulti.run() == WL_CONNECTED)) {
        HTTPClient http;

        for (int j = 0; j < SCREENS; ++j) {
          http.begin(
            String("https://vrrf.finalrewind.org/")+inf[j]._ort+String("/")+inf[j]._stop+String(".html?frontend=html"),
            "29:50:EC:E8:76:66:09:59:89:F0:F1:39:C4:4E:4E:46:F8:D5:77:36"
          );
          int httpCode = http.GET();
          // httpCode will be negative on error
          if(httpCode > 0) {
              if(httpCode == HTTP_CODE_OK) {
                  inf[j]._raw = http.getString();
                  extractP(j);
              }
          }
        }

        http.begin("http://dummer.click/_p/wdrWetter/?text=true");
        int httpCode = http.GET();
        // httpCode will be negative on error
        if(httpCode > 0 && httpCode == HTTP_CODE_OK) wetter = http.getString();
        http.end();
    }
  }
  
  display.clear();
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  if (infoScreen == SCREENS) {
    
    display.drawStringMaxWidth(0, 3, 128, wetter);
    
  } else {
    
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
  if (infoScreen > SCREENS) infoScreen=0;
}

