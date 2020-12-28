#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

//----------------------------------------------------------------------------
const char *ssid = "NAME_OF_DEVICE";
const char *password = "PASSWORD"; //not used
ESP8266WebServer server ( 80 );
//----------------------------------------------------------------------------

RTC_DS3231 rtc;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int sda_gpio = 3; //RX
const int scl_gpio = 1; //TX

const int button1_gpio = 0;
const int button2_gpio = 2;
const int relay_gpio = 12;

const byte TIMER_MODE = 1;
const byte MANUAL_MODE = 2;
byte MODE1 = TIMER_MODE;
byte STATE1 = 0;

int button1State = 1;
int button2State = 1;

int starthr1 =  18;
int startmin1 = 0;
int endhr1 = 6;
int endmin1 = 0;

uint32_t unixstart1, unixend1;
int currstate1 = 0, prevstate1 = 0;

//----------------------------------------------------------------------------
//Edit colours and names here for your own webpage 
void handleRoot() {
  server.send(200, "text/html",
              "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Timer Relay</title>"
              "<script>"
              "var server = '192.168.4.1';"
              "var xhttp = new XMLHttpRequest();"
              "xhttp.onreadystatechange = function() {"
              "if (this.readyState == 4 && this.status == 200) {"
              "var resp = xhttp.responseText.split(',');"
              "document.getElementById('mode1').innerHTML = resp[0];"
              "if(resp[0] == 'TIMER') document.getElementsByName('m1')[0].checked = true;"
              "else document.getElementsByName('m1')[1].checked = true;"
              "if(resp[1] == '0') document.getElementsByName('r1')[0].checked = true;"
              "else document.getElementsByName('r1')[1].checked = true;"
              "document.getElementById('s1').value = pad(resp[2], 2) + ':' + pad(resp[3], 2);"
              "document.getElementById('e1').value = pad(resp[4], 2) + ':' + pad(resp[5], 2);"
              "document.getElementById('ns1').innerHTML = '&nbsp;&nbsp;' + pad(resp[6], 2);"
              "document.getElementById('ne1').innerHTML = '&nbsp;&nbsp;' + pad(resp[7], 2);"
              "document.getElementById('dtl').value = resp[10] + '-' + resp[9] + '-' + resp[8] + 'T' + resp[11] + ':' + resp[12];"
              "  }"
              "var dt = new Date();"
              "document.getElementById('dtl2').value = dt.getFullYear() + '-' + pad((dt.getMonth() + 1), 2) + '-' + pad(dt.getDate(), 2) + 'T' + pad(dt.getHours(), 2) + ':' + pad(dt.getMinutes(), 2);"
              "document.getElementById('dtl2').disabled = true;"
              "};"
              "function manual(chk) {"
              " xhttp.open('GET', 'http://'+ server +'/manual?' + chk.name + '=' + chk.value, true);"
              "xhttp.send();"
              "}"
              "function setTimer(n) {"
              "var url = 'http://'+ server + '/timer?n=' + n;"
              "if(n == 1) {"
              "url += '&sh1=' + document.getElementById('s1').value.split(':')[0];"
              "url += '&sm1=' + document.getElementById('s1').value.split(':')[1];"
              "url += '&eh1=' + document.getElementById('e1').value.split(':')[0];"
              " url += '&em1=' + document.getElementById('e1').value.split(':')[1];"
              "} else {"
              "}"
              "xhttp.open('GET', url, true);"
              " xhttp.send();"
              " }"
              "function setMode(n, m) {"
              "var url = 'http://'+ server + '/mode?t=' + n + '&m=' + m.value;"
              "xhttp.open('GET', url, true);"
              "xhttp.send();"
              "}"
              "function setDateTime() {"
              "var dt = document.getElementById('dtl').value;"
              "console.log(dt);"
              " var d = dt.split('T')[0].split('-')[2];"
              " var M = dt.split('T')[0].split('-')[1];"
              " var y = dt.split('T')[0].split('-')[0];"
              " var h = dt.split('T')[1].split(':')[0];"
              " var m = dt.split('T')[1].split(':')[1];"
              " var s = dt.split('T')[1].split(':')[2];"
              " var url = 'http://'+ server +'/t?d=' + d + '&M=' + M + '&y=' + y;"
              " url += '&h=' + h + '&m=' + m + '&s=' + s;"
              " xhttp.open('GET', url, true);"
              " xhttp.send();"
              "}"
              "function setNow() {"
              "var dt = new Date();"
              "var url = 'http://'+ server +'/t?d=' + dt.getDate();"
              "url += '&M=' + (dt.getMonth() + 1) + '&y=' + dt.getFullYear();"
              "url += '&h=' + dt.getHours() + '&m=' + dt.getMinutes() + '&s=' + dt.getSeconds();"
              "xhttp.open('GET', url, true);"
              "xhttp.send();"
              "}"
              "function pad(num, size) {"
              "var s = num+'';"
              "while (s.length < size) s = '0' + s;"
              "return s;"
              "}"
              "function init() {"
              "xhttp.open('GET', 'http://'+ server +'/manual', true);"
              "xhttp.send();"
              "}"
              "</script>"
              "</head><body onLoad='init()' style='background-color:#222222;'><h3 style='color:#0cbfe9;'>Relay Timer</h3>"
              "<table>"
              "<tr><td style='color:white;'><B>Current Time</B></td><td style='color:white;'> : <input type='datetime-local' name='dtl2' id='dtl2'></td></tr>"
              "<tr><td style='color:white;'><B>Relay Time</B></td><td style='color:white;'> : <input type='datetime-local' name='dtl' id='dtl' onChange='setDateTime();'></td></tr>"
              "<tr><td></td><td><input type='button' value='Set to Current Date and Time' onClick='setNow()'></td></tr>"
              "</table><BR>"
              "<HR>"

              "<table border='0'>"
              "<tr><td style='color:white;'><B>Relay</B></td><td style='color:red;font-weight:bold;'><span style='color:black;font-weight:normal;'>: </span> <input type='radio' name='r1' value='0' checked onclick='manual(this);'> OFF  &nbsp;</td><td style='color:green;font-weight:bold;'><input type='radio' name='r1' value='1' onclick='manual(this);'> ON </td></tr>"
              "<tr><td style='color:white;'><B>Mode </B></td><td style='color:white;'>: <input type='radio' name='m1' value='1' checked onclick='setMode(1, this);'> TIMER  &nbsp;</td><td style='color:white;'><input type='radio' name='m1' value='2' onclick='setMode(1, this);'> MANUAL </td></tr>"
              "</table>"
              "<BR>"
              "<table  border='0'>"
              "<tr><td colspan='2' id='mode1' style='color:#0cbfe9;font-weight:bold'>TIMER</td><td style='color:#0cbfe9;'><B>&nbsp;&nbsp;Next Start/End</B></td></tr>"
              "<tr><td style='color:white;'><B>Start</B> </td><td>: <input type='time' name='s1' id='s1' onChange='setTimer(1)'></td><td style='color:white;' id='ns1'>nextStart1</td></tr>"
              "<tr><td style='color:white;'><B>End</B> </td><td>: <input type='time' name='e1' id='e1' onChange='setTimer(1)'></td><td style='color:white;' id='ne1'>nextEnd1</td></tr>"
              "</table>"
              "<BR><HR>"
              "</body></html>"
             );
}
//----------------------------------------------------------------------------
void handleManualONOFF() {
//  Serial.println("In handleManualONOFF( )");
  for (uint16_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "r1") {
      MODE1 =  MANUAL_MODE;
      EEPROM.write(5, MODE1);
//      Serial.print("MODE1 set to "); Serial.println(MODE1);
      String r1 = server.arg(i);
      STATE1 = atoi(r1.c_str());
      EEPROM.write(6, STATE1);
    }
  }
  server.send(200, "text/html", getResp());
}
//----------------------------------------------------------------------------
void handleMode() {
//  Serial.println("In handleMode( )");
  int m = 0;
  String t = "0";
  for (uint16_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "t") {
      t = server.arg(i);
    }
    if (server.argName(i) == "m") {
      m = atoi(server.arg(i).c_str());
    }
  }
  if (t.equals("1")) MODE1 = m;
  else;
  server.send(200, "text/html", getResp());
}
//----------------------------------------------------------------------------
void handleSetStartEndTimes() {
  int n = 0;
//  Serial.println("In handleSetStartEndTimes( )");
  for (uint16_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "n") {
      String nStr = server.arg(i);
      n = atoi(nStr.c_str());
    }
    if (server.argName(i) == "sh1") {
      String sh1 = server.arg(i);
      starthr1 = atoi(sh1.c_str());
    }
    if (server.argName(i) == "sm1") {
      String sm1 = server.arg(i);
      startmin1 = atoi(sm1.c_str());
    }
    if (server.argName(i) == "eh1") {
      String eh1 = server.arg(i);
      endhr1 = atoi(eh1.c_str());
    }
    if (server.argName(i) == "em1") {
      String em1 = server.arg(i);
      endmin1 = atoi(em1.c_str());
    }
  }
  if (n == 1) {
    MODE1 =  TIMER_MODE;
    EEPROM.write(5, MODE1);
  }
  else

  EEPROM.write(0, starthr1);
  EEPROM.write(1, startmin1);
  EEPROM.write(2, endhr1);
  EEPROM.write(3, endmin1);
  EEPROM.commit();
  calcUnixTime();
  server.send(200, "text/html", getResp());
}
//----------------------------------------------------------------------------
void handleSetTime() {
//  Serial.println("In handleSetTime( )");
  int y, M, d, h, m, s = 0;
  for (uint16_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "y") {
      String y1 = server.arg(i);
      y = atoi(y1.c_str());
    }
    if (server.argName(i) == "M") {
      String M1 = server.arg(i);
      M = atoi(M1.c_str());
    }
    if (server.argName(i) == "d") {
      String d1 = server.arg(i);
      d = atoi(d1.c_str());
    }
    if (server.argName(i) == "h") {
      String h1 = server.arg(i);
      h = atoi(h1.c_str());
    }
    if (server.argName(i) == "m") {
      String m1 = server.arg(i);
      m = atoi(m1.c_str());
    }
    if (server.argName(i) == "s") {
      String s1 = server.arg(i);
      s = atoi(s1.c_str());
    }
    // Year, month, date, hour, min, sec
    rtc.adjust(DateTime(y, M, d, h, m, s));
  }
  calcUnixTime();
  server.send(200, "text/html", getResp());
}
//----------------------------------------------------------------------------
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}
//----------------------------------------------------------------------------
String getResp() {
  DateTime now = rtc.now();
  String y = String(now.year(), DEC);
  String M = pad(String(now.month(), DEC));
  String d = pad(String(now.day(), DEC));
  String h = pad(String(now.hour(), DEC));
  String m = pad(String(now.minute(), DEC));
  String s = pad(String(now.second(), DEC));
  String currdate = d + " / " + M + " / " + y;
  String currtime = h + " : " + m + " : " + s;
  String strMode1 = "TIMER";
  if (MODE1 == MANUAL_MODE) strMode1 = "MANUAL";

  DateTime s1 = DateTime(unixstart1);
  DateTime e1 = DateTime(unixend1);
  String nextStart1 = getDateString(s1);
  String nextEnd1 = getDateString(e1);
  setStates();
  String resp = "";
  resp += strMode1 + "," + String(STATE1) + ",";
  resp += String(starthr1) + "," + String(startmin1) + "," + String(endhr1) + "," + String(endmin1) + "," ;
  resp += nextStart1 + "," + nextEnd1 + ",";
  resp += d + "," + M + "," + y + "," + h + "," + m + "," + s + "," + currdate + "," + currtime;
  return resp;
}
//----------------------------------------------------------------------------

