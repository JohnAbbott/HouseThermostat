#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Blynk App Code for office
//char auth[] = "kWnqJmKRP2jIg_J7VUN9HK4QezKOR26H";
// Blynk App Code for House
//char auth[] = "0cpCeyQIGS5XLezNAPt5cB0u9_SLDuCD";
// Blynk App Code for Third Floor
//char auth[] = "H04IbxPiBKSng_mOtwy0_yXj-6QpZEB8";
// Blynk Code for Kitchen
//char auth[] = "pliRfH0R00oj10pJB3fOpMVN5XRGwyQZ";
// SECOND FLOOR
//char auth[] = "-ZJvFflBA3cQzOm3b2JxoefuCZQ0yNKx";
//char auth[] =  "mmmU_svziKg5vYm2g71rPqLblVlIx9Om";
char auth[] = "mmmU_svziKg5vYm2g71rPqLblVlIx9Om";

#define DHTPIN 2 
#define DHTTYPE DHT22   // DHT 22
#define ON 1
#define OFF 0

DHT dht(DHTPIN, DHTTYPE);
 
// OLED display
#define OLED_ADDR   0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Your WiFi credentials.
// Set password to "" for open networks.
/* Ruth's network
char *ssid = "Flints_Realm";
char *pass = "Pupusas4me";
*/

/* Abbott home network */
// char *ssid = "Thats_What_She_Ssid";
// char *pass = "D0gLessAbb0tts";

/* The Campter */
char *ssid = "Pussy Galore";
char *pass = "stayHigh1024!!";

const int relayPin = D5;
unsigned long lastTransmit;
unsigned long lastUpdate;
unsigned long lastTempCheck;
int enabledState;
int requiredTemp;
//float temp;
int f;
int h;
int heatON;
  
void setup() {
  // reserve three bytes to store the enabledState.  The first time running uncomment the write-commit lines to set an initial value.
  EEPROM.begin(3);

  // When running this for the first time on a new SBC you must uncomment these, upload it.  Then comment them back out and re-upload.
  //EEPROM.write(2,0);
  //EEPROM.commit();
  
  pinMode(relayPin, OUTPUT);

  Serial.begin(9600);
  
  dht.begin();
  
      // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();
  Serial.println("going into blynk begin");
  Blynk.begin(auth, ssid, pass);
  Serial.println("returned from Blynk logged in");
  
  lastTransmit = millis();
  lastUpdate = millis();
  lastTempCheck = millis();
  enabledState = EEPROM.read(1);
  Serial.println("EEPROM: System is  " + String(enabledState ? "ENABLED" : "DISABLED"));
  requiredTemp = EEPROM.read(2);
  Serial.println("EEPROM: Target Temperature is " + String(requiredTemp) + "°F");
  
}

BLYNK_CONNECTED() {
  Serial.println("CONNECTED");  Blynk.run();
  Blynk.virtualWrite(V2, requiredTemp);  Blynk.run();
  Blynk.virtualWrite(V3, requiredTemp);  Blynk.run();
  Blynk.virtualWrite(V0, enabledState);
  Blynk.setProperty(V0, "color", (enabledState ? "#00FF00" : "#FF0000"));  Blynk.run();
  Blynk.virtualWrite(V1, "Ready");  Blynk.run();
  Blynk.setProperty(V1, "color", "#00FF00");  Blynk.run();
  getTemps();
}

BLYNK_WRITE(V0)
{
  int readEnabledState;
  enabledState = !enabledState;
  Serial.println("If the enabled state is bouncing and this is running twice it is because you have not set V0 to be a switch in the app");
  Serial.println("inside blynk write vo");
  Serial.print("coming in enabled state is: ");
  Serial.println(enabledState);
  readEnabledState = EEPROM.read(1);
  Serial.print("reading enabled state is: ");
  Serial.println(readEnabledState);
  
  EEPROM.write(1, enabledState);
  EEPROM.commit();

  Serial.println("System is " + String(enabledState ? "ENABLED" : "DISABLED"));
  Serial.print("Just before sending to the app enabled state is: ");
  Serial.println(enabledState);
  Blynk.virtualWrite(V0, enabledState);
  Blynk.setProperty(V0, "color", enabledState ? "#00FF00" : "#FF0000");  Blynk.run();
  Serial.println("exiting bynk write subroutine");
}

