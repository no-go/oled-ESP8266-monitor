char * wl_ssid1 = "klo.kla";
char * wl_passwd1 = "nummidummiblob";

char * wl_ssid2 = "klo.klatwo";
char * wl_passwd2 = "nummidummi-blob2";

#define REFRESH_RATE     45
#define MIDDLE_LENGTH    14
#define GMTMOD            2

String wetter;
String dateHeader;

int hours,minutes,seconds;

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
  WiFiMulti.addAP(wl_ssid1, wl_passwd1);
  WiFiMulti.addAP(wl_ssid2, wl_passwd2);
  delay(500);
  display.init();
  display.flipScreenVertically();
  //http.setReuse(true);
  
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

        if (minutes%5 == 0) {
          http.begin("http://dummer.click/_p/wdrWetter/?text=true");
          int httpCode = http.GET();
          // httpCode will be negative on error
          if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
            dateHeader = http.header("Date");
            hours   = dateHeader.substring(17,19).toInt() + GMTMOD;
            minutes = dateHeader.substring(20,22).toInt();
            seconds = dateHeader.substring(23,25).toInt();
            wetter = http.getString();
          }
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
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    if (hours<10) {
      display.drawString(22, 36, "0");
      display.drawString(34, 36, String(hours));
    } else {
      display.drawString(22, 36, String(hours));
    }
    
    if (minutes<10) {
      display.drawString(52, 36, "0");
      display.drawString(64, 36, String(minutes));
    } else {
      display.drawString(52, 36, String(minutes));
    }
    
    if (seconds<10) {
      display.drawString(83, 36, "0");
      display.drawString(95, 36, String(seconds));
    } else {
      display.drawString(83, 36, String(seconds));
    }
    
  } else {
    
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(DejaVu_Sans_Condensed_8);
    display.drawString(0, 0, inf[infoScreen]._stop);
    
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    if (hours<10) {
      display.drawString( 99, 0, "0");
      display.drawString(107, 0, String(hours));
    } else {
      display.drawString(107, 0, String(hours));
    }

    if (seconds%2 == 0) display.drawString(112, 0, ":");
    
    if (minutes<10) {
      display.drawString(120, 0, "0");
      display.drawString(128, 0, String(minutes));
    } else {
      display.drawString(128, 0, String(minutes));
    }

    display.setPixel(10,10);
    display.setPixel(20,10);
    display.setPixel(30,10);
    display.setPixel(40,10);
    display.setPixel(50,10);
    display.setPixel(60,10);
    display.setPixel(70,10);
    display.setPixel(80,10);
    display.setPixel(90,10);
    display.setPixel(100,10);
    display.setPixel(110,10);
    display.setPixel(120,10);
    display.drawLine(0, 10, 128-(seccount*128.0/(float)REFRESH_RATE), 10);
  
    for (int i=0;i<5;++i) {
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0,  12+10*i, inf[infoScreen]._lines[i]._name);
      display.drawString(25, 12+10*i, inf[infoScreen]._lines[i]._dest);
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(128, 12+10*i, inf[infoScreen]._lines[i]._time);
    }
  }
  display.display();

  delay(1000);
  --seccount;
  seconds++;
  if (seconds >= 60) {
    minutes++;
    seconds = seconds%60;
  }
  if (minutes >= 60) {
    hours++;
    minutes = minutes%60;
  }
  if (hours >= 24) {
    hours = hours%24;
  }
  if (seccount%5 == 0) infoScreen++;
  if (infoScreen > (SCREENS+1)) infoScreen=0;
}

