

 
//-----Libs-----
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <BlynkSimpleEsp8266.h>
#include <Time.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Servo.h>
#include <DHT.h>


//----Functions

boolean readSensor();

//---Timer last update----
unsigned long updateTimer_time = 0;
unsigned long timeNow_time = 0;
unsigned long timerOpen_time = 0;
unsigned long timerClose_time = 0;
unsigned long testSensor_time = 0;
unsigned long getTempHumidity_time = 0;
unsigned long postToThingspeak_time = 0;

//------Wifi-----
char ssid[] = "xxxxxxxxxxxx";  //  your network SSID (name)
char pass[] = "xxxxxxxxxxxx";       // your network password
WiFiClient client;
long rssi = WiFi.RSSI();

//------Light Sensor----
int SensorNo = 5;
boolean stat;


//----Timer-----
boolean windowsOpen=false;
int openClose[7][4] = {
  {7, 0, 9, 0},
  {7, 0, 9, 0},
  {7, 0, 9, 0},
  {7, 0, 9, 0},
  {7, 0, 9, 0},
  {11, 0, 13, 0},
  {11, 0, 13, 0}, 
};
String dayN[]={"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
unsigned long lastOpenedClosed;


//-----Time------
IPAddress timeServerIP; // time.nist.gov NTP server address
WiFiUDP udp;

int cb; //Test if time was retrieved from the server
void timeNow();
boolean firstTime = 0;
unsigned long lastUpdate=0;
unsigned long sixHours=21600;
String timeNowIs;
String uptime;

//-------Servo---
//Finetuned values for the my relative servo position to the remote
Servo myservo; 
int servoDefault = 150;
int servoOpen = 170;
int servoClose = 130;

//-----ThingSpeak-----
// replace with your channel’s thingspeak API key
String writeApiKey = "xxxxxxxxxxxxxxxx";
String readApiKey = "xxxxxxxxxxxxxxx";
const char* server = "api.thingspeak.com";


//-------Blynk-----
char auth[] = "xxxxxxxxxxxxxxxxxxxxxxxx";

//------Temperature/Humidity---------

#define DHTPIN 13
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

float h;
float t;
float hic;


//---Blinker----
int ledState = LOW;   
const int ledPin =  12; 

//-------END---------



//------FUNCTIONS--------
//Delay thing is not pretty, but it is to short to complicate things
void blinker() {
     digitalWrite(ledPin, HIGH);
     delay(50);
     digitalWrite(ledPin, LOW);
}


// field variable should be something like this
// field =     postStr +="&field1=";    postStr += String(variable);
// where "field1" is the name of the field in thingspeak and variable is the value of the field
// The "field" value can consist of multiple field[x] definitions

void thingspeak(String field){
    if (client.connect(server,80)) { // "184.106.153.149" or api.thingspeak.com
    String postStr = writeApiKey;
    postStr += field;
    postStr += "\r\n\r\n";
    
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+writeApiKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);   
    Serial.println("Sent to Thingspeak");
    }
    client.stop();   
}

void postToThingspeak(){
    String postLine ="&field1=";
    postLine += String(WiFi.RSSI());
    postLine +="&field2=";
    postLine += String(windowsOpen);
    postLine +="&field3=";
    postLine += String(readSensor());
    postLine +="&field4=";
    postLine += String(t);
    postLine +="&field5=";
    postLine += String(h);
    postLine +="&field6=";
    postLine += String(hic);
    postLine +="&field7=";
    postLine += String(millis()/1000/86400);
    thingspeak(postLine);
    blinker();
}


void timeUpdate() {
  
  const char* ntpServerName = "time.nist.gov";
  unsigned long epoch;
  unsigned int localPort = 2390;      // local port to listen for UDP packets
  const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
  byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
  WiFi.hostByName(ntpServerName, timeServerIP); 
// send an NTP packet to a time server
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  
  // wait to see if a reply is available
  delay(1000);
  cb = udp.parsePacket();
  Serial.println(cb);
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.println("packet received");
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    const unsigned long gmt3 =10800;
    // subtract seventy years:
    epoch = secsSince1900 - seventyYears + gmt3;

    if(secsSince1900!=0){
    setTime(epoch);  
    timeNow();
    }else{
    Serial.print("\n----Time=0----\n");     
    }
    secsSince1900=0;
  }
  }

void updateTimer() {
    if(firstTime==0){
    while(!cb){
    timeUpdate();
    }
    firstTime=1;
    Serial.println("Time has been updated for the first time");
    }else if(now()-lastUpdate < sixHours){
    while(!cb){
    timeUpdate();
    }
    lastUpdate=now();
    blinker();
    Serial.println("Time has been updated");
    }
}

boolean readSensor(){
  pinMode(SensorNo, INPUT);
  // read the input pin:
  int sensorPin = digitalRead(SensorNo);
  if (sensorPin==0){
    return true;
  }
    else{
    return false;
  }
  }

void testSensor(){
  if(readSensor()){
    myservo.write(servoDefault); 
    Serial.println("LIGHT DETECTED");
  }else{
    Serial.println("Test Ok");
    }
  }
  
void servo(int x) {
    pinMode(16, OUTPUT); //Touch sensor pin On
    myservo.write(x); 
    delay(200);
    if(digitalRead(SensorNo)){
    pinMode(16, INPUT); //Touch sensor pin Off
    myservo.write(servoDefault);
    }else{
    delay(200);
    pinMode(16, INPUT); //Touch sensor pin Off
    myservo.write(servoDefault);
    }
  }

