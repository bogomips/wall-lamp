
// arduino-cli compile --fqbn esp8266:esp8266:d1_mini_lite wall-lights.ino 
// arduino-cli upload -p /dev/cu.usbserial-14340  --fqbn esp8266:esp8266:d1_mini_lite:baud=3000000

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

#define VERBOSE false


struct wifiSetup {
    char* ssid = "yourNetwork";
    char* password = "yourp=Pass";
} wifiConfig;

struct udpSetup {
    WiFiUDP udp;
    unsigned int udpPort = 4210;
    char incomingPacket[255];
    int packetSize;
} udpConfig;

IPAddress staticIP(192, 168, 1, 233); //ESP static ip
IPAddress gateway(192, 168, 1, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(8, 8, 8, 8);  //DNS

void mDNS() {

    if (MDNS.begin("wall-lamp")) {        
        #if VERBOSE     
            Serial.println("mDNS responder started");
        #endif
    } else {
        #if VERBOSE
            Serial.println("Error setting up MDNS responder!");
        #endif
    }

    MDNS.addService("wall-lamp", "udp", (int)udpConfig.udpPort);
}


void connectWiFi() {

    byte ledStatus = LOW;

    WiFi.mode( WIFI_STA );
    WiFi.config(staticIP, gateway, subnet, dns);

    // Connect WiFi
    WiFi.hostname("wall-lamp");
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);

    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #if VERBOSE
        Serial.print(".");
    #endif
    digitalWrite( BUILTIN_LED, ledStatus ); // Write LED high/low.
    ledStatus = ( ledStatus == HIGH ) ? LOW : HIGH;
    }

    #if VERBOSE
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println( WiFi.localIP() );    
        Serial.print("IP address: ");
        Serial.print(WiFi.localIP());
        Serial.println("");
    #endif

    udpConfig.udp.begin(udpConfig.udpPort);  

}

void setup() {

    Serial.begin(115200);

    connectWiFi();
    mDNS();
}

void readIncomingUdp() {
    udpConfig.packetSize = udpConfig.udp.parsePacket();
    if (udpConfig.packetSize) {
        #if VERBOSE
            Serial.printf("Received %d bytes from %s, port %d\n", udpConfig.packetSize, udpConfig.udp.remoteIP().toString().c_str(), udpConfig.udp.remotePort());
        #endif
        int len = udpConfig.udp.read(udpConfig.incomingPacket, 255);
        if (len > 0) {
            udpConfig.incomingPacket[len] = 0;
        }
    }
}


void loop() {
    
    MDNS.update();
    readIncomingUdp();

    if (udpConfig.packetSize) {

        #if VERBOSE 
            Serial.printf("UDP packet contents: %s\n", udpConfig.incomingPacket);
        #endif

        Serial.printf("%s\n", udpConfig.incomingPacket);
    }
}


