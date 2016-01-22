#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <RCSwitch.h>

/*
 Web Server
 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009 by David A. Mellis modified 9 Apr 2012, by Tom Igoe

 modified nov. 2015, bij Sibbele Oosterhaven (support GET-variables to set/unset digital pins)
 */

//Defines
#define maxLength     50  // string length
#define lightSensorPin     1  // on channel A1
#define tempSensorPin      5  // on digital 5 
#define rfPin         7
#define ledPin        8  
#define infoPin       9  

#define CODE_Button1on   2666031
#define CODE_Button1off  2666030
#define CODE_Button2on   2666029 
#define CODE_Button2off  2666028
#define CODE_Button3on   2666027 
#define CODE_Button3off  2666026
#define CODE_Button4on   2666023
#define CODE_Button4off  2666022
#define CODE_Button5on   2666018
#define CODE_Button5off  2666017

// Enter a MAC address and IP address for your controller below. The IP address will be dependent on your local network:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x10, 0x1C, 0x5E }; 
IPAddress ip(192, 168, 1, 200);

// Initialize the Ethernet server library (port 80 is default for HTTP):
EthernetServer server(80);
RCSwitch mySwitch = RCSwitch();

String httpHeader;           // = String(maxLength);
int arg = 0, val = 0;        // to store get/post variables from the URL (argument and value, http:\\192.168.1.3\website?p8=1)
int lightToggle = 0;
int threshold = 500;
int clapCount = 0;

bool lightOn = false;

void setup() 
{

  //Multi color LEDE
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  
   //Init I/O-pins
   DDRD = 0xFC;              // p7..p2: output
   DDRB = 0x3F;              // p14,p15: input, p13..p8: output
   pinMode(ledPin, OUTPUT);
   pinMode(infoPin, OUTPUT);    
   
   // RF 1
   pinMode(rfPin, OUTPUT);  // Either way, we'll use pin 7 to drive the data pin of the transmitter.
   
   mySwitch.enableTransmit(rfPin);
   
   pinMode(lightSensorPin,INPUT);
   pinMode(tempSensorPin,INPUT);
   pinMode(4,INPUT);
   
   //Default states
   digitalWrite(ledPin, LOW);
   digitalWrite(infoPin, LOW);
  
   // Open serial communications and wait for port to open:
   Serial.begin(9600);


   // Start the Ethernet connection and the server:
   //Try to get an IP address from the DHCP server
   // if DHCP fails, use static address
   if (Ethernet.begin(mac) == 1) 
   {
     Serial.println("No DHCP");
   }

   Ethernet.begin(mac, ip);
  
   //Start the ethernet server and give some debug info
   server.begin();
   Serial.println("Embedded Webserver");
   Serial.println("Ethernetboard connected (pins 10, 11, 12, 13 and SPI)");
   Serial.print("Ledpin at pin "); Serial.println(ledPin);
   Serial.print("Server is at "); Serial.println(Ethernet.localIP());

}