// Check if window is not opened
// Check which day it is today
// Check if current hour matches the defined "open" value
// If so, Open the window, print the msg, set windowsOpen value to true and turn on the LED 
// Test for windows status removed, since it will make scenario more bulletproof. If the Window is already opened, it won't hurt if the button is pressed once more
void timerOpen() { 
    String xc = dayStr(weekday());  
  //------------------------------------
    for (int i=0; i <= 6; i++){
      if(xc==dayN[i]){
        if(hour() == openClose[i][0]  && minute() == openClose[i][1] && millis()-lastOpenedClosed>60){
           servo(servoOpen);
           servo(servoOpen);
           Serial.println("Opened on ");
           Serial.print(dayN[i]);
           windowsOpen=true;
           digitalWrite(2, LOW); 
           break;
           lastOpenedClosed = millis();
          }
         }
      } 
}


// Check if window is  opened
// Check which day it is today
// Check if current hour matches the defined "open" value
// If so, Close the window, print the msg, set windowsOpen value to true and turn on the LED 

void timerClose() {
    String xc = dayStr(weekday());  
  //------------------------------------
    for (int i=0; i <= 6; i++){
      if(xc==dayN[i]){
        if(hour() == openClose[i][2]  && minute() == openClose[i][3] && millis()-lastOpenedClosed>60){
           servo(servoClose);
           servo(servoClose);
           Serial.println("Closed on ");
           Serial.print(dayN[i]);
           windowsOpen=false;
           digitalWrite(2, HIGH); 
           lastOpenedClosed = millis();
           break;          
         }
      }
    }
}

void timeNow(){
    timeNowIs = String(hour());
    timeNowIs +="-";
    timeNowIs += String(minute());
    timeNowIs +="-";
    timeNowIs += String(second());    
    Serial.println(timeNowIs);
    Serial.println();
    
//---Courtesy of Ardino comunnity user Limbo
    long days=0;
    long hours=0;
    long mins=0;
    long secs=0;
    secs = millis()/1000; //convect milliseconds to seconds
    mins=secs/60; //convert seconds to minutes
    hours=mins/60; //convert minutes to hours
    days=hours/24; //convert hours to days
    secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max 
    mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
    hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
    
    uptime = String(days);
    uptime +="D ";
    uptime +=String(hours);
    uptime +="H ";
    uptime +=String(mins);
    uptime +="M";

    Serial.println(uptime);
    Serial.println();
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}



void getTempHumidity(){

  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature() -1.5;  //-1.5 is approximate difference between DHT22 reading and the actual value. The difference moste likely come from a heat from the ESP8266/DHT22
  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("Heat Index (*C): ");
  Serial.println(hic); 
}

//----Blynk functions-----
//Buttons Open and Close 
  BLYNK_WRITE(V0)
{
    Serial.println("Message: Write V0");
    servo(servoClose);
    Serial.println("Windows closed");
    windowsOpen=false;
    digitalWrite(2, HIGH); 
}

  BLYNK_WRITE(V1)
{
      Serial.println("Message: WriteV1");
      servo(servoOpen);
      Serial.println("Windows opened");
      windowsOpen=true;
      digitalWrite(2, LOW);
}

  //Window Status
  BLYNK_READ(V2)
{
  String text;
  if(windowsOpen){
    text = "Window OPENED";
  } else{
    text = "Window CLOSED";
  }
    Blynk.virtualWrite(V2, text);
    blinker();
  }

//The rest of the values to be updated less frequently

  BLYNK_READ(V3){ 
 Blynk.virtualWrite(V3, t);
 Blynk.virtualWrite(V4, h);
 Blynk.virtualWrite(V5, timeNowIs);
 //Blynk.virtualWrite(V6, hic);
 Blynk.virtualWrite(V7, uptime);
 blinker();
}

//-------SETUP--------

void setup(){
//-------Servo---------
//Servo is reset first because on the boot of NodeMCU some current is run through the pins and usually it opens the window. Default position for the window is closed so in case power is lost and the controller restarts, it should make sure the window is closed by activating the Close position
  myservo.attach(4); 
  myservo.write(servoClose); 
  delay(200); 
  myservo.write(servoDefault);
// Set time for testing  
//  setTime(8,59,50,30,5,2016); 

//Pin 16 is used for the touche sensor. It should always be LOW
  digitalWrite(16, LOW);

  
  Serial.begin(115200);


//---------WIFI--------
  // Connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  printWifiStatus();

//-----Blynk-----
  Blynk.config(auth);

//-------LED-------
  pinMode(2, OUTPUT); 
  pinMode(12, OUTPUT); 

//----DHT Temp-----
dht.begin();  

}
  void loop(){     
  Blynk.run(); //Runs all Blynk activities

  if(millis()- updateTimer_time>60000){
    updateTimer();
    updateTimer_time = millis();
  Serial.println("updateTimer");
  }

  if(millis() - timeNow_time >3000){
    timeNow();
    timeNow_time = millis();   
      Serial.println("timeNow");
  }

  if(millis() - timerOpen_time>30000){
    timerOpen();
    timerOpen_time = millis(); 
      Serial.println("timerOpen");  
  }
  
  if(millis() - timerClose_time>30000){
    timerClose();
    timerClose_time = millis();   
          Serial.println("timerClose");
  }
  
  if(millis() - testSensor_time>5000){
    testSensor();
    testSensor_time = millis(); 
          Serial.println("testSensor");  
  }
  
  if(millis() - getTempHumidity_time>50000){
    getTempHumidity();
    getTempHumidity_time = millis();   
          Serial.println("getTempHumidity");
  }
  
  if(millis() - postToThingspeak_time>60000){
    postToThingspeak();
    postToThingspeak_time = millis();   
      Serial.println("postToThingspeak");
  }
  yield();
}

//Known bugs- won't be fixed:
// In case of launching the controller <60 seconds after opening or closing time, one time manual mode might work incorrectly. After second try lastOpenedClosed variable will be upadted
