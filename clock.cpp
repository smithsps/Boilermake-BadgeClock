/*
 * This badge clock was coded by none other than Simon Smith and Cameron Kachur.
 * It does clock things such as tell time and.........................
 * You wish you could make a clock badgerooni like this.
 * Happy clocking and turn down for only dead batteries.
 * Boilermake 2014 - All clock no sleep
 */

#define MAX_TERMINAL_LINE_LEN 40
#define MAX_TERMINAL_WORDS     7

// 14 is strlen("send FFFF -m ")
// the max message length able to be sent from the terminal is
// total terminal line length MINUS the rest of the message
#define MAX_TERMINAL_MESSAGE_LEN  MAX_TERMINAL_LINE_LEN - 14

#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>

void welcomeMessage(void);
void printHelpText(void);
void printPrompt(void);

void networkRead(void);
void serialRead(void);
void handleSerialData(char[], byte);

void setValue(word);
void handlePayload(struct payload *);

void ledDisplay(byte);
void displayDemo();

// Maps commands to integers
const byte PING   = 0; // Ping
const byte LED    = 1; // LED pattern
const byte MESS   = 2; // Message
const byte DEMO   = 3; // Demo Pattern

// Global Variables
int SROEPin = 3; // using digital pin 3 for SR !OE
int SRLatchPin = 8; // using digital pin 4 for SR latch
boolean terminalConnect = false; // indicates if the terminal has connected to the board yet

// nRF24L01 radio static initializations
RF24 radio(9,10); // Setup nRF24L01 on SPI bus using pins 9 & 10 as CE & CSN, respectively

uint16_t this_node_address = (EEPROM.read(0) << 8) | EEPROM.read(1); // Radio address for this node

struct payload{ // Payload structure
  byte command;
  byte led_pattern;
  char message[30];
};

// This runs once on boot
void setup() {
  Serial.begin(9600);

  // SPI initializations
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); // nRF requires MSB first
  SPI.setClockDivider(16); // Set SPI clock to 16 MHz / 16 = 1 MHz

  // nRF radio initializations
  radio.begin();
  radio.setDataRate(RF24_1MBPS); // 1Mbps transfer rate
  radio.setCRCLength(RF24_CRC_16); // 16-bit CRC
  radio.setChannel(80); // Channel center frequency = 2.4005 GHz + (Channel# * 1 MHz)
  radio.setRetries(200, 5); // set the delay and number of retries upon failed transmit
  radio.openReadingPipe(0, this_node_address); // Open this address
  radio.startListening(); // Start listening on opened address

  // Shift register pin initializations
  pinMode(SROEPin, OUTPUT);
  pinMode(SRLatchPin, OUTPUT);
  digitalWrite(SROEPin, HIGH);
  digitalWrite(SRLatchPin, LOW);


  // make the pretty LEDs happen
  displayDemo();
}


// This loops forever
void loop() {
  // Displays welcome message if serial terminal connected after program setup
  if (Serial && !terminalConnect) { 
    terminalConnect = true;
  } else if (!Serial && terminalConnect) {
    terminalConnect = false;
  }
digitalWrite(SROEPin, LOW);
  displayTime();
digitalWrite(SROEPin, HIGH);
}

void displayTime() {
  int hour = 8;
  int minute = 8;
  int minCount = 0;
  setValue(ledNum(minute + 1) | ledNum(hour + 1));
  while (1) {
      while (minCount < 12) {
        int minuteBlink = 0;
        while (minuteBlink < 3000) {
          setValue(ledNum(hour + 1));
          delay(1000);
          setValue(ledNum(minute + 1) | ledNum(hour + 1));
          delay(1000);
          minuteBlink += 20;
        }
        if (minute + 1 == 3 || minute + 1 == 5 || minute + 1 == 11 || minute + 1 == 13) {
          minute += 2;
        } else {
          minute = (minute + 1) % 16;
        }
        minCount++;
        setValue(ledNum(minute + 1) | ledNum(hour + 1));
      }
      if (hour + 1 == 3 || hour + 1 == 5 || hour + 1 == 11 || hour + 1 == 13) {
        hour += 2 ;
      } else {
        hour = (hour + 1) % 16;
      }
      minCount = 0;
  }
}


// Handle reading from the radio
void networkRead() {
  while (radio.available()) {
    struct payload * current_payload = (struct payload *) malloc(sizeof(struct payload));

    // Fetch the payload, and see if this was the last one.
    radio.read( current_payload, sizeof(struct payload) );
    handlePayload(current_payload);
  }
}

// Grab message received by nRF for this node
void handlePayload(struct payload * myPayload) {
  switch(myPayload->command) {

    case PING:
      Serial.println("Someone pinged us!");
      printPrompt();
      break;

    case LED:
      ledDisplay(myPayload->led_pattern);
      break;

    case MESS:
      Serial.print("Message:\r\n  ");
      Serial.println(myPayload->message);
      printPrompt();
      break;

    case DEMO:
      displayDemo();
      break;

    default:
      Serial.println(" Invalid command received.");
      break;
  }
  free(myPayload); // Deallocate payload memory block
}

void printPrompt(void){
  Serial.print("> ");
}

/*
// Display LED pattern

// LED numbering:

           9
       8       10
     7           11
   6               12
  5                 13
   4               14
     3           15
       2       16
           1

shift register 1-8: AA
shift register 9-16: BB

setValue data in hex: 0xAABB
where AA in binary = 0b[8][7][6][5][4][3][2][1]
      BB in binary = 0b[16][15][14][13][12][11][10][9]

*/
word ledNum(int i) {
  word shit[17];
  shit[0] = 0x0000;

  shit[1] = 0x0100;
  shit[2] = 0x0200;
  shit[3] = 0x0400;
  shit[4] = 0x0800;
  shit[5] = 0x1000;
  shit[6] = 0x2000;
  shit[7] = 0x4000;
  shit[8] = 0x8000;

  shit[9] =  0x0001;
  shit[10] = 0x0002;
  shit[11] = 0x0004;
  shit[12] = 0x0008;
  shit[13] = 0x0010;
  shit[14] = 0x0020;
  shit[15] = 0x0040;
  shit[16] = 0x0080;

  return shit[i];
}

void ledDisplay(int i) {
  
  setValue(ledNum(i));
  delay(62);
  
}


// LED display demo
void displayDemo() {
  digitalWrite(SROEPin, LOW);
  for (int i = 0; i < 4; i++) {
    setValue(0xAAAA);
    delay(125);
    setValue(0x5555);
    delay(125);
  }
  digitalWrite(SROEPin, HIGH);
}

// Sends word sized value to both SRs & latches output pins
void setValue(word value) {
  byte Hvalue = value >> 8;
  byte Lvalue = value & 0x00FF;
  SPI.transfer(Lvalue);
  SPI.transfer(Hvalue);
  digitalWrite(SRLatchPin, HIGH);
  digitalWrite(SRLatchPin, LOW);
}
