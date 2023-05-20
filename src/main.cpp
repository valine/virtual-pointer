#include <SPI.h>
#include <WiFiNINA.h>
#include <ctime>
#include "Keyboard.h"
#include "Mouse.h"

#define WIFI_SSID        "HelloThere"
#define WIFI_PASSWORD    "Wrathofkhan"
const bool USE_12_HOUR_TIME = true;

unsigned int localPort = 2390;
IPAddress timeServer(132, 163, 97, 3); // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
WiFiUDP Udp;

WiFiServer server(80);
const int buttonPin = 5; // the button is connected to digital pin 2

unsigned long epochTime = 0;
unsigned long millisAtEpoch = 0;

const int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
void sendNTPpacket(IPAddress& address) {
    // Initialize all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);

    // Initialize values needed to form NTP request
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // Send a packet requesting a timestamp
    Udp.beginPacket(address, 123);
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    int result = Udp.endPacket();

    if (result) {
        Serial.println("NTP request sent successfully");
    } else {
        Serial.println("Error sending NTP request");
    }
}

unsigned long getTime() {
    sendNTPpacket(timeServer);
    unsigned long startedWaitingAt = millis();

    while (!Udp.parsePacket()) {
        if (millis() - startedWaitingAt > 5000) {
            Serial.println("No NTP Response");
            return 0;
        }
        delay(10);
    }

    Udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;

    Serial.println("Received NTP response");
    for (int i = 0; i < NTP_PACKET_SIZE; i++) {
        Serial.print(packetBuffer[i], HEX);
        Serial.print(' ');
    }
    Serial.println();

    return epoch;
}


void reconnectWiFi() {
    while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        epochTime = getTime();
        millisAtEpoch = millis();
        delay(5000); // Wait for 5 seconds before retrying
    }
}

void setup() {
    pinMode(buttonPin, INPUT_PULLUP);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD) ;
    Udp.begin(localPort);
    Serial.begin(9600);
    server.begin();
    Keyboard.begin();
    Mouse.begin();

    epochTime = getTime();
    millisAtEpoch = millis();
}


String getLocalTime() {
    unsigned long currentEpochTime = epochTime + (millis() - millisAtEpoch) / 1000;

    // Convert the currentEpochTime to time_t type before passing it to localtime()
    time_t rawTime = (time_t)currentEpochTime;
    struct tm * ti;
    ti = localtime (&rawTime);

    int year = ti->tm_year + 1900;
    int month = ti->tm_mon + 1;
    int day = ti->tm_mday;
    int hour = ti->tm_hour;
    int min = ti->tm_min;
    int sec = ti->tm_sec;

    // Check for DST
    bool isDst = false;
    if (month > 3 && month < 11) {
        isDst = true;
    } else if (month == 3) {
        int dstStartDay = (14 - (1 + year * 5 / 4) % 7);
        if (day >= dstStartDay) {
            isDst = true;
        }
    } else if (month == 11) {
        int dstEndDay = (7 - (1 + year * 5 / 4) % 7);
        if (day < dstEndDay) {
            isDst = true;
        }
    }

    // Apply Central Standard Time offset
    hour = hour - 6;
    // Apply DST offset
    if (isDst) {
        hour = hour + 1;
    }

    // Correct for overflow
    if (hour < 0) {
        hour += 24;
        day--;
        if (day == 0) {
            month--;
            if (month == 0) {
                year--;
                month = 12;
            }
            day = daysInMonth[month - 1];
        }
    } else if (hour >= 24) {
        hour -= 24;
        day++;
        if (day > daysInMonth[month - 1]) {
            day = 1;
            month++;
            if (month > 12) {
                month = 1;
                year++;
            }
        }
    }
    String am_pm = "";
    if (USE_12_HOUR_TIME) {
        if (hour == 0) {
            hour = 12;
            am_pm = " AM";
        } else if (hour == 12) {
            am_pm = " PM";
        } else if (hour > 12) {
            hour -= 12;
            am_pm = " PM";
        } else {
            am_pm = " AM";
        }
    }
    String timeString = String(year) + "-" + String(month) + "-" + String(day) + " " + String(hour) + ":" + String(min) + ":" + String(sec);
    return timeString;
}

void startPulse() {
    Keyboard.press(KEY_LEFT_GUI); // "Command" or "Windows" key
    Keyboard.press(KEY_LEFT_ALT); // "Alt" key
    Keyboard.press(KEY_LEFT_SHIFT); // "Shift" key
    Keyboard.press('}'); // "}" key
    delay(100);
    Keyboard.releaseAll(); // release all keys at once
    delay(200); // delay to avoid sending the key combination multiple times
}

