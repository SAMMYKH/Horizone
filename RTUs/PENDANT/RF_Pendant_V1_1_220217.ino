/*  
 RF Transmitter with ATTiny85
 
 02/09/14: Production Testing release. ID = 0001.
 10/09/14: Calibrated OSCCAL (5Dh) for best osc & stability.
 16/09/14: Send RF twice, setting OSSCAL in EEPROM loc.17.
 13/10/14: Lowered Baud rate 2400 to 1000. See if we get a better range.
 15/10/14: All set back to 8MHz, 2400b. Send rfData 3 times with 15ms in b/w.
 20/04/15: Added Firmware date code to Eeprom addr. DD=loc 21, MM=loc 22, YY=loc 23.
 05/05/15: ID is only read from EEPROM.
 06/05/15: moved eeprom mem. addresses, fixed up for proper ID read & Tx, cleaned up.
 22/02/2017: Fixed EEPROM corruption
 */

//____________________________________________________________________________________________
#define F_CPU 8000000                           // Set Clk to 8MHz

#include <crc16.h>                              // for VW
#include <VirtualWire.h>                        // Using V1.19
#include <avr/sleep.h>                          // Sleep Modes
#include <avr/power.h>                          // Power Management
#include <EEPROM.h>

const int callButton = 1;                       // PB1 - Pin6 Call Switch
const int rfData = 0;                           // PB0 - Pin5 RF data & LED
const int loBatt = 2;                           // PB2 - Pin7 Low battery
const int rfPwr = 3;                            // PB3 - Pin2 Rf power
const int lightButton = 4;                      // PB4 - Pin3 Bed light Switch

const unsigned int lightConv = 16384;           // "OR" value for light button ID (4000h = 01000000 00000000)
unsigned int msgID;
char msg[6];                                    // Message digit length (5digits + 1 = 6)

int unsigned IDnum;
int idHigh;                                     // High byte of ID num for EEPROM
int idLow;                                      // Low byte of ID num for EEPROM

ISR (PCINT0_vect){                              // Interrupt vector
}

//____________________________________________________________________________________________
void setup() {
  pinMode(callButton, INPUT);                   // Switch is input
  digitalWrite (callButton, HIGH);              // internal pull-up
  pinMode(rfData,OUTPUT);                       // rfData/LED is output
  digitalWrite(rfData, LOW);                    // turn off RF data
  pinMode(loBatt, INPUT);                       // loBatt is input
  pinMode(rfPwr, OUTPUT);                       // rfPwr is output
  digitalWrite(rfPwr, LOW);                     // turn off RF power		
  pinMode(lightButton, INPUT);                  // Switch is input
  digitalWrite (lightButton, HIGH);             // internal pull-up

  readID();                                     // Get ID from EEPROM

  if(EEPROM.read(16 == 0xFF))
    EEPROM.write(16, OSCCAL);                     // write old osccal value to addr 12
  OSCCAL = EEPROM.read(17);                     // read new from addr 13
  if(EEPROM.read(18 != OSCCAL))
    EEPROM.write(18, OSCCAL);                     // write new osccal value to addr 14

  if(EEPROM.read(121 != 0x16))  
    EEPROM.write(21, 0x16);                         // F/w date - day
  if(EEPROM.read(22 != 0x02))
    EEPROM.write(22, 02);                         // F/w date - month
  if(EEPROM.read(23 != 0x11))
    EEPROM.write(23, 11);                         // F/w date - year

  vw_setup(2400);                               // VirtualWire set-up. Bits per sec
  vw_set_tx_pin(rfData);                        // Set Tx pin to rfData

    // pin change interrupt (D0 & D4)
  PCMSK = bit (PCINT1) | bit (PCINT4);          // pins 6(PB1) and 3(PB4)
  GIFR   |= bit (PCIF);                         // clear any outstanding interrupts
  GIMSK  |= bit (PCIE);                         // enable pin change interrupts 
}

//____________________________________________________________________________________________
void loop() {
  //CALL CODE
  int bounce1 = digitalRead(callButton);	      // De-bounce - 1st read
  delay(40);								                    // Wait
  int bounce2 = digitalRead(callButton);	      // De-bounce - 2nd read
  if ((bounce1 == bounce2) && (bounce1 == LOW))	// If both bounces are the same and LOW
  {
    digitalWrite(rfPwr, HIGH);                  // turn on RF power
    delay(200);                                 // allow Rf module to settle
    msgID = IDnum;                              // insert ID number
    sendRf();                                   // send it out (1)
    delay(15);
    sendRf();                                   // send it out again. (2)
    delay(15);
    sendRf();                                   // send it out again. (3)
    digitalWrite(rfPwr, LOW);                   // turn off RF power		
    ledBlink(1,300);
    chkBat();                                   // check battery status
  }

  //BEDLIGHT CODE
  bounce1 = digitalRead(lightButton);           // De-bounce - 1st read
  delay(40);                                    // Wait
  bounce2 = digitalRead(lightButton);           // De-bounce - 2nd read
  if ((bounce1 == bounce2) && (bounce1 == LOW))	// If both bounces are the same and LOW
  {  
    digitalWrite(rfPwr, HIGH);                  // turn on RF power
    delay(200);                                 // allow Rf module to settle
    msgID = IDnum | lightConv;                  // modify ID number for Light button press
    sendRf();                                   // send it out (1)
    delay(15);
    sendRf();                                   // send it out again. (2)
    delay(15);
    sendRf();                                   // send it out again. (3)
    digitalWrite(rfPwr, LOW);                   // turn off RF power
    ledBlink(1,300);
    chkBat();                                   // Check battery status
  }  
  goToSleep ();
}

//____________________________________________________________________________________________
void goToSleep ()
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA &= ~_BV(ADEN);                         // disable ADC
  ACSR |= _BV(ACD);                             // disable the analogue comparator
  power_all_disable ();                         // power off ADC, Timer 0 / 1, serial interface
  sleep_enable();
  sleep_cpu();                                  // go to sleep

  // Wakes up here after the interrupts
  sleep_disable();                         
  power_all_enable();                           // power everything back on
}    

//____________________________________________________________________________________________
void sendRf(){
  itoa(msgID,msg,10);                           // Convert int data to Char array directly, ASCII-encoded decimal(base10) 
  vw_send((uint8_t *)msg, strlen(msg));
  vw_wait_tx();                                 // Wait until the whole message is gone
}

void ledBlink(int times, int lengthms){         // Routine for blinking a LED
  for (int x=0; x<times;x++){
    digitalWrite(rfData, HIGH);
    delay (lengthms);
    digitalWrite(rfData, LOW);
    delay(lengthms);
  }
}

void chkBat(){
  if (digitalRead (loBatt) == LOW)              // if Batt Low flash LED
    ledBlink(5, 250);                           // warning flash
}

void readID() {                                 // Reads ID no. from EEPROM
  idLow = EEPROM.read(25);	                    // Read low from addr 24
  idHigh = EEPROM.read(24);                     // Read high from addr 25
  IDnum = word(idHigh, idLow);                  // convert to one word
}

