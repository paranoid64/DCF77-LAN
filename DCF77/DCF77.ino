#include "DCF77.h"
#include "TimeLib.h"
#include "DHT.h"
#include <EtherCard.h>

#include <LiquidCrystal_I2C.h>

#define DCF77_PIN 2          // Connection pin to DCF 77 device
#define DCF77_INTERRUPT 0    // Interrupt number associated with pin

#define PIN_LED 7
#define DHTPIN 4           // Digital pin connected to the DHT sensor

char humi[6];//Humidity
char temp[6];//Temperature as Celsius
char dcf77date[12];
char dcf77time[9];

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

static byte mac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
byte Ethernet::buffer[500];

LiquidCrystal_I2C lcd(0x3F,20,4);

DHT dht(DHTPIN, DHTTYPE);

DCF77 DCF = DCF77(DCF77_PIN, DCF77_INTERRUPT); // FALSE = inverted

// Some stuff for responding to the request
static char* on = "ON";
static char* off = "OFF";
//static char* statusLabel;
//static char* buttonLabel;
BufferFiller bfill;

static word homePage() {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n\r\n"
    "<html><head><meta http-equiv='refresh' content='10'/>"
    "<meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>"
    "<title>DCF77-LAN</title></head><body>"
    "<h1>Arduino DCF77</h1>"
    "<h3>Humi:</h3><p>$S%</p>"
    "<h3>Temp:</h3><p>$S°C</p>"
    "<h3>Time:</h3><p>$S</p>"
    "<h3>Date:</h3><p>$S</p>"
    "<p>LCD Background:</p>"   
    "<a href='?LED=ON'>ON</a>|<a href='?LED=OFF'>OFF</a><br></body>"    
    ), humi, temp, dcf77time, dcf77date);
  return bfill.position();

/*   | Format | Parameter   | Output
 *   |--------|-------------|----------
 *   | $D     | uint16_t    | Decimal representation
 *   | $H     | uint16_t    | Hexadecimal value of lsb (from 00 to ff)
 *   | $L     | long        | Decimal representation
 *   | $S     | const char* | Copy null terminated string from main memory
 *   | $F     | PGM_P       | Copy null terminated string from program space
 *   | $E     | byte*       | Copy null terminated string from EEPROM space
 *   | $$     | _none_      | '$'
 */
  
}

void setup() {
  
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH); // turn on pullup resistors
  pinMode(DCF77_PIN, INPUT);
  
  DCF.Start();
  lcd.init();
  dht.begin();

  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.clear();
  lcd.print(F("DCF77 TO LAN"));
  delay(5000);

  //Show MAC
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("MAC:"));
  
  lcd.setCursor(0,1);

  for (byte i = 0; i < 6; ++i) {
    lcd.print(mac[i], HEX);
    if (i < 5){
      lcd.print(':');
    }
  }
  
  delay(5000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("DHCP:"));

   // start the Ethernet connection and the server:
  lcd.setCursor(0,1);
  if (ether.begin(sizeof Ethernet::buffer, mac, 10) == 0){
    lcd.println(F("No Ethernet"));
  }
  else {
        if (!ether.dhcpSetup()){
          lcd.print(F("DHCP failed"));
        } else {
          printIP();                
        }
  }
  
  delay(2000);
  lcd.clear();
  
}

void loop() {
  time_t DCFtime = DCF.getTime(); // Check if new DCF77 time is available
  
  if (DCFtime!=0){
    setTime(DCFtime);
    lcd.clear();
    digitalWrite(PIN_LED, HIGH);
  } else {
    digitalWrite(PIN_LED, LOW);
  }
 
  digitalClockDisplay();
  web();
}

void digitalClockDisplay() {

  fdate();
  ftime();
  lcd.setCursor(0,0);
  lcd.print(dcf77time);
  lcd.setCursor(0,1);
  lcd.print(dcf77date);

  fhumi();
  ftemp();
  lcd.setCursor(0,2);
  lcd.print(F("F:"));
  lcd.print(humi);
  lcd.print(F(" / T:"));
  lcd.print(temp);
  lcd.print(F(" C"));
 
  lcd.setCursor(0,3);
  printIP();
}

void printIP(){
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
      lcd.print(ether.myip[thisByte], DEC);
      if(thisByte<3){
        lcd.print(".");
      } 
  }
}

void fhumi(){
  double h = dht.readHumidity();
  if(!isnan(h)){
    dtostrf(h, 2, 2, humi);
  }
}

void ftemp(){
  float t = dht.readTemperature();
  if(!isnan(t)){
    dtostrf(t, 2, 2, temp);
  }
}

void fdate(){
  sprintf(dcf77date, "%02d.%02d.%02d",day(),month(),year());
}

void ftime(){
  sprintf(dcf77time, "%02d:%02d:%02d",hour(),minute(),second());
}

void web() {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if(strstr((char *)Ethernet::buffer + pos, "/?LED=ON") != 0) {
    lcd.backlight(); // turn on backlight.
  }

  if(strstr((char *)Ethernet::buffer + pos, "/?LED=OFF") != 0) {
    lcd.noBacklight(); // turn off backlight
  }

  if (pos)  // check if valid tcp data is received
    ether.httpServerReply(homePage()); // send web page data
}