BLYNK_WRITE(V2)
{
  requiredTemp = param.asInt();  Blynk.run();
  Blynk.virtualWrite(V3, requiredTemp);  Blynk.run();
  EEPROM.write(2, requiredTemp);
  EEPROM.commit();
  Serial.println("Target Temperature is " + String(requiredTemp));
}

void heatingControl(boolean onOff)
{
  Serial.println("Inside heat control");
  
  // Code to turn relay on and off.
  if (onOff == ON){
     heatON=1;  // Set this variable to be read later in the OLED display
     Serial.println("Turn on Relay");
     digitalWrite(relayPin, HIGH);  // turn on
  } else if(onOff == OFF){
     heatON=0;
     Serial.println("Relay set to Off");
     digitalWrite(relayPin, LOW);  // turn off
  } else {
    Serial.println("Error Code: 1");
  }
  
  Blynk.virtualWrite(V1, String(onOff ? "Heat ON" : "Heat OFF"));  Blynk.run();
  Blynk.setProperty(V1, "color", String(onOff ? "#00FF00" : "#FF0000"));  Blynk.run();
  lastTransmit = millis();
}

void checkTemp()
{
  Serial.println("Inside checkTemp");
  // changed January 11th to heat one degree above what is set.  This should prevent short cycling the boiler
  if ((f > (requiredTemp+1)) && (enabledState == ON)) heatingControl(OFF);  
  if ((f < requiredTemp) && (enabledState == ON))  heatingControl(ON);
  lastTempCheck = millis();
}

void getTemps()
{
  /*int h = dht.readHumidity();
  int f = dht.readTemperature(true);
  
  float humd = myHumidity.readHumidity(); Blynk.run();
  temp = myHumidity.readTemperature(); Blynk.run();
  */
  Blynk.run();
  h = dht.readHumidity();
  f = dht.readTemperature(true);
  Serial.print(f);
  Serial.print("  ");
  Serial.println(h);
  Blynk.virtualWrite(V4, f);
  Blynk.virtualWrite(V5, h);

  // Display to OLED
     display.clearDisplay();
     display.setTextSize(1);
     display.setTextColor(WHITE,BLACK);

     display.setCursor(3,3);
     //display.print("Pearl Street Temp");
     display.print("Two Creeks Temp");

     display.drawLine(70, 16, 70, 53, WHITE);
     display.drawRect(80, 18, 35, 16, WHITE);
     if (heatON==1){
       display.setCursor(93,22);
       display.print("ON");
     } else {
       display.setCursor(89,22);
       display.print("OFF");
     }
     
     display.setTextSize(2);
     display.setCursor(87,40);
     display.print(requiredTemp);

     display.setTextSize(1);
     display.setCursor(2,56);
     display.print("Humidity: ");
     display.setCursor(60,56);
     display.print(h);
     display.print("%");
     
     display.setFont(&FreeSerif24pt7b);
     display.setCursor(15,47);
     display.print(f);
     display.print((char)247);  //Degree symbol
     display.setFont();


     display.display();
}

void loop() {

  //Serial.print(heatingControl);
  //Serial.print("Inside Loop: ");
  
  if ((millis() - lastTransmit) > 60000)
  {
    Serial.print(f);
  Serial.print(",");
  Serial.print(enabledState);
  Serial.print(",");
  Serial.print(millis()-lastTransmit);
  Serial.print(",");
  Serial.println(requiredTemp);
    Serial.println("Inside Laast Transmit if");
    if ((f < requiredTemp) && (enabledState == ON)) heatingControl(ON);
    else heatingControl(OFF);
   }

   // It is possible that the EEPROM gets biffed so if the temp is over 80, maybe you better re-read it.
   if (f > 100){
    Serial.print("temp: ");
    Serial.println(f);
    Serial.println("Inside too damed hot test");
    getTemps();
   }

  if ((millis() - lastUpdate) > 1000)
  {
    Serial.println("Inside lastUpdate if");
    getTemps();
    //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastUpdate = millis();
  }

  if ((millis() -   lastTempCheck) > 60000) checkTemp();
  
  Blynk.run();
  // put your main code here, to run repeatedly:
  /* int h = dht.readHumidity();
  int f = dht.readTemperature(true);
  Blynk.virtualWrite(V4, f);
  Blynk.virtualWrite(V5, h);
  */
  /*
  Serial.print("Temperature = ");
  Serial.println(f);
  Serial.print("Humidity = ");
  Serial.println(h);
  */
  // delay(1000);
}
