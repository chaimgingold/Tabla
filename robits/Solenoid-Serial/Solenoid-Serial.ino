#define INPUT_SIZE 30

void setup() {
  Serial.begin(115200); //Start serial with baud rate 115200
  Serial.println("Serial begin <3"); 
  
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}


void loop() {} // nothing in loop

void serialEvent() { // event listener
 while (Serial.available()){
  
    // parse serial inputs: input is a C string e.g. "HQ" 
    // string must end in Q to indicate end of string.
    char input[INPUT_SIZE + 1]; //make a buffer of length 31
    byte size = Serial.readBytesUntil('Q',input, INPUT_SIZE); // read incoming bytes until Q, put them in the buffer named input
    input[size] = 0; // add null to c string
    
    // read each command
    // we don't use & in our input string but we might want to if we have different messages. splits a C string into substrings, based on a separator character
    char* command = strtok(input, "&"); 
    while (command != 0) { 
      if (command[0] == 'H') {
        ++command; // advance the pointer 
        digitalWrite(13, HIGH);   // send HIGH to pin 13 - ON.
        delay(100);              
        digitalWrite(13, LOW);    // send LOW
      }  
    command = strtok(0, "&"); //goes to next substring 
    } // end while (command != 0)
  } // end while (Serial.available())
} // end void serialEvent()

