/*  
 RF Transmitter with ATTiny85 for the PIR
 
 20/05/15: Copied the 'RF_Pendant_V1_1_060515', removed the Sleep, Check battery, & Bedlight functions.
 11/04/2017: Added Low Battery Warning. added Intrrupt.
 */

//____________________________________________________________________________________________
#define F_CPU 8000000                           // Set Clk to 8MHz

#include <crc16.h>                              // for VW
#include <VirtualWire.h>                        // Using V1.19
#include <avr/sleep.h>                          // Sleep Modes
#include <avr/power.h>                          // Power Management
#include <EEPROM.h>

long batteryAlertThreshhold = 3190;

const int callButton = 1;                       // PB1 - Pin6 Call Switch
const int rfData = 0;                           // PB0 - Pin5 RF data & LED
const int rfPwr = 3;                            // PB3 - Pin2 Rf power
//const int lightButton = 4;                      // PB4 - Pin3 Bed light Switch

//const unsigned int lightConv = 16384;           // "OR" value for light button ID (4000h = 01000000 00000000)

char msg[7];                                    // Message digit length (6digits + 1 = 7)

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
   pinMode(rfPwr, OUTPUT);                       // rfPwr is output
  digitalWrite(rfPwr, LOW);                     // turn off RF power		
//  pinMode(lightButton, INPUT);                  // Switch is input
//  digitalWrite (lightButton, HIGH);             // internal pull-up

  readID();                                     // Get ID from EEPROM

  EEPROM.write(16, OSCCAL);                     // write old osccal value to addr 12
  OSCCAL = EEPROM.read(17);                     // read new from addr 13
 if(EEPROM.read(18) != OSCCAL)
    EEPROM.write(18, OSCCAL);                     // write new osccal value to addr 14

  if(EEPROM.read(21) != 0x0B)
    EEPROM.write(21, 0x0B);                         // F/w date - day
  if(EEPROM.read(22) != 0x04)
    EEPROM.write(22, 0x04);                         // F/w date - month
  if(EEPROM.read(21) != 0x11)
    EEPROM.write(23, 0x11);                         // F/w date - year

  vw_setup(2400);                               // VirtualWire set-up. Bits per sec
  vw_set_tx_pin(rfData);                        // Set Tx pin to rfData

    // pin change interrupt (D0 & D4)
  PCMSK = bit (PCINT1);// | bit (PCINT4);          // pins 6(PB1) and 3(PB4)
  GIFR   |= bit (PCIF);                         // clear any outstanding interrupts
  GIMSK  |= bit (PCIE);                         // enable pin change interrupts 
  sei();
}

//____________________________________________________________________________________________
void loop() {
  //Trigger on Call input
  int bounce1 = digitalRead(callButton);	// De-bounce - 1st read
  delay(40);					// Wait
  int bounce2 = digitalRead(callButton);	// De-bounce - 2nd read
  if ((bounce1 == bounce2) && (bounce1 == LOW))	// If both bounces are the same and LOW
  {
     digitalWrite(rfPwr, HIGH);                  // turn on RF power
    delay(300);                                 // allow Rf module to settle
    itoa(IDnum,msg,10);                           // Convert int data to Char array directly, ASCII-encoded decimal(base10) 
    if ((getVoltage() > batteryAlertThreshhold))             
      msg[strlen(msg)] = 'H';                    //BATTERY STATUS
    if ((getVoltage() < batteryAlertThreshhold))             
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
    chkBat(5,300);                                   // check battery status 
    //delay(500);                                 // Try to stop extra triggers
  }
  goToSleep();
}

//____________________________________________________________________________________________
/*void sendRf(){
 itoa(IDnum,msg,10);                           // Convert int data to Char array directly, ASCII-encoded decimal(base10) 
 vw_send((uint8_t *)msg, strlen(msg));
 vw_wait_tx();                                 // Wait until the whole message is gone
 }
 */
void ledBlink(int times, int lengthms){         // Routine for blinking a LED
  for (int x=0; x<times;x++){
    digitalWrite(rfData, HIGH);
    delay (lengthms);
    digitalWrite(rfData, LOW);
    delay(lengthms);
  }
}
//______________________________________________________________________________________________
void readID() {                                 // Reads ID no. from EEPROM
  idLow = EEPROM.read(25);	        // Read low from addr 24
  idHigh = EEPROM.read(24);               // Read high from addr 25
  IDnum = word(idHigh, idLow);            // convert to one word
}
//_________________________________________________________________________________________________
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

//_____________________________________________________________________________________

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
//___________________________________________________________________________________

void chkBat(int times, int lengthms)
{
  if ((getVoltage() < batteryAlertThreshhold) ) // if the battery is below the threshold and the button wasnt pressed, then beep
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


