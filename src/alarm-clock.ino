#define _Digole_Serial_I2C_
// #define _Digole_Serial_SPI_

// This #include statement was automatically added by the Spark IDE.
#include "DigoleSerialDisp.h"
#include "rest_client.h"
#include "jsmnSpark.h"

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
int u_update_alarm(String nothing);

bool updateTemp = false;
char otemp[5];

uint32_t nextAlarmTime;
uint32_t nextAlarmDay;
uint32_t nextAlarmStatus;
char nextAlarmName[50];
bool updateAlarm = false;
bool haveAlarm = false;

const unsigned char fonts[] = {6, 10, 18, 51, 120, 123};
const char _days_short[8][4] = {"","Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

bool fullRedraw = true;

RestClient client = RestClient("api.weecode.com");

#define TOKEN_STRING(js, t, s) \
	(strncmp(js+(t).start, s, (t).end - (t).start) == 0 \
	&& strlen(s) == (t).end - (t).start)

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

int u_update_alarm(String nothing) {
	updateAlarm = true;
  return 1;
}

String response;
char urlPath[60];
char devId[25];
void getNextAlarm() {
	Spark.deviceID().toCharArray(devId, 25);

	response = "";
	sprintf(urlPath, "/alarm/v1/devices/%s/alarms/next", devId);

	int statusCode = client.get(urlPath, &response);
	if (statusCode == 200) {
		int i, r;
		jsmn_parser p;
		jsmntok_t tok[20];
		jsmn_init(&p);
		char obj[50];

		r = jsmn_parse(&p, response.c_str(), tok, 17);
		if (r == JSMN_SUCCESS) {
			for (i = 1; i < 17; i+=2) {
				strlcpy(obj, &response.c_str()[tok[i].start], (tok[i].end - tok[i].start + 1));

				if (TOKEN_STRING(response.c_str(), tok[i], "name")) {
					strlcpy(nextAlarmName, &response.c_str()[tok[i+1].start], (tok[i+1].end - tok[i+1].start + 1));
				} else if (TOKEN_STRING(response.c_str(), tok[i], "time")) {
					strlcpy(obj, &response.c_str()[tok[i+1].start], (tok[i+1].end - tok[i+1].start + 1));
					nextAlarmTime = atoi(obj);
				} else if (TOKEN_STRING(response.c_str(), tok[i], "dayOfWeek")) {
					strlcpy(obj, &response.c_str()[tok[i+1].start], (tok[i+1].end - tok[i+1].start + 1));
					nextAlarmDay = atoi(obj);
				} else if (TOKEN_STRING(response.c_str(), tok[i], "status")) {
					strlcpy(obj, &response.c_str()[tok[i+1].start], (tok[i+1].end - tok[i+1].start + 1));
					if (TOKEN_STRING(response.c_str(), tok[i+1], "true")) {
						nextAlarmStatus = 1;
					} else {
						nextAlarmStatus = 0;
					}
				}
			}
			haveAlarm = true;
		}
	} else {
		haveAlarm = false;
	}
}
