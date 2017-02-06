#define INPUT_SIZE 30

void setup() {
  Serial.begin(115200); //Start serial with baud rate 115200
  Serial.println("Serial begin <3"); 

  for (int x = 2; x <= 15; x++) {
    pinMode(x, OUTPUT);
    digitalWrite(x, LOW);
  }
}

void loop() {} // nothing in loop

void serialEvent() { // event listener
 while (Serial.available()){
  
    // parse serial inputs: input is a C string e.g. "A0Q, A1Q" A is the name of the solenoid (or pin) and 0 1 is low or high.  
    // string must end in Q to indicate end of string.
    char input[INPUT_SIZE + 1]; //make a buffer of length 31
    byte size = Serial.readBytesUntil('Q',input, INPUT_SIZE); // read incoming bytes until Q, put them in the buffer named input
    input[size] = 0; // add null to c string
    
    // read each command
    // we don't use & in our input string but we might want to if we have different messages. splits a C string into substrings, based on a separator character
    char* command = strtok(input, "&"); 
    while (command != 0) {

      if (command[0]) {
        int pin = command[0];
        ++command; // advance the pointer 
        int msg = atoi(command);
        solenoidSignal(pin,msg);
      }
    command = strtok(0, "&"); //goes to next substring 
    } // end while (command != 0)
  } // end while (Serial.available())
} // end void serialEvent()



void solenoidSignal(int pinName,int msg) {
  int pin = pinName - 63;
//  Serial.print(pin);
//  Serial.println(msg);
  digitalWrite(pin,msg);
}

