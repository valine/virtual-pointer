#include <SPI.h>
#include <WiFiNINA.h>

#define WIFI_SSID        "HelloThere"
#define WIFI_PASSWORD    "Wrathofkhan"

WiFiServer server(80);

void reconnectWiFi() {
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to WiFi...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(5000); // Wait for 5 seconds before retrying
    }
    Serial.println("Reconnected to WiFi!");
}

void setup() {
    Serial.begin(9600);

    if (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) == WL_CONNECT_FAILED) {
        Serial.println("Failed to connect to WiFi");
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    server.begin();
}

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
            client.println("<body><h1>Hello World!</h1></body>");
            client.println("</html>");
        }
        delay(1);
        client.stop();
    }

    if (WiFi.status() != WL_CONNECTED) {
        reconnectWiFi();
    }
}