void setup_wifi() {
  WiFi.softAP(ssid);

  IPAddress myIP = WiFi.softAPIP();
//  Serial.print("AP IP address: ");
//  Serial.println(myIP);

  if ( MDNS.begin ( "timerrelay" ) ) {
//    Serial.println ( "MDNS responder started" );
  }
}

//----------------------------------------------------------------------------

void calcUnixTime()
{
  DateTime now = rtc.now();

  DateTime startdt1 = DateTime(now.year(), now.month(), now.day(), starthr1, startmin1);
  DateTime enddt1   = DateTime(now.year(), now.month(), now.day(), endhr1,   endmin1);

  unixstart1 = startdt1.unixtime();
  unixend1   = enddt1.unixtime();
  if (unixend1 < unixstart1) unixend1 += (60 * 60 * 24);
  if (unixend1 <= now.unixtime()) {
    unixstart1 += (60 * 60 * 24);
    unixend1 += (60 * 60 * 24);
  }

  DateTime s1 = DateTime(unixstart1);
  DateTime e1 = DateTime(unixend1);

//  Serial.print("Now : "); Serial.println(getDateString(now));
//  Serial.print("Sta1: "); Serial.println(getDateString(s1));
//  Serial.print("End1: "); Serial.println(getDateString(e1));

}

