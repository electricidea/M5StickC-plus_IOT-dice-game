#include "web_server.h"


#include <Arduino.h>
#include "M5StickCPlus.h"

// website stuff
#include "index.h"
#include "electric_logo.h"
#include "favicon.h"
#include "refresh.h"


char dice1_html_value;
char dice2_html_value;


// WIFI and https client librarys:
#include "WiFi.h"
#include <WiFiClientSecure.h>

// WiFi network configuration:
char wifi_ssid[] = "M5StickC-plus";
char wifi_key[] = "1234567890";

IPAddress ip(192, 168, 0, 1);  
IPAddress gateway(192, 168, 0, 1); 
IPAddress subnet(255, 255, 255, 0);


WiFiClient myclient;
WiFiServer server(80);

// GET request indication
#define GET_unknown 0
#define GET_index_page  1
#define GET_favicon  2
#define GET_logo  3
#define GET_refresh_img  4
#define GET_dicevalue 5
int html_get_request;


/***************************************************************************************
* Function name:          web_server_init
* Description:            Initialization of teh AP and web server
***************************************************************************************/
void web_server_init(){
    WiFi.mode(WIFI_AP);
    WiFi.softAP(wifi_ssid, wifi_key);
    WiFi.softAPConfig(ip, gateway, subnet);
    WiFi.begin();
    delay(1000);
    // print the IP-Adress
    char String_buffer[128]; 
    snprintf(String_buffer, sizeof(String_buffer), "IP: %s\n",WiFi.localIP().toString().c_str());
    Serial.print("SSID: ");
    Serial.println(wifi_ssid);
    Serial.print("Password: ");
    Serial.println(wifi_key);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // Start TCP/IP-Server
    server.begin();    
    dice1_html_value = 'X';   
    dice2_html_value = 'X';
}

/***************************************************************************************
* Function name:          check_webserver
* Description:            check for new clients and handle response generation
***************************************************************************************/
void web_server_update(){
    // check for incoming clients
    WiFiClient client = server.available(); 
    if (client) {  
        // force a disconnect after 2 seconds
        unsigned long timeout_millis = millis()+2000;
        Serial.println("New Client.");  
        // a String to hold incoming data from the client line by line        
        String currentLine = "";                
        // loop while the client's connected
        while (client.connected()) { 
        // if the client is still connected after 2 seconds,
        // something is wrong. So kill the connection
        if(millis() > timeout_millis){
            Serial.println("Force Client stop!");  
            client.stop();
        } 
        // if there's bytes to read from the client,
        if (client.available()) {             
            char c = client.read();            
            Serial.write(c);    
            // if the byte is a newline character             
            if (c == '\n') {    
            // two newline characters in a row (empty line) are indicating
            // the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
                // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                // and a content-type so the client knows what's coming, then a blank line,
                // followed by the content:
                switch (html_get_request)
                {
                case GET_index_page: {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println();
                    client.write_P(index_html, sizeof(index_html));
                    break;
                }
                case GET_favicon: {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:image/x-icon");
                    client.println();
                    client.write_P(electric_favicon, sizeof(electric_favicon));
                    break;
                }
                case GET_logo: {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:image/jpeg");
                    client.println();
                    client.write_P(electric_logo, sizeof(electric_logo));
                    break;
                }
                case GET_dicevalue: {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:application/javascript");
                    client.println();
                    client.printf("var dice1value = \"%c\";\n", dice1_html_value);
                    client.printf("var dice2value = \"%c\";\n", dice2_html_value);
                    break;
                }
                case GET_refresh_img: {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:image/png");
                    client.println();
                    client.write_P(refresh_img, sizeof(refresh_img));
                    break;
                }
                default:
                    client.println("HTTP/1.1 404 Not Found");
                    client.println("Content-type:text/html");
                    client.println();
                    client.print("404 Page not found.<br>");
                    break;
                }
                // The HTTP response ends with another blank line:
                client.println();
                // break out of the while loop:
                break;
            } else {    // if a newline is found
                // Analyze the currentLine:
                // detect the specific GET requests:
                if(currentLine.startsWith("GET /")){
                html_get_request = GET_unknown;
                // if no specific target is requested
                if(currentLine.startsWith("GET / ")){
                    html_get_request = GET_index_page;
                }
                // if the logo image is requested
                if(currentLine.startsWith("GET /electric-idea_50x50.jpg")){
                    html_get_request = GET_logo;
                }
                // if the favicon icon is requested
                if(currentLine.startsWith("GET /favicon.ico")){
                    html_get_request = GET_favicon;
                }
                // if the screenshot image is requested
                if(currentLine.startsWith("GET /dicevalue.js")){
                    html_get_request = GET_dicevalue;
                }
                // if the refresh image is requested
                if(currentLine.startsWith("GET /refresh-40x30.png")){
                    html_get_request = GET_refresh_img;
                }
                }
                currentLine = "";
            }
            } else if (c != '\r') {  
            // add anything else than a carriage return
            // character to the currentLine 
            currentLine += c;      
            }
        }
        }
        // close the connection:
        client.stop();
        Serial.println("Client Disconnected.");
    }
}