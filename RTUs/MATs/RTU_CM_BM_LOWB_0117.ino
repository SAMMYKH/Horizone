/*  
 RF Floor Transmitter with ATTiny85
 
 21/01/15: Taken from "RF_Pendant_V1_0_Oct15"
 22/01/15: Disabled all 'lightButton', Changed switch detect to High (in  Bounce routine), 
 disable pull-up on callButton.
 13/05/15: Updated with RF Pend V1.1 060515 routines
 28/05/15: Renamed 'callButton' to 'floorCall' & 'lightButton' to 'bedChair' and enabled, both triggers send the same ID.
 02/06/15: Shortened 'triggered' overall time, eliminating double send on FM trigger.
 03/06/15: Added whichMat check, PCB has LK1 that shorts bedChair to GND when not used.
 29/06/2016: R4 removed on PCB, and bedcahir pulled up.
 07/07/2016: using PB4 for ALL MATs, LEDBLINK routine removed for timeing issues, Check Battery routine modified, RF TX routine modified for better timeing and stability, whichmat removed, 
 */

#define F_CPU 8000000                           // Set Clk to 8MHz
#define rfData 0                           // PB0 - Pin5 RF data & LED
#define battery A1                           // PB2 - Pin7 Low battery
#define rfPwr 3                            // PB3 - Pin2 Rf power

#include <crc16.h>                              // for VW
#include <VirtualWire.h>                        // Using V1.19
#include <avr/sleep.h>                          // Sleep Modes
#include <avr/power.h>                          // Power Management
#include <EEPROM.h>



long batteryAlertThreshhold = 2700;


boolean loBatt;
////////////////////////////////////////////////////////////
//remmember to uncomment corresponding MAT/////////////////
//////////////////////////////////////////////////////////
//const int floorCall = 1;                        // PB4 - Pin3 Floor mat trigger
const int bedChair = 1;                         // PB1 - Pin6 Bed or Chair mat trigger

/////////////////////////////////////////////////////////////
//const unsigned int ID = 0x01;
const unsigned int lightConv = 16384;           // "OR" value for light button ID (4000h = 01000000 00000000)
char msg[7];                                    // Message digit length (5digits + 1 = 6)


int unsigned IDnum;
int idHigh;                                     // High byte of ID num for EEPROM
int idLow;                                      // Low byte of ID num for EEPROM
//int whichMat = LOW;                             // Flag for which mat has triggered
float sensorValue;

ISR (PCINT0_vect){                              // Interrupt vector
}

//____________________________________________________________________________________________
void setup() {

  pinMode(rfData,OUTPUT);                       // rfData/LED is output
  digitalWrite(rfData, LOW);                    // turn off RF data


  pinMode(rfPwr, OUTPUT);                       // rfPwr is output
  digitalWrite(rfPwr, LOW);                     // turn off RF power		

  //////////////////////////////////////////////////////////
  //Uncomment for Floor MAT and Comment for Bed/Chair MAT//
  ////////////////////////////////////////////////////////

  //pinMode(floorCall, INPUT);                    // Switch is input
  //digitalWrite (floorCall, HIGH);                   // internal pull-up

  /////////////////////////////////////////////////////////
  //Uncomment for Bed/Chair MAT and Comment for Floor MAT//
  ////////////////////////////////////////////////////////

  pinMode(bedChair, INPUT);                     // Switch is input
   digitalWrite (bedChair, HIGH);


  readID();                                     // Get ID from EEPROM

  EEPROM.write(16, OSCCAL);                     // write old osccal value to addr 12
  OSCCAL = EEPROM.read(17);                     // read new from addr 13
  if(EEPROM.read(18) != OSCCAL)
    EEPROM.write(18, OSCCAL);                     // write new osccal value to addr 14

  if(EEPROM.read(21) != 0x17)
    EEPROM.write(21, 0x17);                       // F/w date - day
  if(EEPROM.read(22) != 0x01)
    EEPROM.write(22, 0x01);                       // F/w date - month
  if(EEPROM.read(23) != 0x11)
    EEPROM.write(23, 0x11);                       // F/w date - year

  vw_setup(2400);                               // VirtualWire set-up. Bits per sec
  vw_set_tx_pin(rfData);                        // Set Tx pin to rfData

    // pin change interrupt (D0 & D4)
  PCMSK = bit (PCINT1) ;//| bit (PCINT4);          // pins 6(PB1) and 3(PB4)
  GIFR   |= bit (PCIF);                         // clear any outstanding interrupts
  GIMSK  |= bit (PCIE);                         // enable pin change interrupts 
}

