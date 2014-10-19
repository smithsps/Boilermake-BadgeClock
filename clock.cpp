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

void networkRead(void);

void setLEDS(word);
void handlePayload(struct payload *);

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
unsigned long startTime = 0;

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

  startTime = millis();
  // 
  digitalWrite(SROEPin, LOW);
}


// This loops forever
void loop() {
  if (Serial && !terminalConnect) { 
    terminalConnect = true;
    sync_time_serial();
  } else if (!Serial && terminalConnect) {
    terminalConnect = false;
  }
  
  
  displayTime();
}

void displayTime() {
  unsigned long time = millis() - startTime;  
  int hour = (time / 3600000) % 12;
  int minute = (time / 60000) % 60;
  int second = (time / 1000) % 60;
  
  if (second % 2) {  
    setLEDS(ledNum(toLED16(hour)) | ledNum(toLED16(minute / 5)));
  } else {
    setLEDS(ledNum(toLED16(hour)));
  }
  delay(1000);
  if (minute % 5 == 0 && second == 0) {
    for (int i = 0; i < 16; i++) {
      setLEDS(ledNum((i + 9 + hour) % 16 + 1));
      delay((int) 1000/16);
    }
  } 

  Serial.print(millis());
  Serial.print(" : ");
  Serial.print(time);
  Serial.print(" : ");
  Serial.print(hour);
  Serial.print(" : ");
  Serial.println(minute);
}


//12 hour clock, but 16 LEDs
int toLED16(int i) {
  int ret = i; 

  //We have to not use 4 of the 16 for a 12 clock
  if (ret >= 3) ret += 1;
  if (ret >= 5) ret += 1;
  if (ret >= 11) ret += 1;
  if (ret >= 13) ret += 1;
  ret += 9;  
  return ret % 16;
}

//Wait a bit and see if we get serial time sync
void sync_time_serial() {
  
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

setLEDS data in hex: 0xAABB
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

// LED display demo
void displayDemo() {
  digitalWrite(SROEPin, LOW);
  for (int i = 0; i < 4; i++) {
    setLEDS(0xAAAA);
    delay(125);
    setLEDS(0x5555);
    delay(125);
  }
  digitalWrite(SROEPin, HIGH);
}

// Sends word sized value to both SRs & latches output pins
void setLEDS(word value) {
  byte Hvalue = value >> 8;
  byte Lvalue = value & 0x00FF;
  SPI.transfer(Lvalue);
  SPI.transfer(Hvalue);
  digitalWrite(SRLatchPin, HIGH);
  digitalWrite(SRLatchPin, LOW);
}
