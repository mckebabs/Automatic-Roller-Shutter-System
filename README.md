# Automatic Window Roller Shutter System

The following code is used to run Automatic Roller Shutter Controller. 
From a simple purpose device it evolved into a multi functional IoT device to utilize the idle cpu time and unused ports. 
Hardware: ESP8266 - NodeMCU 1.0v, Temeprature sensort DHT11, 9g servo, LED, some capacitors

Features:

1) Autonomous Open/Close functionality for each day of the week. Controlled by timerOpen(); and timerClose();
Values should be defined in the array openClose.

2) Online access. Youŗ Wifi SSID and Password must be entered in the "wifi" section

3) Time is synced from the nist.time.gov every 6 hours

4) Sensor data is stored in Thingspeak.com few times a minute. User has to define his own Read and Write keys

5) Device is controllable from a mobile devices (iOS/Android) via Blynk. User has to define it own Blynk key

6) Light sensor for the failsafe (looks at the led on the remote)

7) Tricky part with the touch buttons on the remote was to activate them. With a simple piece of aluminum foil if human flash is not touching it, it won't work. A solution was to connect the aluminum foil "finger" to a small current which now works very well. 

8) Humidity/Temperature sensor (DHT11)

9) Programmable schedule created for one open time and on close time. Precision is set to minutes

10) 2 LEDs for activity/status indications. Posting data to Thingspeak or sending data to Blynk with one LED and second onboard led displays if window is opened or closed 

11) Fake "multi threading" orchestrating all processes