//____________________________________________________________________________________________
void loop() {
/*
  //FLOOR MAT CODE
  int bounce1 = digitalRead(floorCall);	        // De-bounce - 1st read
  delay(40);								                    // Wait
  int bounce2 = digitalRead(floorCall);	        // De-bounce - 2nd read
  if ((bounce1 == bounce2) && (bounce1 == LOW))	// If both bounces are the same and "LOW"
  {
    digitalWrite(rfPwr, HIGH);                  // turn on RF power
    delay(300);                                 // allow Rf module to settle
    itoa(IDnum,msg,10);                           // Convert int data to Char array directly, ASCII-encoded decimal(base10) 
    if ((getVoltage() > batteryAlertThreshhold))             
      msg[strlen(msg)] = 'H';                    //BATTERY STATUS
    if ((getVoltage() <= batteryAlertThreshhold))             
      msg[strlen(msg)] = 'L';     
    vw_send((uint8_t *)msg, strlen(msg));
    vw_wait_tx();                                 // Wait until the whole message is gone                                   // send it out (1)
    vw_send((uint8_t *)msg, strlen(msg));
    vw_wait_tx();                                 // Wait until the whole message is gone                                  // send it out again. (2)
    vw_send((uint8_t *)msg, strlen(msg));
    vw_wait_tx();                                 // Wait until the whole message is gone                                  // send it out again. (3)
    digitalWrite(rfPwr, LOW);                   // turn off RF power	
    delay(100);	
    digitalWrite(rfData, HIGH);
    delay(1000);
    digitalWrite(rfData, LOW);  
    chkBat(5,200);                                   // check battery status 
  }*/
  
  //BEDCHAIR CODE
   int   bounce1 = digitalRead(bedChair);	            // De-bounce - 1st read
   delay(40);								                    // Wait
   int   bounce2 = digitalRead(bedChair);	            // De-bounce - 2nd read
   if ((bounce1 == bounce2) && (bounce1 == HIGH))    // If both bounces are the same and "LOW"+
   {
   
   digitalWrite(rfPwr, HIGH);                  // turn on RF power
   delay(300);                                 // allow Rf module to settle
   itoa(IDnum,msg,10);                           // Convert int data to Char array directly, ASCII-encoded decimal(base10) 
   if ((getVoltage() > batteryAlertThreshhold))             
   msg[strlen(msg)] = 'H';                    //BATTERY STATUS
   if ((getVoltage() <= batteryAlertThreshhold))             
   msg[strlen(msg)] = 'L';     
   vw_send((uint8_t *)msg, strlen(msg));
   vw_wait_tx();                                 // Wait until the whole message is gone                                   // send it out (1)
   vw_send((uint8_t *)msg, strlen(msg));
   vw_wait_tx();                                 // Wait until the whole message is gone                                  // send it out again. (2)
   vw_send((uint8_t *)msg, strlen(msg));
   vw_wait_tx();                                 // Wait until the whole message is gone                                  // send it out again. (3)
   digitalWrite(rfPwr, LOW);                   // turn off RF power	
   delay(100);	
   digitalWrite(rfData, HIGH);
   delay(1000);
   digitalWrite(rfData, LOW);  
   chkBat(5,200);                                   // check battery status 
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
long getVoltage(){

  ADCSRA = bit(ADEN);
  ADMUX = _BV(MUX3) | _BV(MUX2);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000

  ADMUX&=~(REFS0);
  ADMUX&=~(REFS1);
  return result; // Vcc in millivolts
}


void chkBat(int times, int lengthms)
{
  if ((getVoltage() <= batteryAlertThreshhold) ) // if the battery is below the threshold and the button wasnt pressed, then beep
  {
    delay (500);
    for (int x=0; x<times;x++)
    {
      digitalWrite(rfData,HIGH );
      delay (lengthms);
      digitalWrite(rfData, LOW);
      delay (lengthms/2);
    }                          // warning flash
  }
}


void readID() {                                 // Reads ID no. from EEPROM
  idLow = EEPROM.read(25);	                    // Read low from addr 25
  idHigh = EEPROM.read(24);                     // Read high from addr 24
  IDnum = word(idHigh, idLow);                  // convert to one word
}



/*
void sendRf(){
 itoa(msgID,msg,10);                           // Convert int data to Char array directly, ASCII-encoded decimal(base10) 
 vw_send((uint8_t *)msg, strlen(msg));
 vw_wait_tx();                                 // Wait until the whole message is gone
 }
 
 void ledBlink(int times, int lengthms){         // Routine for blinking a LED
 for (int x=0; x<times;x++){
 digitalWrite(rfData,HIGH );
 delay (lengthms);
 digitalWrite(rfData, LOW);
 }
 }
 */

//______________________________________________________________________________























