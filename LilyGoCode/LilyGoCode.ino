// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN ""
float lat, lon,exlat,exlon;
float lat1 = 260, lon1 = 260;
// Your GPRS credentials, if any
const char apn[] = "YOUR-APN";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
#include <Ticker.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else

TinyGsm modem(SerialAT);

#endif

#define uS_TO_S_FACTOR      1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP       60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD           9600
#define PIN_DTR             25
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4

#define SD_MISO             2
#define SD_MOSI             15
#define SD_SCLK             14
#define SD_CS               13
#define LED_PIN             12


void enableGPS(void) {
    // Set SIM7000G GPIO4 LOW ,turn on GPS power
    // CMD:AT+SGPIO=0,4,1,1
    // Only in version 20200415 is there a function to control GPS power
    modem.sendAT("+SGPIO=0,4,1,1");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" SGPIO=0,4,1,1 false ");
    }
    modem.enableGPS();


}

void disableGPS(void) {
    // Set SIM7000G GPIO4 LOW ,turn off GPS power
    // CMD:AT+SGPIO=0,4,1,0
    // Only in version 20200415 is there a function to control GPS power
    modem.sendAT("+SGPIO=0,4,1,0");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" SGPIO=0,4,1,0 false ");
    }
    modem.disableGPS();
}

void modemPowerOn() {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1000);    //Datasheet Ton mintues = 1S
    digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff() {
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1500);    //Datasheet Ton mintues = 1.2S
    digitalWrite(PWR_PIN, HIGH);
}


void modemRestart() {
    modemPowerOff();
    delay(1000);
    modemPowerOn();
}

void setup() {
    // Set console baud rate
    SerialMon.begin(115200);

    delay(10);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    modemPowerOn();

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    Serial.println("/**********************************************************/");
    Serial.println("To initialize the network test, please make sure your GPS");
    Serial.println("antenna has been connected to the GPS port on the board.");
    Serial.println("/**********************************************************/\n\n");

    delay(1000);


    Serial.println("/**********************************************************/");
    Serial.println("After the network test is complete, please enter the  ");
    Serial.println("AT command in the serial terminal.");
    Serial.println("/**********************************************************/\n\n");


}

void loop() {


    getlocation();
    // delay(1000);
    if(lat!=exlat||lon!=exlon){
          sendlocation();
          exlat=lat;
          exlon=lon;
    }
    else 
    {
      Serial.println("same coordinates!");
     delay(1000);
    }

    /*while (1) {
        while (SerialAT.available()) {
            SerialMon.write(SerialAT.read());
        }
        while (SerialMon.available()) {
            SerialAT.write(SerialMon.read());
        }
    }*/
}

void getlocation() {

    if (!modem.testAT()) {
        Serial.println("Failed to restart modem, attempting to continue without restarting");
        modemRestart();
        return;
    }

    Serial.println("Start positioning . Make sure to locate outdoors.");
    Serial.println("The blue indicator light flashes to indicate positioning.");

    enableGPS();
    while (1) {
        if (modem.getGPS(&lat, &lon)) {
            Serial.println("The location has been locked, the latitude and longitude are:");
            Serial.print("latitude:");
            Serial.println(String(lat, 8));
            Serial.print("longitude:");
            Serial.println(String(lon, 8));

            break;
        }
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(500);
    }

    //disableGPS();
}

void sendlocation() {



    // SerialAT.println("ATE1");
    //SerialAT.println("AT+CPIN?");    delay(500);  

    SerialAT.println("AT+CFUN=1");
    delay(500);
    // SerialAT.println("AT+CSQ");     delay(500); 
    // SerialAT.println("AT+COPS?"); delay(500); 
    // SerialAT.println("AT+CPSI?"); delay(500); 
    SerialAT.println("AT+CGATT=1");
    delay(1000);
    SerialAT.println("AT+CIPSHUT");
    delay(1000);
    SerialAT.println("AT+CIPMUX=0");
    delay(1000);
    SerialAT.println("AT+CSTT=\"internet\"");
    delay(2500);
    SerialAT.println("AT+CIICR");
    delay(2500);
    SerialAT.println("AT+CIFSR");
    delay(1000);

    SerialAT.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");
    delay(3000);
    SerialAT.println("AT+CIPSEND");
    delay(500);
    SerialAT.print("GET https://api.thingspeak.com/update?api_key=DWA2S9BE15RCA43E&field1=");
    SerialAT.print(String(lat, 8));
    SerialAT.print("&field2=");
    SerialAT.println(String(lon, 8));
    SerialAT.println((char) 26);
    delay(500);
    SerialAT.println("AT+CIPCLOSE");
    Serial.println("sent!");
}