void connectPulse() {
    Keyboard.press(KEY_LEFT_GUI); // "Command" or "Windows" key
    Keyboard.press(KEY_LEFT_ALT); // "Alt" key
    Keyboard.press(KEY_LEFT_SHIFT); // "Shift" key
    Keyboard.press('|'); // "}" key
    delay(100);
    Keyboard.releaseAll(); // release all keys at once
    delay(200); // delay to avoid sending the key combination multiple times
}

void writeString(const String& output) {
    // Loop over characters in the string.
    for (int i = 0; i < output.length(); ++i) {
        // Type the next character.
        Keyboard.write(output[i]);
        // Short delay to allow the keyboard to catch up.
        delay(10);
    }
}

void enterLogin() {
    writeString("valinl1");
    Keyboard.write(KEY_TAB);
    writeString("Medpass31");
    Keyboard.write(KEY_RETURN);
    delay(3500);
}

void loginToComputer() {
    Mouse.click(); // Send left mouse click
    delay(3500); // delay to avoid multiple clicks
    writeString("Medpass31");
    Keyboard.write(KEY_RETURN);
}



void morningEvent() {
    loginToComputer();
    delay(1000);
    startPulse();
    delay(1000);
    connectPulse();
    delay(3000);
    enterLogin();
}

void eveningEvent() {
    // Lock the screen
    Keyboard.press(KEY_LEFT_GUI); // "Command" or "Windows" key
    Keyboard.press(KEY_LEFT_CTRL); // "Alt" key
    Keyboard.press('q'); // "}" key
    delay(100);
    Keyboard.releaseAll(); // release all keys at once
    delay(200); // delay to avoid sending the key combination multiple times
}

enum EventIndex {
    MORNING_EVENT,
    EVENING_EVENT,
    // Add more events here
    NUM_EVENTS // This should always be the last entry
};

void (*eventFunctions[NUM_EVENTS])() = {
        morningEvent,
        eveningEvent
        // Add more event functions here
};

int eventTimes[NUM_EVENTS] = {
        8,  // Morning event at 8AM
        12 + 6  // Evening event at 6PM
        // Add more event times here
};

// Additional global definition
int eventMinutes[NUM_EVENTS] = {0, 0}; // set the event minutes


bool eventRunToday[NUM_EVENTS] = {false};

void onButtonPress() {
    startPulse();
    delay(1000);
    connectPulse();
    delay(3000);
    enterLogin();
}

void handleEvents() {

    if (epochTime == 0) {
        return;
    }

    // Get the current time
    String currentTime = getLocalTime();

    // Parse the hour and minute from the current time
    int currentHour = currentTime.substring(currentTime.indexOf(' ') + 1, currentTime.indexOf(':')).toInt();
    int currentMinute = currentTime.substring(currentTime.indexOf(':') + 1, currentTime.lastIndexOf(':')).toInt();

    for (int i = 0; i < NUM_EVENTS; ++i) {
        // If it's currently the time for this event and it hasn't been run yet today, run the event
        if (currentHour == eventTimes[i] && currentMinute == eventMinutes[i] && !eventRunToday[i]) {
            eventFunctions[i]();
            eventRunToday[i] = true;
        }
            // If it's past the time for this event, reset the flag so it will run again tomorrow
        else if ((currentHour > eventTimes[i]) || (currentHour == eventTimes[i] && currentMinute > eventMinutes[i])) {
            eventRunToday[i] = false;
        }
    }
}

unsigned long lastMinuteMillis = 0;
void wakeUp() {
    Keyboard.press(KEY_CAPS_LOCK); // you can replace F1 with F2, F3, etc.
    delay(10);
    Keyboard.release(KEY_CAPS_LOCK); // make sure to release the same key
    delay(10);
}

bool isPressed = false;
void loop() {
    WiFiClient client = server.available();

    if (client) {
        String request = client.readStringUntil('\r');
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();

        if (request.indexOf("GET /") != -1) {
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            client.println("<body><h1>" + getLocalTime() + "</h1></body>");
            client.println("</html>");
        }
        delay(1);
        client.stop();
    }

    if (WiFi.status() != WL_CONNECTED) {
        reconnectWiFi();
    }

    if (digitalRead(buttonPin) == LOW) { // if the button is pressed
        isPressed = true;
        delay(200);
    }

    if (isPressed && digitalRead(buttonPin) == HIGH) {
        onButtonPress();
        isPressed = false;
        delay(200);
    }

    handleEvents();


    // Check if a minute has passed since the last time we called wakeUp()
    if (millis() - lastMinuteMillis >= 4 * 1000) { // 60 * 1000 milliseconds in a minute
        wakeUp();
        lastMinuteMillis = millis();
    }
}