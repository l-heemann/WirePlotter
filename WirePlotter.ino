// ===========  CONSTANTS  ===========
// NC Normalised Units, denotes lengths in x/y coords based on this normalised width/height:
float NU_WIDTH = 50;
float NU_HEIGHT = 42;

// SU Step Units, denotes lengths in stepper steps
int STEPS_PER_TURN = 2048; // Why is this half of the specced value
int SU_MIN = 0;
float TURNS_FOR_MAX_LENGTH = 3.5;
int SU_MAX = STEPS_PER_TURN * TURNS_FOR_MAX_LENGTH;

// ========== STATE ================
// int position = 0;
// int targetPosition = position;
float x = NU_WIDTH/2;
float y = NU_HEIGHT/2;
// int targetX = x;
// int targetY = y;

float r1_val = 0; 
float r2_val = 0;

// #########################  Wifi stuff #####################
#include <WiFi.h>

const char *ssid = "bornhack";

NetworkServer server(80);
void setupNetwork() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid);
  // WiFi.begin(
  //   ssid,
  //   WPA2_AUTH_TTLS,
  //   username, // identity
  //   username, // username
  //   password, // password
  //   __null, // ca_pem
  //   __null, // client_crt
  //   __null, // client_key
  //   2, // TTLS phase 2 type https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_wifi.html#_CPPv425esp_eap_ttls_phase2_types
  //   3, // channel
  //   0, // bssid
  //   true // connect
  // );

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(WiFi.status());
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());

  server.begin();
}
void handleNetCall() {
  // Serial.print("#");
  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
    while (client.connected()) {    // loop while the client's connected
      if (client.available()) {     // if there's bytes to read from the client,
        char c = client.read();     // read a byte, then
        // Serial.write(c);            // print it out the serial monitor
        if (c == '\n') {            // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<a href=\"/U\">up</a><br>");
            client.print("<a href=\"/L\">left</a> ");
            client.print("<a href=\"/R\">right</a><br>");
            client.print("<a href=\"/D\">down</a><br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // // Check to see if the client request was "GET /H" or "GET /L":
        // if (currentLine.endsWith("GET /H")) {
        //   digitalWrite(13, HIGH);  // GET /H turns the LED on
        // }
        // if (currentLine.endsWith("GET /L")) {
        //   digitalWrite(13, LOW);  // GET /L turns the LED off
        // }
                // Check to see if the client request was "GET /H" or "GET /L":
        float margin = 10.0;
        if (currentLine.endsWith("GET /R")) {
          Serial.println("Right");
          x += 1;
        }
        if (currentLine.endsWith("GET /L")) {
          Serial.println("Left");
          x -= 1;
        }
        if (currentLine.endsWith("GET /U")) {
          Serial.println("Up");
          y -= 1;
        }
        if (currentLine.endsWith("GET /D")) {
          Serial.println("Down");
          y += 1;
        }
        x = min(max(x, margin), NU_WIDTH-margin);
        y = min(max(y, margin), NU_HEIGHT-margin);
      }
    }
    // close the connection:
    client.stop();
    // Serial.println("Client Disconnected.");
  }
}

// ######################## end Wifi stuff ###################



#include <Adafruit_MotorShield.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);
Adafruit_StepperMotor *leftMotor = AFMS.getStepper(4096/2, 2); // steps per turn, side
Adafruit_StepperMotor *rightMotor = AFMS.getStepper(4096/2, 1); // steps per turn, side


void setupSteppers(){
  Serial.println("Setup steppers");

  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
  // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");

  leftMotor->setSpeed(6000);
  rightMotor->setSpeed(6000);

}

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  while (!Serial);
  Serial.println("Stepper plotter setup!");
  setupNetwork();
  setupSteppers();
  r1_val = r1(x, y);
  r2_val = r2(x, y);
}

// Circle eq for dx = 100
// sqrt(x^2 + y^2) = r1
// sqrt((x-100)^2 + y^2) = r2
float r1(float x, float y){
  return sqrt(sq(x)+sq(y));
}
float r2(float x, float y){
  // if (x > NU_WIDTH) {
  //   Serial.println("err: x argument larger than 100!");
  //   x = NU_WIDTH;
  // }
  return sqrt(sq(NU_WIDTH-x)+sq(y));
}


int NU_MAX_LINE_LENGTH = sqrt(sq(NU_WIDTH) + sq(NU_HEIGHT));
int NU_length_to_SU_length(float nu_length){
  // Assumes step 0 is min length
  float ratio = nu_length / NU_MAX_LINE_LENGTH;
  return int(floor(ratio*TURNS_FOR_MAX_LENGTH*STEPS_PER_TURN));
}

void printVal(String name, int value){
  Serial.print(name);
  Serial.print(": ");
  Serial.println(value);
}

void printVal(String name, float value){
  Serial.print(name);
  Serial.print(": ");
  Serial.println(value);
}

void loop() {
  // Serial.println("Loop");
  handleNetCall();
  moveSteppers();
}


void moveSteppers() {
  // TODO use r1/r2 instead

  float targetR1 = r1(x, y);
  float targetR2 = r2(x, y);

  Serial.print(targetR1);
  Serial.print(" ");
  Serial.print(targetR2);
  Serial.print(" ");
  Serial.print(x);
  Serial.print(" ");
  Serial.println(y);

  if (targetR1 < SU_MIN) {
    targetR1 = SU_MIN;
    Serial.print("err:X lower than min:");
    Serial.print(SU_MIN);
  }
  if (targetR2 < SU_MIN) {
    targetR2 = SU_MIN;
    Serial.print("err:Y lower than min:");
    Serial.print(SU_MIN);
  }
  if (targetR1 > SU_MAX) {
    targetR1 = SU_MAX;
    Serial.print("err:X higher than max:");
    Serial.println(SU_MAX);
  }
  if (targetR2 > SU_MAX) {
    targetR2 = SU_MAX;
    Serial.print("err:Y higher than max:");
    Serial.println(SU_MAX);
  }
  float deltaR1 = targetR1-r1_val;
  float deltaR2 = targetR2-r2_val;
  int dirR1 = FORWARD;
  if (deltaR1 < 0) {
    dirR1 = BACKWARD;
  }
  int dirR2 = FORWARD;
  if (deltaR2 < 0) {
    dirR2 = BACKWARD;
  }

  // Serial.print(deltaR1);
  // Serial.print(";");
  // Serial.println(deltaR2);

  leftMotor->step(abs(deltaR1)*108.0, dirR1, SINGLE);
  rightMotor->step(abs(deltaR2)*108.0, dirR2, SINGLE);
  r1_val = targetR1;
  r2_val = targetR2;
}