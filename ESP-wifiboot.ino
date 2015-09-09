#include "ESP8266WiFi.h"
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>

MDNSResponder mdns;
WiFiServer server(80);

const char* ssid = "esp8266-hue";
const char* ident  = "esp8266user";
String command;
String hueip = "";  
  
// PIR sensor
int PIRsensor =   0;
int LightStatus = 0;
int PIRStatus   = 0;

String st;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i) 
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  

// ip max 16 znakow
  for (int i = 96; i < 112; ++i) 
    {
      hueip += char(EEPROM.read(i));
    }
  Serial.print("HUE IP: ");
  Serial.println(hueip);    
  
  
//  if ( esid.length() > 1 ) {
      // test esid 
      WiFi.begin(esid.c_str(), epass.c_str());
      if ( testWifi() == 20 ) { 
          //launchWeb(0);
          return;
      }
//  }
  setupAP(); 
  
  
  // regular setup:
  pinMode(PIRStatus,INPUT);
}


int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return(20); } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  return(10);
} 

void launchWeb(int webtype) {
          Serial.println("");
          Serial.println("WiFi connected");
          Serial.println(WiFi.localIP());
          Serial.println(WiFi.softAPIP());
          if (!mdns.begin("esp8266", WiFi.localIP())) {
            Serial.println("Error setting up MDNS responder!");
            while(1) { 
              delay(1000);
            }
          }
          Serial.println("mDNS responder started");
          // Start the server
          server.begin();
          Serial.println("Server started");   
          int b = 20;
          int c = 0;
          while(b == 20) { 
             b = mdns1(webtype);
           }
}

void setupAP(void) {
  
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ul>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st +=i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ul>";
  delay(100);
  WiFi.softAP(ssid);
  Serial.println("softap");
  Serial.println("");
  launchWeb(1);
  Serial.println("over");
}

int mdns1(int webtype)
{
  // Check for any mDNS queries and send responses
  mdns.update();
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return(20);
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
   }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush(); 
  String s;
  if ( webtype == 1 ) {
      if (req == "/")
      {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        s += ipStr;
        s += "<p>";
        s += st;
        s += "<form method='get' action='a'><label>SSID: </label><input name='ssid' length=32><br><label>Pass:</label><input name='pass' length=64><br><label>hue ip:</label><input name='hueip' length=16><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/a?ssid=") ) {
        
        //req = URLEncode(string2char(req));
        Serial.println("clearing eeprom");
        for (int i = 0; i < 112; ++i) { EEPROM.write(i, 0); }

        // wycina od 8 znaku do pierwszego wystapienia "&"
        Serial.print("query: "); Serial.println(req);
        Serial.println("GET SSID:");
        String qsid = req.substring(8,req.indexOf('&'));
        qsid = escapeParameter(qsid);
        Serial.println(qsid);
        
        // skreacamy req - usuwamy pierwszy czlon
        req = req.substring(req.indexOf('&')+1, req.length());
        Serial.print("query: "); Serial.println(req);
        
        // wycina od 5 znaku do pierwszego wystapienia "&"
        Serial.println("GET PASS:");
        String qpass = req.substring(5,req.indexOf('&'));
        qpass = escapeParameter(qpass);
        Serial.println(qpass);

        
        // skreacamy req - usuwamy pierwszy czlon
        req = req.substring(req.indexOf('&')+1, req.length());   
        Serial.print("query: "); Serial.println(req);     
        
        // wycina od 5 znaku do pierwszego wystapienia "&"
        Serial.println("GET HUE IP:");
        String qhueip = req.substring(req.lastIndexOf('=')+1);
        Serial.println(qhueip);        
        
        
        Serial.println("");
                
        
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        Serial.println("writing eeprom hue ip:"); 
        for (int i = 0; i < qhueip.length(); ++i)
          {
            EEPROM.write(96+i, qhueip[i]);
            Serial.print("Wrote: ");
            Serial.println(qhueip[i]); 
          }            
          
        EEPROM.commit();
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 ";
        s += "Found ";
        s += req;
        s += "<p> saved to eeprom... reset to boot into new wifi</html>\r\n\r\n";
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }
  } 
  else
  {
      if (req == "/")
      {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>ESP8266 HUE PIR sensor.";
        s += "<p></p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/cleareeprom") ) {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>Clearing the EEPROM<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");  
        Serial.println("clearing eeprom");
        for (int i = 0; i < 112; ++i) { EEPROM.write(i, 0); }
        EEPROM.commit();
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }       
  }
  client.print(s);
  Serial.println("Done with client");
  return(20);
}

String escapeParameter(String param) {
  param.replace("+"," ");
  param.replace("%21","!");
  param.replace("%23","#");
  param.replace("%24","$");
  param.replace("%26","&");
  param.replace("%27","'");
  param.replace("%28","(");
  param.replace("%29",")");
  param.replace("%2A","*");
  param.replace("%2B","+");
  param.replace("%2C",",");
  param.replace("%2F","/");
  param.replace("%3A",":");
  param.replace("%3B",";");
  param.replace("%3D","=");
  param.replace("%3F","?");
  param.replace("%40","@");
  param.replace("%5B","[");
  param.replace("%5D","]");
  
  return param;
}





void loop() {
  // put your main code here, to run repeatedly:
  
  PIRStatus = digitalRead(PIRsensor);
  
  Serial.print("PIR: ");
  Serial.print(PIRStatus);
  Serial.print("   light: ");
  Serial.println(LightStatus);
  
  if(PIRStatus == 1 && LightStatus == 0) {
    TurnOn(14);  
    LightStatus = 1;
  }  
  else if(PIRStatus == 0 && LightStatus == 1) {
    TurnOff(14);
    LightStatus = 0;
  }
  
}


void TurnOn(int bulb) {
  WiFiClient client;
  if (!client.connect(hueip.c_str(), 80)) {
    Serial.println("connection failed");
    return;
  }
  // \"hue\": 50100,\"sat\":255,
  command = "{\"on\": true,\"bri\":128,\"transitiontime\":2}";
  // This will send the request to the server
  client.print("PUT /api/");
  client.print(ident);
  client.print("/lights/");
  client.print(bulb);  // hueLight zero based, add 1
  client.println("/state HTTP/1.1");
  client.println("keep-alive");
  client.print("Host: ");
  client.println(hueip.c_str());
  client.print("Content-Length: ");
  client.println(command.length());
  client.println("Content-Type: text/plain;charset=UTF-8");
  client.println();  // blank line before body
  client.println(command);  // Hue command
  client.stop();
  Serial.println("closing connection");
}

void TurnOff(int bulb) {
  WiFiClient client;
  if (!client.connect(hueip.c_str(), 80)) {
    Serial.println("connection failed");
    return;
  }
  command = "{\"on\": false}";
  // This will send the request to the server
  client.print("PUT /api/");
  client.print(ident);
  client.print("/lights/");
  client.print(bulb);  // hueLight zero based, add 1
  client.println("/state HTTP/1.1");
  client.println("keep-alive");
  client.print("Host: ");
  client.println(hueip.c_str());
  client.print("Content-Length: ");
  client.println(command.length());
  client.println("Content-Type: text/plain;charset=UTF-8");
  client.println();  // blank line before body
  client.println(command);  // Hue command
  client.stop();
  Serial.println("closing connection");
}
