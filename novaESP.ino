#include <ESP8266WiFi.h>
#include <HX711.h>
#include <Servo.h> 
#include <DHT.h>

#define DHTTYPE DHT11
#define DHTPIN 2
#define ledica 13
#define FloatSensor 12
#define servoPinVoda 0
#define servoPinHrana 15
#define LOADCELL_DOUT_PIN 16
#define LOADCELL_SCK_PIN 14

#define ssid "Baric hotspot"
#define password "baric1995"

HX711 scale;
DHT dht(DHTPIN, DHTTYPE);
Servo ServoVoda;
Servo ServoHrana; 

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

//variables to store the current output state
String outputVodaState = "ZATVORENO";
String outputHranaState = "ZATVORENO";
String outputTarirano = "NE";

void setup() 
{
  Serial.begin(9600);
  delay(100);

  pinMode(ledica, OUTPUT);
  pinMode(FloatSensor,INPUT_PULLUP);
  pinMode(DHTPIN, INPUT);
  dht.begin();
  ServoVoda.attach(servoPinVoda);
  ServoHrana.attach(servoPinHrana);
  ServoVoda.write(0);
  ServoHrana.write(0);
  delay(200);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Serial.println("Uklonite sve utege s vage");
  Serial.println("Pritisnite t za tariranje");
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  //Connect to your local wi-fi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 
  server.begin();
  Serial.println("HTTP server started");

}

void loop()
{
  int voda = read_floatSensor();
  float temp = read_temperature();
  float masa = get_grams();


  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) // If a new client connects,
  {                            
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) { // loop while the client's connected     
      if (client.available())  // if there's bytes to read from the client,
      {            
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') // if the byte is a newline character
        {                    
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) 
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Refresh: 1");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /VODA/on") >= 0) 
            {
              Serial.println("GPIO Voda on");
              outputVodaState = "OTVORENO";
              ServoVoda.write(90);            
            } 
            
            else if (header.indexOf("GET /VODA/off") >= 0) 
            {
              Serial.println("GPIO Voda off");
              outputVodaState = "ZATVORENO";
              ServoVoda.write(0);
            } 
            
            else if (header.indexOf("GET /HRANA/on") >= 0) 
            {
              Serial.println("GPIO Hrana on");
              outputHranaState = "OTVORENO";
              ServoHrana.write(90);
            } 
            
            else if (header.indexOf("GET /HRANA/off") >= 0)
            {
              Serial.println("GPIO Hrana off");
              outputHranaState = "ZATVORENO";
              ServoHrana.write(0);
            }
            else if (header.indexOf("GET /TARIRA/on") >= 0)
            {
              Serial.println("TARIRANO");
              outputTarirano = "DA";
              scale.tare();
            }
            else if (header.indexOf("GET /TARIRA/off") >= 0)
            {
              Serial.println("NE TARIRAM");
              outputTarirano = "NE";
              //scale.tare();
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #008000; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #FF0000;}"); 
            client.println(".button3 {background-color: #FF0000;}</style></head>"); 
            
            // Web Page Heading
            //client.println("<img src=https://www.ferit.unios.hr/new-images/ferit-logo-350.png>");
            client.println("<body><h1>Pet Feeder</h1>");
            
             
            client.println("<p>Razina vode: (0 - ima vode; 1 - nema vode)</p>");
            client.println("<p><b>" + String(voda) + "</b></p>");
            
            
            client.println("<p>Temperatura okruzenja: </p>");
            client.println("<p><b> " + String(temp) + " *C" "</b></p>");

            client.println("<p>Tezina hrane: </p>");
            client.println("<p><b> " + String(masa) + " grama" "</b></p>");
            
            // Display current state, and ON/OFF buttons for GPIO 5  
            client.println("<p>Servo Voda - Status " + outputVodaState + "</p>");         
            // If the outputVodaState is ZATVORENO, it displays the Otvori button       
            if (outputVodaState == "ZATVORENO") 
            {
              client.println("<p><a href=\"/VODA/on\"><button class=\"button\">Otvori</button></a></p>");
            } 
            else 
            {
              client.println("<p><a href=\"/VODA/off\"><button class=\"button button2 button3\">Zatvori</button></a></p>");
            } 
               
            // Display current state, and OTVORENO/ZATVORENO buttons for SERVO HRANA  
            client.println("<p>Servo Hrana - Status " + outputHranaState + "</p>");
            // If the outputHranaState is ZAT, it displays the Otvori button       
            if (outputHranaState == "ZATVORENO") 
            {
              client.println("<p><a href=\"/HRANA/on\"><button class=\"button\">Otvori</button></a></p>");
            }
            else 
            {
              client.println("<p><a href=\"/HRANA/off\"><button class=\"button button2 button3\">Zatvori</button></a></p>");
            }


            if (outputTarirano == "NE") 
            {
              client.println("<p><a href=\"/TARIRA/on\"><button class=\"button\">TARIRAJ</button></a></p>");
            }
            else 
            {
              client.println("<p><a href=\"/TARIRA/off\"><button class=\"button button2 button3\">BACK</button></a></p>");
            }
            client.println("<body style=background-color:powderblue;>");
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } 
          else 
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } 
        else if (c != '\r') 
        {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}


float get_grams()
{
  long t;
  float calibration_factor = -841650;
  float masa, grams;
  scale.set_scale(calibration_factor);
  if (millis() > t + 1000) { //funkcija za pauzu od 1 sekunde koja ne zaustavlja program kao delay, nego samo svake sekunde ispisuje očitanje, ali program se stalno izvršava
    Serial.print("Ocitavanje: ");
    masa = scale.get_units();
    grams = 1000 * masa;
    Serial.print(grams, 3);
    Serial.print(" grams"); 
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();
    t = millis();
  }
  if(Serial.available())
  {
  char tmp = Serial.read();
  //ako je poslano slovo t onda vagu postavimo na 0(tariramo)
  if(tmp == 't')
  scale.tare();
  }
  return grams;
}


int read_floatSensor()
{
  long vrijeme;
  bool buttonStateVoda;
  buttonStateVoda = digitalRead(FloatSensor);
  if (millis() > vrijeme + 1000) 
  { 
    if (buttonStateVoda == 0)
    {
      Serial.println("IMA vode!!!");
      ServoVoda.write(0);
      digitalWrite(ledica, buttonStateVoda);
    }
    if (buttonStateVoda == 1)
    {
      Serial.println("UPOZORENJE!!! Nema vode!!!");
      digitalWrite(ledica, buttonStateVoda);
    }
    vrijeme = millis();
  }
  return buttonStateVoda;
}


float read_temperature()
{
  float temperature;
  temperature = dht.readTemperature();
  if (isnan(temperature))
  {
    Serial.print("Neuspješno očitavanje senzora! ");
  }
  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.print("*C");
  Serial.println();
  return temperature;
}