void loop() 
{

  //Check if the lightsensor has input
  int sensorValue = analogRead(lightSensorPin);

  lightToggle = map(sensorValue, 300, 850, 0, 1);

  //Show orange light with multicolor LED
  if(lightToggle == 1)
  {
    Serial.println(sensorValue);
    mySwitch.send(CODE_Button1on,24);
    delay(200);
    mySwitch.send(CODE_Button1off,24);
    
  }

  int tempValue = digitalRead(tempSensorPin);

  int soundValue = digitalRead(4);

  if(lightToggle == 1){soundValue = HIGH;}

  if(soundValue == LOW)//Is reversed if HIGH than sensor does not detect sound
  {
      
        clapCount++;
        Serial.print("Clap!");
        delay(60);

        if(clapCount >1 && !lightOn)
        {
           mySwitch.send(CODE_Button1on,24);
           delay(50);
           
           Serial.print("Send RF code ");    
           soundValue = HIGH;
                      
           clapCount =0;
           lightOn = true;
        }
        if(clapCount >1 && lightOn)
        {
           mySwitch.send(CODE_Button1off,24);
           delay(50);

           Serial.print("Send RF code off "); 
           soundValue = HIGH;
           
           clapCount = 0;
           lightOn = false;
        }

  }
  
  // listen for incoming clients 
  EthernetClient client = server.available(); 

  //Webpage part
  if (client) 
  {
    Serial.println("New client connected");
    
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    
    while (client.connected()) 
	  {

      if (client.available()) 
	    {

        //read characters from client in the HTTP header
        char c = client.read();
        
		    //store characters to string
        
    		if (httpHeader.length() < maxLength)
    		{
    			httpHeader += c; 
    		}  
		    // don't need to store the whole Header
   
        // at end of the line (new line) and the line is blank, the http request has ended, so you can send a reply
        
        if (c == '\n' && currentLineIsBlank) //When \n is found send response
    		{
  		    // client HTTP-request received
            httpHeader.replace(" HTTP/1.1", ";");                // clean Header, and put a ; behind (last) arguments
            httpHeader.trim();                                   // remove extra chars like space
            //Serial.println(httpHeader);                          // first part of header, for debug only
               
          // send a standard http response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            
  		    //client.println("Refresh: 3");               // refresh the page automatically every 3 sec
          client.println();
          client.println("<HTML>");
          client.println("<HEAD></HEAD>");
          client.println("<BODY>");
            
            client.print("<P>LichtSensor"); client.print(": ");
            client.print(lightToggle);
            client.println("</P>");

            client.print("<P>TemperatuurSensor"); client.print(": ");
            client.print(tempValue);
            
            //grab the commands from url
            client.println("</P>");         
  
            if (parseHeader(httpHeader, arg, val)) 
  		      {   
  			        // search for argument and value, eg. p8=1
                digitalWrite(arg, val);                // Recall: pins 10..13 used for the Ethernet shield
                client.print("Pin ");client.print(arg); client.print(" = "); client.println(val);
            }
      		  else 
      		  {
      			  client.println("No IO-pins to control"); 
      		  }
  
            client.println("</P>");
            
            // end of website
            client.println("</BODY>");
            client.println("</HTML>");
            break;
  
          }
        
        if (c == '\n')
		    {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') 
		    {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }

      }

    }

    // give the web browser time to receive the data
    delay(1);
    
	// close the connection:
    client.stop();
    httpHeader = "";
    
	Serial.println("Client disconnected");
  }
}

// GET-vars after "?"   192.168.1.3/?p8=1
// parse header. Argument starts with p (only p2 .. p9)
bool parseHeader(String header, int &a, int &v)
{
  int foundIndex = header.indexOf("/");

  a = header.substring(foundIndex+1,foundIndex+2).toInt();

  foundIndex = header.indexOf("=");
  
  v = header.substring(foundIndex+1,foundIndex+2).toInt();

  switch(a)
  {
    case 7:
          if(v == 1)
          {
            mySwitch.send(CODE_Button1on,24);
            delay(50);
            Serial.print("Send RF code "+CODE_Button1on);
          }
          if(v == 0)
          {
            mySwitch.send(CODE_Button1off,24);
            delay(50);
            Serial.print("Send RF code "+CODE_Button1off);
          }
          
          break;
    case 6:
          if(v == 1)
          {
            mySwitch.send(CODE_Button2on,24);
            delay(50);
            Serial.print("Send RF code "+CODE_Button2on);
          }
          if(v == 0)
          {
            mySwitch.send(CODE_Button2off,24);
            delay(50);
            Serial.print("Send RF code "+CODE_Button2off);
          }
          break;
   case 5:
          if(v == 1)
          {
            mySwitch.send(CODE_Button3on,24);
            delay(50);
            Serial.print("Send RF code "+CODE_Button3on);
          }
          if(v == 0)
          {
            mySwitch.send(CODE_Button3off,24);
            delay(50);
            Serial.print("Send RF code "+CODE_Button3off);
          }
          break;
  }

          
}