//----------------------------------------------------------------------------
String getDateString(DateTime d) {
  return pad(String(d.day())) + "/" + pad(String(d.month())) + "/" + String(d.year()) + "  " + pad(String(d.hour())) + ":" + pad(String(d.minute()));
}
//----------------------------------------------------------------------------
String getTimeString(DateTime d) {
  return pad(String(d.hour())) + ":" + pad(String(d.minute())) + "  " + pad(String(d.day())) + "/" + pad(String(d.month()))  ;
}
//----------------------------------------------------------------------------
String pad(String v) {
  if (v.length() < 2) return ("0" + v);
  else return v;
}

//----------------------------------------------------------------------------

void setup () {

  //********** CHANGE PIN FUNCTION  TO GPIO **********
  //GPIO 1 (TX) swap the pin to a GPIO.
  pinMode(1, FUNCTION_3); 
  //GPIO 3 (RX) swap the pin to a GPIO.
  pinMode(3, FUNCTION_3); 
  //**************************************************

  Wire.begin(sda_gpio, scl_gpio); //SDA (RX - 3), SCL (TX - 1) For sonoff -> must disable serial port
  
//  Serial.begin(115200);
  Serial.end();
  //----------------------------------------------------------------------------
  setup_wifi();
  //----------------------------------------------------------------------------
  pinMode(relay_gpio, OUTPUT);
  digitalWrite(relay_gpio, LOW);
  pinMode(button1_gpio, INPUT_PULLUP);
  pinMode(button2_gpio, INPUT_PULLUP);

  delay(3000); // wait for console opening
  //----------------------------------------------------------------------------
  if (! rtc.begin()) {
//    Serial.println("Couldn't find RTC");
    while (1);
  }

  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (rtc.lostPower()) {
//    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

 if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
//    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  EEPROM.begin(512);
  starthr1 = EEPROM.read(0);
  startmin1 = EEPROM.read(1);
  endhr1 = EEPROM.read(2);
  endmin1 = EEPROM.read(3);
  currstate1 = EEPROM.read(4);
  MODE1 = EEPROM.read(5);
  STATE1 = EEPROM.read(6);

  calcUnixTime();
  //----------------------------------------------------------------------------
  server.on("/", handleRoot);
  server.on("/t", handleSetTime);
  server.on("/manual", handleManualONOFF);
  server.on("/timer", handleSetStartEndTimes);
  server.on("/mode", handleMode);
  server.onNotFound(handleNotFound);
  server.begin();
//  Serial.println("HTTP server started");
}
//----------------------------------------------------------------------------
void setStates() {
  DateTime now = rtc.now();
  uint32_t unixtime =  now.unixtime();

  prevstate1 = currstate1;

  if (MODE1 == TIMER_MODE) {
    if (unixtime >= unixstart1 && unixtime < unixend1) {
      digitalWrite( relay_gpio, HIGH);
      currstate1 = HIGH;
      STATE1 = 1;
    }
    else {
      digitalWrite( relay_gpio, LOW);
      currstate1 = LOW;
      STATE1 = 0;
    }
    EEPROM.write(6, STATE1);
    EEPROM.write(4, currstate1);
  }
  else digitalWrite( relay_gpio, STATE1);

}

void button1_callback() {

  // read the state of the pushbutton value
  button1State = digitalRead(button1_gpio);
//  Serial.print("Button 1 state is:"); Serial.println(button1State);
  prevstate1 = currstate1;
//  Serial.print("The mode is:"); Serial.println(MODE1);
  if (button1State == LOW) {
    if (MODE1 == TIMER_MODE) {
      
//      Serial.print("Relay state is:"); Serial.println(STATE1);
      MODE1 = MANUAL_MODE;
//      Serial.print("MODE1 changed to:"); Serial.println(MODE1);
      if (STATE1 == 1) {

        currstate1 = LOW;
        STATE1 = 0;
//        Serial.print("Relay changed to:"); Serial.println(STATE1);
      }
      else if (STATE1 == 0)  {

        currstate1 = HIGH;
        STATE1 = 1;
//        Serial.print("Relay changed to:"); Serial.println(STATE1);
      }
      EEPROM.write(6, STATE1);
      EEPROM.write(4, currstate1);
      EEPROM.write(5, MODE1);

      server.send(200, "text/html", getResp());
    }
    else if (MODE1 == MANUAL_MODE) {
      
      if (STATE1 == 1) {

        currstate1 = LOW;
        STATE1 = 0;
//        Serial.print("Relay changed to:"); Serial.println(STATE1);
      }
      else if (STATE1 == 0)  {

        currstate1 = HIGH;
        STATE1 = 1;
//        Serial.print("Relay changed to:"); Serial.println(STATE1);
      }
      EEPROM.write(6, STATE1);
      EEPROM.write(4, currstate1);
      EEPROM.write(5, MODE1);

      server.send(200, "text/html", getResp());
    }
  }
}

void button2_callback() {

  // read the state of the pushbutton value
  button2State = digitalRead(button2_gpio);
//  Serial.print("Button 1 state is:"); Serial.println(button2State);
//  Serial.print("The mode is:"); Serial.println(MODE1);
  if (button2State == LOW ) {
    if (MODE1 == TIMER_MODE) {
      
      MODE1 = MANUAL_MODE;
//      Serial.print("MODE1 changed to Manual");
      
      EEPROM.write(5, MODE1);

      server.send(200, "text/html", getResp());
    }
    else if (MODE1 == MANUAL_MODE) {
      
      MODE1 = TIMER_MODE;
//      Serial.print("MODE1 changed to Timer");
   
      EEPROM.write(5, MODE1);

      server.send(200, "text/html", getResp());
    }
  }
}

void display_callback() {

  DateTime now = rtc.now();
  DateTime s1 = DateTime(unixstart1);
  DateTime e1 = DateTime(unixend1);
  String timeNow = getTimeString(now);
  String nextStart1 = getTimeString(s1);
  String nextEnd1 = getTimeString(e1);
  
  //clear display
  display.clearDisplay();

  //display time
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Time: ");
  display.print(timeNow);

  //display mode and state
  display.setCursor(0,18);
  display.print("Mode: ");
  if(MODE1 == MANUAL_MODE) {
    display.print("Manual ");
  }
  else display.print("Timer  ");
  if(STATE1 == 1) {
    display.print("ON");
  }
  else display.print("OFF");

  //display on and off times
  display.setCursor(0, 36);
  display.print("On:   ");
  display.print(nextStart1);
  
  display.setCursor(0, 54);
  display.print("Off:  ");
  display.print(nextEnd1);
 
  
  display.display();
}

void loop () {
  server.handleClient();

  setStates();
  button1_callback();
  button2_callback();
  display_callback();
  
  if (prevstate1 == HIGH && currstate1 == LOW) {
//    Serial.println("Advancing unixtimes by 24 hrs for Timer1");
    unixstart1 += (60 * 60 * 24);
    unixend1   += (60 * 60 * 24);
  }

  EEPROM.write(4, currstate1);
  EEPROM.commit();

  delay(100);
}
