#define _Digole_Serial_I2C_
// #define _Digole_Serial_SPI_

// This #include statement was automatically added by the Spark IDE.
#include "DigoleSerialDisp.h"

#define UPDATE_DISPLAY 200
#define UPDATE_MESSAGE 300

#define SYNC_TIME_MILLIS (60 * 60 * 1000) // Once per hour
unsigned long lastSync = millis();

DigoleSerialDisp digole(0x27); //I2C
// DigoleSerialDisp digole(A2); //SPI

uint32_t currentTime;
uint32_t lastTime = 0UL;
uint8_t currentSec;
uint8_t lastSec;
uint32_t displayUpdated = 0UL;
String timeStr;
char timeoutput[20];
char secoutput[2];

char dateStr[8];

bool updateMsg = false;
uint32_t msgUpdated = 0UL;
uint8_t msgStart = 0;
String fullMsg = "";
char message[14];

int u_otemp(String temp);
int u_message(String msg);

bool updateTemp = false;
char otemp[5];

const unsigned char fonts[] = {6, 10, 18, 51, 120, 123};
const char _days_short[8][4] = {"","Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

bool fullRedraw = true;

void setup() {
    Serial.begin(9600);
    Time.zone(-4);
    
    pinMode(A0, OUTPUT); //buzzer
    
    digole.begin(LCD128x64);  //Set display to color OLED 160x128
    digole.disableCursor();
    // digole.displayConfig(0);
    // digole.displayStartScreen(0);
    digole.clearScreen(); //CLear screen
    delay(50);
    
    Spark.variable("time", &timeoutput, STRING);
    Spark.variable("otemp", &otemp, STRING);
    
    Spark.function("u_otemp", u_otemp);
    Spark.function("u_message", u_message);
    
    // TODO put some place holder test in placd for temp and dates
    
    // RGB.control(true); 
    // RGB.color(0, 0, 0);
    
    // tone(D2, 440, 200);
}

void loop() {
    currentTime = Time.now();
    uint32_t n = millis();
    
    if (n - lastSync > SYNC_TIME_MILLIS) {
        Spark.syncTime();
        lastSync = n;
    }
    
    if (n - displayUpdated > UPDATE_DISPLAY || displayUpdated == 0 || currentTime < displayUpdated || fullRedraw) {
        displayUpdated = n;
        if (currentTime != lastTime) {
            currentSec = Time.second();
            
            if (Time.minute() == 0 && currentSec == 0) {
                fullRedraw = true;
                digole.clearScreen();
            }

            lastTime = currentTime;
            sprintf(timeoutput, "%02d.%02d", Time.hourFormat12(), Time.minute());
            
            digole.setFont(fonts[5]);
            digole.setPrintPos(0, 0);
            digole.print(timeoutput);
            
            digole.setFont(0);
         
            // Seconds
            digole.setPrintPos(14, 3);
            sprintf(secoutput, "%02d", currentSec);
            digole.print(secoutput);

            // Date
            if ((Time.minute() % 5 == 0 && currentSec == 0) || fullRedraw) {
                digole.setPrintPos(7, 4);
                sprintf(dateStr, "%3s %02d/%02d", _days_short[Time.weekday()], Time.month(), Time.day());
                digole.print(dateStr);
            }
            
            // Alarm
            if ((Time.minute() == 30 && Time.hour() == 6) && (currentSec >= 0 && currentSec <= 20) && Time.weekday() >= 2 && Time.weekday() <= 6 && Time.year() > 2000) {
                playAlarm();
            } else if ((Time.minute() == 50 && Time.hour() == 19) && (currentSec >= 0 && currentSec <= 20) && Time.weekday() >= 1 && Time.weekday() <= 5 && Time.year() > 2000) {
                playAlarm();
            } else if ((Time.minute() == 0 && Time.hour() == 20) && (currentSec >= 0 && currentSec <= 20) && Time.weekday() >= 1 && Time.weekday() <= 5 && Time.year() > 2000) {
                playAlarm();
            }

            // delay(50);
            lastSec = currentSec;
        }
    }

    if (updateTemp || fullRedraw) {
        if (updateTemp) {
            updateTemp = false;
            Spark.publish("jk-blink/outsidetemp", otemp, 300, PRIVATE);
        }
        
        digole.setFont(0);
        digole.setPrintPos(0, 4);
        // digole.print("       "); // Clear out old temp space
        // digole.setPrintPos(0, 4);
        digole.print(otemp);
    }
    
    if (updateMsg) {
        updateMsg = false;
        Spark.publish("jk-blink/message", message, 300, PRIVATE);
        sprintf(message, "%-13s", "");
        digole.setFont(0);
        digole.setPrintPos(0, 3);
        digole.print(message);
    }
    
    if (fullMsg.length() > 0) {
        if (n - msgUpdated > UPDATE_MESSAGE || msgUpdated == 0 || currentTime < msgUpdated) {
            msgUpdated = n;        
            if (fullMsg.length() <= 13) {
                sprintf(message, "%-13s", fullMsg.c_str());
                // sprintf(message, "%-13s", msg.substring(0, msg.length() > 13 ? 13 : msg.length()).c_str());
            } else {
                // Scroll it
                String tmp = fullMsg + " " + fullMsg; // FIXME this doesn't work
                sprintf(message, "%-13s", tmp.substring(msgStart, 13).c_str());
                if (msgStart++ >= 14) msgStart = 0;
            }
    
            digole.setFont(0);
            digole.setPrintPos(0, 3);
            digole.print(message);
        }
    }
    
    // if (updateMsg || fullRedraw) {
    //     digole.setFont(0);
    //     digole.setPrintPos(0, 3);
    //     digole.print(message);
    // }
    
    if (fullRedraw) {
        fullRedraw = false;
    }
}

int u_otemp(String temp) {
    updateTemp = true;
    // strcpy(otemp, temp.c_str());
    sprintf(otemp, "%-4s", temp.c_str());
    Serial.print("Temp:");
    Serial.println(otemp);

    return 1;
}

int u_message(String msg) {
    updateMsg = true;
    msg.replace("%20"," ");
    fullMsg = msg;

    // sprintf(message, "%-13s", msg.substring(0, msg.length() > 13 ? 13 : msg.length()).c_str());
    // // strcpy(message, msg.c_str());
    // Serial.print("Message:");
    // Serial.println(msg);
    return 1;
}

void playAlarm() {
    tone(A0, 600, 500);
}