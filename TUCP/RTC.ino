#define F_CPU 16000000              // Set Clk to 16MHz
#define holdtime 5                           // delay for entering setup page
#define SWDAY   0x1A                //26
#define SWMOTH  0x04                //04
#define SWYR 0x11                   //2017

// I/O PORTS
#define rfData  70                      // PJ0 RF data
#define ledIn  9                       // PH6 LED trigger Input (UCP)
#define extIn  8                       // PH5 External Input (UCP)
#define pendIn  A8                      // PK0 Pendant button press (UCP)
#define poAlarm  7                      // PH4 Pendant Out Alarm Input
#define callOut  10                     // PB4 Call Output (UCP)
#define cancelOut  11                    // PB5 Cancel/Zone/Dome Output (UCP)
#define beep  A3                         // PF7 Beeper
#define bliteOut 72                     // PE6 RF Bed light Output (UCP)

#define SD_CS_PIN SS

#include <UTFT.h>             // ITEAD Display Library
#include <UTouch.h>             // ITEAD Touch Library
#include <EEPROM.h>
#include <Arduino.h>
#include <crc16.h>                 // for VW
#include <VirtualWire.h>          // RF Library
#include <SdFat.h>               // SD Card Library
#include <SPI.h>
#include <DS3231.h>              //Real Time control Library

char BoardSerial[6] = "99999";               // must be define as board serial number in case of lost password

// SoftWare Version.
char SWver[16] = "3.01";


// Initialize display
// ------------------
// Set the pins to the correct ones for your board
// -----------------------------------------------------------
// Standard Arduino MEGA             : <display model>,38, 39, 40, 41 (TRS(A5), TWR(A4), TCS(A3), TRST(A2))
//
// Remember to change the model parameter to suit your display module!
UTFT    myGLCD(ITDB32S, 38, 39, 40, 41);

// Initialize touchscreen
// ----------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino MEGA           : 6, 5, 4, 3, 2(TCLK(A1), TCS(A3), TDin(A0),TIRQ(A7), TDOUT(A6))
// ElecHouse TFT LCD/SD Shield for Arduino Due : 25,26,27,29,30
//
UTouch  myTouch( 6, 5, 4, 3, 2);

// Init the DS3231 using the hardware interface (Real Time Controller)
DS3231  rtc(20, 21);



// Declare which fonts we will be using
extern uint8_t BigFont[];
extern uint8_t SmallFont[];
extern uint8_t INDIGO[];
extern uint8_t indigo[];

//SD card Variables
SdFat SD;
File myFile;
File OldFile;
File OlderFile;
File OldestFile;
uint32_t volumesize;
boolean FirstLog = false;
boolean SecondLog = false;
int daycount = 0;
int FileSelect;

boolean PENDANTCALL;
//TIME VARIABLES
boolean SetTimeFlag;
int HOUR, Hour;
int MIN, Min;
int SEC, Sec;
int DAY, Day;
int MONTH, Month;
int YEAR, Year;
boolean DayFlag;
boolean CLOCKFLAG;

// LCD & Touch VARIABLES
int x, y, cnt, hold, mode, addr, addrName;
int stCurrentLen = 0;
int PAGE = 1;
char stCurrent[16];
char stLast[16];
char  master[16] ;
char ROOMNO[16];
char stName[16]  ;
char TIME;
boolean CancelFlag = false;
boolean SetupFlag = false;
boolean PassWordFlag = false;
boolean SETPASS = false;
boolean SetFlag = false;
boolean SDflag = false;
boolean timeoutFlag = false;
boolean poAStat = false;
boolean modeFlag = false;
boolean wrongpassFlag = false;
boolean BeepFlag = false;
boolean PASSFLAG , ROOMFLAG , SetRoom, NAMEFLAG;

///////////////////////////////////////////////////////////////////////////////////////////////
//RF VARIABLES
///////////////////////////////////////////////////////////////////////////////////////////////
int unsigned rfIDnum;                       // Received ID number
char CharMsg[2];                            // RF Transmission container
boolean whichButton;                        // Call (true) or Bed light (false) press from Tx
boolean RF_RX = false;
// MEMORY VARIABLES
int idHigh;                                 // High byte of ID num for EEPROM
int idLow;                                  // Low byte of ID num for EEPROM

int memPointer;                             // Memory location count
int memlocH;                                // Memory location High
int memlocL;                                // Memory location Low

int unsigned id1;                           // Copy of ID in loc1
int unsigned id2;                           // Copy of ID in loc2
int unsigned id3;                           // Copy of ID in loc3
int unsigned id4;                           // Copy of ID in loc4
int unsigned id5;                           // Copy of ID in loc5

int unsigned loc1H = 24;                    // Mem. location 1 High byte
int unsigned loc1L = 25;                    // Mem. location 1 Low byte
int unsigned loc2H = 26;                    //  "       "    2 High "
int unsigned loc2L = 27;                    //  "       "    2 Low "
int unsigned loc3H = 28;                    //  "       "    3 High "
int unsigned loc3L = 29;                    //  "       "    3 Low "
int unsigned loc4H = 30;                    //  "       "    4 High "
int unsigned loc4L = 31;                    //  "       "    4 Low "
int unsigned loc5H = 32;                    //  "       "    5 High "
int unsigned loc5L = 33;                    //  "       "    5 Low "




/*************************
**   Custom functions   **
*************************/

/********************************update Str**********************************/
void updateStr(int val)
{
  if (stCurrentLen < 15)
  {
    stCurrent[stCurrentLen] = val;
    stCurrent[stCurrentLen + 1] = '\0';
    stCurrentLen++;
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(255, 255, 255);
    if (EEPROM.read(71))
      myGLCD.print(stCurrent, LEFT, 224);
    if (!EEPROM.read(71))
      myGLCD.print(stCurrent, LEFT, 300);
  }
  else
  {
    if (EEPROM.read(71))
    {
      myGLCD.setColor(255, 0, 0);
      myGLCD.setBackColor(255, 255, 255);
      myGLCD.print("Many Letters!", CENTER, 205);
      delay(500);
      myGLCD.print("             ", CENTER, 205);
      delay(500);
      myGLCD.print("Many Letters!", CENTER, 205);
      delay(500);
      myGLCD.print("             ", CENTER, 205);
    }
    if (!EEPROM.read(71))
    {
      myGLCD.setColor(255, 0, 0);
      myGLCD.setBackColor(255, 255, 255);
      myGLCD.print("Many Letters!", CENTER, 245);
      delay(500);
      myGLCD.print("             ", CENTER, 245);
      delay(500);
      myGLCD.print("Many Letters!", CENTER, 245);
      delay(500);
      myGLCD.print("             ", CENTER, 245);
    }
    myGLCD.setColor(0, 255, 0);
  }
}
/*******************************Button Flashing*****************************/

void waitForIt(int x1, int y1, int x2, int y2)
{
  if (BeepFlag)
  {
    digitalWrite(beep, HIGH);
    delay(100);
    digitalWrite(beep, LOW);
  }
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(0, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}

/*******************************SD Card File remove***********************************/
void FileRemove()
{
  Time  t;
  t = rtc.getTime();      //Get present time from RTC
  int Day = (t.date);
  int Month = (t.mon);
  int Hour = (t.hour);
  int Min = (t.min);
  int Sec = (t.sec);

  if ((Day == 1) && (Hour == 00) && (Min == 00) && ( Sec == 1))  // check if it is then of the month
  {
    delay(1000);
    daycount = daycount + 1;
  }

  if ((daycount == 1) && SD.exists("LOG.txt"))
  {
    myFile = SD.open("LOG.txt", FILE_WRITE);
    myFile.rename(SD.vwd(), "30_Days_Ago.txt");
    myFile.close();
    myFile = SD.open("LOG.txt", FILE_WRITE);
    myFile.close();

  }
  if ((daycount == 2) && SD.exists("30_Days_Ago.txt"))         // check if it is then of the month
  {
    myFile = SD.open("30_Days_Ago.txt", FILE_WRITE);
    myFile.rename(SD.vwd(), "60_Days_Ago.txt");
    myFile.close();
    myFile = SD.open("LOG.txt", FILE_WRITE);
    myFile.rename(SD.vwd(), "30_Days_Ago.txt");
    myFile.close();
    myFile = SD.open("LOG.txt", FILE_WRITE);
    myFile.close();
    daycount = 0;
  }
}
/*****************************CALL LOG***************************************/
void CallLog()
{
  myFile = SD.open("LOG.txt", FILE_WRITE);
  myFile.print("Name: ");
  myFile.println(stName);
  myFile.println("");
  myFile.print("Date: ");
  myFile.println(rtc.getDateStr());
  if (!PENDANTCALL)
  {
    switch (EEPROM.read(102))
    {
      case 1:
        myFile.print("CALL Time: ");
        break;
      case 2:
        myFile.print("ASSIST Time: ");
        break;
      case 3:
        myFile.print("EMERGENCY Time: ");
        break;
    }
  }
  else
    myFile.print("PENDANT CALL Time: ");
  myFile.println(rtc.getTimeStr());
  myFile.println("");
  myFile.close();
}
/********************************CANCEL LOG *************************************/
void CancelLog()
{
  myFile = SD.open("LOG.txt", FILE_WRITE);
  myFile.print("Date: ");
  myFile.println(rtc.getDateStr());
  myFile.print("CANCEL Time:  ");
  myFile.println(rtc.getTimeStr());
  myFile.println("");
  myFile.println("********************************************************");
  myFile.close();
}
/********************************CALL PROFILE OPERATION*******************************/

void MOM_Call()                               // MOMENTARY Call routine
{
  boolean ledStat;

  digitalWrite(callOut, HIGH);              // Set Call O/p high - triggered state
  //  digitalWrite(cancelOut, HIGH);            // Set Cancel O/p high - triggered state
  digitalWrite(beep, HIGH);                 // Beeper on
  CancelFlag = true;
  delay(200);
  digitalWrite(beep, LOW);                  // Beeper off
  delay(300);
  digitalWrite(callOut, LOW);               // Set Call O/p low - Idle state aftyer 500 millisenconds
  ledStat = digitalRead(ledIn);       // Aknowledgment Pulse input, Active Low
  if (!ledStat)                // If No Aknowledgment, go back to CALL page with warning BeepFlag
  {
    drawLayer();
    CancelFlag = false;
    digitalWrite(beep, HIGH);         // Beeper On
    delay(500);
    digitalWrite(beep, LOW);                  // Beeper off
    delay(500);
    digitalWrite(beep, HIGH);         //Beeper On
    delay(500);
    digitalWrite(beep, LOW);          //Beeper Off
  }
  else
    drawCANCEL();               // draw cancel page
  if (SDflag)                 // If SD card is available, LOG the CALL
    CallLog();
}

void LATCH_Call()                            //LATCH Call routine
{
  digitalWrite(callOut, HIGH);              // K1 is Momentary output
  digitalWrite(cancelOut, HIGH);            // K2 (Cancel) - latched
  digitalWrite(beep, HIGH);                 // Beeper on
  CancelFlag = true;
  delay(200);
  digitalWrite(beep, LOW);                  // Beeper off
  delay(300);
  digitalWrite(callOut, LOW);               // Set Call O/p low - Idle state
  drawCANCEL();               // draw cancel page
  if (SDflag)               // If SD card is available, LOG the CALL
    CallLog();
}

void D_LATCH_Call()                        //Dual Latch Call routine
{
  digitalWrite(callOut, HIGH);              // K1 Dome (Call) - latched
  digitalWrite(beep, HIGH);                 // Beeper on
  CancelFlag = true;
  delay(200);
  digitalWrite(beep, LOW);                  // Beeper off
  drawCANCEL();               // draw cancel page
  if (SDflag)               // If SD card is available, LOG the CALL
    CallLog();
}
void dualPendCall()
{
  digitalWrite(cancelOut, HIGH);            // K2 Zone (Cancel) - latched
  digitalWrite(beep, HIGH);                 // Beeper on
  CancelFlag = true;
  delay(200);                               // 0.5sec wait
  digitalWrite(beep, LOW);                  // Beeper off
  delay(750);                               // 0.75sec wait
  drawCANCEL();               // draw cancel page
  PENDANTCALL = true;
  if (SDflag)               // If SD card is available, LOG the CALL
    CallLog();
}

/*******************************CANCEL PROFILE OPERATION*************************/
void Cancel()
{

  if (wrongpassFlag && SETPASS)
  {
    drawCANCEL();
    wrongpassFlag = false;
  }

  switch (EEPROM.read(71))
  {

    case true:
      {
        myGLCD.setFont(INDIGO);
        myGLCD.print("CANCEL", CENTER, 90);
        delay(500);
        myGLCD.print("@@@@@@", CENTER, 90);   //@ = space
        break;
      }
    case false:
      {
        myGLCD.setFont(INDIGO);
        myGLCD.print("CANCEL", CENTER, 120);
        delay(500);
        myGLCD.print("@@@@@@", CENTER, 120);   //@ = space
        break;
      }
  }
  delay(500);
  myGLCD.setColor(0, 0, 0);
  if (myTouch.dataAvailable())
  {
    myGLCD.setFont(BigFont);
    myTouch.read();
    x = myTouch.getX();
    y = myTouch.getY();

    while ( myTouch.dataAvailable() && (CancelFlag == true) && (x >= 0) && (x <= 320) && (y >= 0) && (y <= 240)) // CANCEL Button pressed
    {
      if (SETPASS)
      {
        PassWord();
        if (PassWordFlag)
        {
          switch (mode)
          {
            case 1:
              {
                digitalWrite(cancelOut, HIGH);
                CancelFlag = false;
                digitalWrite(callOut, LOW);
                delay(750);
                digitalWrite(cancelOut, LOW);
                break;
              }
            case 2:
              {
                CancelFlag = false;
                digitalWrite(callOut, LOW);
                digitalWrite(cancelOut, LOW);
                break;
              }
            case 3:
              {
                CancelFlag = false;
                digitalWrite(callOut, LOW);
                digitalWrite(cancelOut, LOW);
                break;
              }
          }
          drawLayer();
          if (SDflag)
            CancelLog();
        }
      }
      else
      {
        switch (mode)
        {
          case 1:
            {
              digitalWrite(cancelOut, HIGH);
              CancelFlag = false;
              digitalWrite(callOut, LOW);
              delay(750);
              digitalWrite(cancelOut, LOW);
              break;
            }
          case 2:
            {
              CancelFlag = false;
              digitalWrite(callOut, LOW);
              digitalWrite(cancelOut, LOW);
              break;
            }
          case 3:
            {
              CancelFlag = false;
              digitalWrite(callOut, LOW);
              digitalWrite(cancelOut, LOW);
              break;
            }
        }
        drawLayer();
        if (SDflag)
          CancelLog();
      }
    }
  }
  if (poAStat) {
    digitalWrite(beep, HIGH);
    delay(500);
    digitalWrite(beep, LOW);
    delay(150);
  }

}
/******************************* Draw LAYER************************/
void drawLayer()
{
  if (EEPROM.read(71))
    myGLCD.InitLCD(LANDSCAPE);
  else if (!EEPROM.read(71))
    myGLCD.InitLCD(PORTRAIT);
  myGLCD.clrScr();
  myGLCD.setColor(0, 0, 0);
  myGLCD.setFont(INDIGO);

  switch (EEPROM.read(102))
  {
    /************CALL*****************/
    case 1:
      {
        myGLCD.fillScr(0, 255, 0);
        myGLCD.setBackColor(0, 255, 0);
        if (EEPROM.read(71))
          myGLCD.print("CALL", CENTER, 90);
        else if (!EEPROM.read(71))
          myGLCD.print("CALL", CENTER, 140);
        break;
      }
    /**************ASSISST************/
    case 2:
      {
        myGLCD.fillScr(255, 255, 0);
        myGLCD.setBackColor(255, 255, 0);
        myGLCD.setBackColor(255, 255, 0);
        if (EEPROM.read(71))
          myGLCD.print("ASSIST", CENTER, 90);
        else if (!EEPROM.read(71))
          myGLCD.print("ASSIST", CENTER, 140);
        break;
      }
    /*************EMERGENCY*************/
    case 3:
      {

        myGLCD.fillScr(255, 0, 0);
        myGLCD.setBackColor(255, 0, 0);
        if (EEPROM.read(71))
        {
          myGLCD.setFont(INDIGO);
          myGLCD.print("EMERGENCY", CENTER, 90);
        }
        else if (!EEPROM.read(71))
        {
          myGLCD.setFont(indigo);
          myGLCD.print("EMERGENCY", CENTER, 140);
        }
        break;
      }
    default:
      {
        myGLCD.fillScr(0, 255, 0);
        myGLCD.setBackColor(0, 255, 0);
        if (EEPROM.read(71))
          myGLCD.print("CALL", CENTER, 80);
        else if (!EEPROM.read(71))
          myGLCD.print("CALL", CENTER, 120);
        break;
      }
  }
  myGLCD.setColor(0, 0, 0);
  myGLCD.setFont(BigFont);
  myGLCD.print("ROOM ", 5, 30);
  myGLCD.print(ROOMNO, 80, 30);
  myGLCD.print(stName, 5, 10);

}
/**********************************Cancel*****************************/
void drawCANCEL ()
{
  myGLCD.clrScr();
  myGLCD.setFont(INDIGO);
  myGLCD.fillScr(255, 255, 255);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(255, 255, 255);
  if (EEPROM.read(71))

    myGLCD.print("CANCEL", CENTER, 90);
  else if (!EEPROM.read(71))
    myGLCD.print("CANCEL", CENTER, 120);

  ShowTime();
}

/**************************************TIME AND DATE PAGE********************************/
void drawUpButton(int x, int y)
{
  myGLCD.setColor(64, 64, 128);
  myGLCD.fillRoundRect(x, y, x + 32, y + 25);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(x, y, x + 32, y + 25);
  myGLCD.setColor(128, 128, 255);
  for (int i = 0; i < 15; i++)
    myGLCD.drawLine(x + 6 + (i / 1.5), y + 20 - i, x + 27 - (i / 1.5), y + 20 - i);
}

void buttonWait(int x, int y)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect(x, y, x + 32, y + 25);
  while (myTouch.dataAvailable() == true)
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(x, y, x + 32, y + 25);
}

void drawDownButton(int x, int y)
{
  myGLCD.setColor(64, 64, 128);
  myGLCD.fillRoundRect(x, y, x + 32, y + 25);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect(x, y, x + 32, y + 25);
  myGLCD.setColor(128, 128, 255);
  for (int i = 0; i < 15; i++)
    myGLCD.drawLine(x + 6 + (i / 1.5), y + 5 + i, x + 27 - (i / 1.5), y + 5 + i);
}

/***********************************TIME PAD *********************************/
void TimePad()
{
  myGLCD.clrScr();

  ////////////////////////////LANDSCAPE///////////////////////////////////////
  if (EEPROM.read(71))
  {
    myGLCD.InitLCD(LANDSCAPE);
    myGLCD.setFont(BigFont);
    // Draw Save button
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillRoundRect(165, 200, 319, 239);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect(165, 200, 319, 239);
    myGLCD.setBackColor(0, 255, 0);
    myGLCD.print("Save", 210, 212);
    myGLCD.setBackColor(0, 0, 0);

    // Draw Cancel button
    myGLCD.setColor(255, 0, 0);
    myGLCD.fillRoundRect(0, 200, 154, 239);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect(0, 200, 154, 239);
    myGLCD.setBackColor(255, 0, 0);
    myGLCD.print("Cancel", 29, 212);

    myGLCD.setColor(64, 64, 128);
    myGLCD.setBackColor(0, 0, 0);
    drawUpButton(122, 10);
    drawUpButton(170, 10);
    drawUpButton(218, 10);
    drawDownButton(122, 61);
    drawDownButton(170, 61);
    drawDownButton(218, 61);
    drawUpButton(122, 110);
    drawUpButton(170, 110);
    drawUpButton(234, 110);
    drawDownButton(122, 161);
    drawDownButton(170, 161);
    drawDownButton(234, 161);

    // Draw frames
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect(0, 0, 319, 96);
    myGLCD.drawRoundRect(0, 100, 319, 196);
    myGLCD.print("Time:", 10, 40);
    myGLCD.print(":", 154, 40);
    myGLCD.print(":", 202, 40);
    myGLCD.print("Date:", 10, 140);
    myGLCD.print("/", 154, 140);
    myGLCD.print("/", 202, 140);

    if (!CLOCKFLAG)
    {
      digitalWrite(callOut, HIGH);
      delay(500);
      digitalWrite(callOut, LOW);
    }

    while (true)
    {
      if (myTouch.dataAvailable())
      {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();
        if ((y >= 10) && (y <= 35)) // Buttons: Time UP
        {
          if ((x >= 122) && (x <= 154))
          {
            buttonWait(122, 10);
            HOUR += 1 ;
            if (HOUR == 24)
              HOUR = 0;
            if (HOUR < 10)
            {
              myGLCD.printNumI(0, 122, 40);
              myGLCD.printNumI(HOUR, 138, 40);
            }
            else
            {
              myGLCD.printNumI(HOUR, 122, 40);
            }
          }
          if ((x >= 170) && (x <= 202))
          {
            buttonWait(170, 10);
            MIN += 1;
            if (MIN == 60)
              MIN = 0;
            if (MIN < 10)
            {
              myGLCD.printNumI(0, 170, 40);
              myGLCD.printNumI(MIN, 186, 40);
            }
            else
            {
              myGLCD.printNumI(MIN, 170, 40);
            }
          }
          if ((x >= 218) && (x <= 250))
          {
            buttonWait(218, 10);
            SEC += 1;
            if (SEC == 60)
              SEC = 0;
            if (SEC < 10)
            {
              myGLCD.printNumI(0, 218, 40);
              myGLCD.printNumI(SEC, 234, 40);
            }
            else
            {
              myGLCD.printNumI(SEC, 218, 40);
            }
          }
        }
        else if ((y >= 61) && (y <= 86)) // Buttons: Time DOWN
        {
          if ((x >= 122) && (x <= 154))
          {
            buttonWait(122, 61);
            HOUR -= 1;
            if (HOUR < 0)
              HOUR = 23;
            if (HOUR < 10)
            {
              myGLCD.printNumI(0, 122, 40);
              myGLCD.printNumI(HOUR, 138, 40);
            }
            else
            {
              myGLCD.printNumI(HOUR, 122, 40);
            }
          }
          else if ((x >= 170) && (x <= 202))
          {
            buttonWait(170, 61);
            MIN -= 1;
            if (MIN < 0)
              MIN = 59;
            if (MIN < 10)
            {
              myGLCD.printNumI(0, 170, 40);
              myGLCD.printNumI(MIN, 186, 40);
            }
            else
            {
              myGLCD.printNumI(MIN, 170, 40);
            }
          }
          else if ((x >= 218) && (x <= 250))
          {
            buttonWait(218, 61);
            SEC -= 1;
            if (SEC < 0)
              SEC = 59;
            if (SEC < 10)
            {
              myGLCD.printNumI(0, 218, 40);
              myGLCD.printNumI(SEC, 234, 40);
            }
            else
            {
              myGLCD.printNumI(SEC, 218, 40);
            }
          }
        }
        if ((y >= 110) && (y <= 135)) // Buttons: Date UP
        {
          if ((x >= 122) && (x <= 154))
          {
            buttonWait(122, 110);
            DAY += 1;
            if ((MONTH == 1) or (MONTH == 3) or (MONTH == 5) or (MONTH == 7) or (MONTH == 8) or (MONTH == 9) or (MONTH == 12))
            {
              if (DAY > 31)
                DAY = 1;
            }
            else
            {
              if ( DAY > 30)
                DAY = 1;
            }
            if (DAY < 10)
            {
              myGLCD.printNumI(0, 122, 140);
              myGLCD.printNumI(DAY, 138, 140);
            }
            else
            {
              myGLCD.printNumI(DAY, 122, 140);
            }
          }
          else if ((x >= 170) && (x <= 202))
          {
            buttonWait(170, 110);
            MONTH += 1;
            if (MONTH == 13)
              MONTH = 1;
            if (MONTH < 10)
            {
              myGLCD.printNumI(0, 170, 140);
              myGLCD.printNumI(MONTH, 186, 140);
            }
            else
            {
              myGLCD.printNumI(MONTH, 170, 140);
            }
          }
          else if ((x >= 234) && (x <= 266))
          {
            buttonWait(234, 110);
            YEAR += 1;
            if (YEAR > 99)
              YEAR = 0;
            myGLCD.printNumI(20, 218, 140);
            if (YEAR <= 9)
            {
              myGLCD.printNumI(0, 250, 140);
              myGLCD.printNumI(YEAR, 266, 140);
            }
            else
              myGLCD.printNumI(YEAR, 250, 140);
          }
        }
        else if ((y >= 161) && (y <= 186)) // Buttons: Date DOWN
        {
          if ((x >= 122) && (x <= 154))
          {
            buttonWait(122, 161);
            DAY -= 1;
            if (DAY < 1)
              DAY = 31;
            if (DAY < 10)
            {
              myGLCD.printNumI(0, 122, 140);
              myGLCD.printNumI(DAY, 138, 140);
            }
            else
            {
              myGLCD.printNumI(DAY, 122, 140);
            }

          }
          else if ((x >= 170) && (x <= 202))
          {
            buttonWait(170, 161);
            MONTH -= 1;
            if (MONTH < 0)
              MONTH = 12;
            if (MONTH < 10)
            {
              myGLCD.printNumI(0, 170, 140);
              myGLCD.printNumI(MONTH, 186, 140);
            }
            else
            {
              myGLCD.printNumI(MONTH, 170, 140);
            }
          }
          else if ((x >= 234) && (x <= 266))
          {
            buttonWait(234, 161);
            YEAR -= 1;
            if (YEAR < 00)
              YEAR = 99;
            myGLCD.printNumI(20, 218, 140);
            if (YEAR <= 9)
            {
              myGLCD.printNumI(0, 250, 140);
              myGLCD.printNumI(YEAR, 266, 140);
            }
            else
              myGLCD.printNumI(YEAR, 250, 140);
          }
        }
        else if ((y >= 200) && (y <= 239)) // Buttons: CANCEL / SAVE
        {
          if ((x >= 165) && (x <= 319))
          {
            Hour = HOUR;
            Min = MIN;
            Sec = SEC;
            Day = DAY;
            Month = MONTH;
            Year = 2000 + YEAR;
            rtc.setTime(Hour, Min, SEC);     // Set the time  to 12:00:00 (24hr format)
            rtc.setDate(Day, Month, Year);   // Set the date

            CLOCKFLAG = true;
            myGLCD.setColor (255, 0, 0);
            myGLCD.drawRoundRect(165, 200, 319, 239);
            return;
          }
          else if ((x >= 0) && (x <= 154))
          {
            rtc.getTimeStr();
            rtc.getDateStr();
            myGLCD.setColor (255, 0, 0);
            myGLCD.drawRoundRect(0, 200, 154, 239);
            return;
          }
        }
      }
    }
  }
  ////////////////////////////PORTRAIT///////////////////////////////////////
  else if (!EEPROM.read(71))
  {
    myGLCD.InitLCD(PORTRAIT);
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillRoundRect(129, 310, 239, 270);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect(129, 310, 239, 270);
    myGLCD.setBackColor(0, 255, 0);
    myGLCD.print("SAVE", 150, 280);


    // Draw Cancel button
    myGLCD.setColor(255, 0, 0);
    myGLCD.fillRoundRect(5, 310, 115, 270);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect(5, 310, 115, 270);
    myGLCD.setBackColor(255, 0, 0);
    myGLCD.print("CANCEL", 10, 280);

    myGLCD.setBackColor(0, 0, 0);
    drawUpButton(90, 10);
    drawUpButton(145, 10);
    drawUpButton(200, 10);
    drawDownButton(90, 61);
    drawDownButton(145, 61);
    drawDownButton(200, 61);
    drawUpButton(90, 110);
    drawUpButton(145, 110);
    drawUpButton(200, 110);
    drawDownButton(90, 161);
    drawDownButton(145, 161);
    drawDownButton(200, 161);

    // Draw frames
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect(0, 0, 239, 96);
    myGLCD.drawRoundRect(0, 100, 239, 196);
    myGLCD.print("Time:", 10, 40);
    myGLCD.print(":", 130, 40);
    myGLCD.print(":", 185, 40);
    myGLCD.print("Date:", 10, 140);
    myGLCD.print("/", 125, 140);
    myGLCD.print("/", 180, 140);


    while (true)
    {
      if (myTouch.dataAvailable())
      {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();
        if ((x >= 285) && (x <= 315)) // Buttons: Time UP
        {
          if ((y >= 90) && (y <= 125))
          {
            buttonWait(90, 10);
            HOUR += 1 ;
            if (HOUR == 24)
              HOUR = 0;
            if (HOUR < 10)
            {
              myGLCD.printNumI(0, 90, 40);
              myGLCD.printNumI(HOUR, 106, 40);
            }
            else
            {
              myGLCD.printNumI(HOUR, 90, 40);
            }
          }
          if ((y >= 145) && (y <= 180))
          {
            buttonWait(145, 10);
            MIN += 1;
            if (MIN == 60)
              MIN = 0;
            if (MIN < 10)
            {
              myGLCD.printNumI(0, 145, 40);
              myGLCD.printNumI(MIN, 161, 40);
            }
            else
            {
              myGLCD.printNumI(MIN, 145, 40);
            }
          }
          if ((y >= 200) && (y <= 235))
          {
            buttonWait(200, 10);
            SEC += 1;
            if (SEC == 60)
              SEC = 0;
            if (SEC < 10)
            {
              myGLCD.printNumI(0, 200, 40);
              myGLCD.printNumI(SEC, 218, 40);
            }
            else
            {
              myGLCD.printNumI(SEC, 200, 40);
            }
          }
        }
        if ((x >= 230) && (x <= 270)) // Buttons: Time DOWN
        {
          if ((y >= 90) && (y <= 125))
          {
            buttonWait(90, 61);
            HOUR -= 1;
            if (HOUR < 0)
              HOUR = 23;
            if (HOUR < 10)
            {
              myGLCD.printNumI(0, 90, 40);
              myGLCD.printNumI(HOUR, 108, 40);
            }
            else
            {
              myGLCD.printNumI(HOUR, 90, 40);
            }
          }
          if ((y >= 145) && (y <= 180))
          {
            buttonWait(145, 61);
            MIN -= 1;
            if (MIN < 0)
              MIN = 59;
            if (MIN < 10)
            {
              myGLCD.printNumI(0, 145, 40);
              myGLCD.printNumI(MIN, 163, 40);
            }
            else
            {
              myGLCD.printNumI(MIN, 145, 40);
            }
          }
          if ((y >= 200) && (y <= 235))
          {
            buttonWait(200, 61);
            SEC -= 1;
            if (SEC < 0)
              SEC = 59;
            if (SEC < 10)
            {
              myGLCD.printNumI(0, 200, 40);
              myGLCD.printNumI(SEC, 218, 40);
            }
            else
            {
              myGLCD.printNumI(SEC, 200, 40);
            }
          }
        }
        if ((x >= 185) && (x <= 215)) // Buttons: Date UP
        {
          if ((y >= 90) && (y <= 125))
          {
            buttonWait(90, 110);
            DAY += 1;
            if ((MONTH == 1) or (MONTH == 3) or (MONTH == 5) or (MONTH == 7) or (MONTH == 8) or (MONTH == 9) or (MONTH == 12))
            {
              if (DAY > 31)
                DAY = 1;
            }
            else
            {
              if ( DAY > 30)
                DAY = 1;
            }
            if (DAY < 10)
            {
              myGLCD.printNumI(0, 90, 140);
              myGLCD.printNumI(DAY, 108, 140);
            }
            else
            {
              myGLCD.printNumI(DAY, 90, 140);
            }
          }
          if ((y >= 145) && (y <= 180))
          {
            buttonWait(145, 110);
            MONTH += 1;
            if (MONTH == 13)
              MONTH = 1;
            if (MONTH < 10)
            {
              myGLCD.printNumI(0, 145, 140);
              myGLCD.printNumI(MONTH, 163, 140);
            }
            else
            {
              myGLCD.printNumI(MONTH, 145, 140);
            }
          }
          if ((y >= 200) && (y <= 235))
          {
            buttonWait(200, 110);
            YEAR += 1;
            if (YEAR > 99)
              YEAR = 0;
            // myGLCD.printNumI(20, 200, 140);
            if (YEAR <= 9)
            {
              myGLCD.printNumI(0, 200, 140);
              myGLCD.printNumI(YEAR, 218, 140);
            }
            else
              myGLCD.printNumI(YEAR, 200, 140);
          }
        }
        if ((x >= 135) && (x <= 165)) // Buttons: Date DOWN
        {
          if ((y >= 90) && (y <= 125))
          {
            buttonWait(90, 161);
            DAY -= 1;
            if (DAY < 1)
              DAY = 31;
            if (DAY < 10)
            {
              myGLCD.printNumI(0, 90, 140);
              myGLCD.printNumI(DAY, 108, 140);
            }
            else
            {
              myGLCD.printNumI(DAY, 90, 140);
            }

          }
          else if ((y >= 145) && (y <= 180))
          {
            buttonWait(145, 161);
            MONTH -= 1;
            if (MONTH < 0)
              MONTH = 12;
            if (MONTH < 10)
            {
              myGLCD.printNumI(0, 145, 140);
              myGLCD.printNumI(MONTH, 163, 140);
            }
            else
            {
              myGLCD.printNumI(MONTH, 145, 140);
            }
          }
          if ((y >= 200) && (y <= 235))
          {
            buttonWait(200, 161);
            YEAR -= 1;
            if (YEAR < 00)
              YEAR = 99;
            // myGLCD.printNumI(20, 200, 140);
            if (YEAR <= 9)
            {
              myGLCD.printNumI(0, 200, 140);
              myGLCD.printNumI(YEAR, 218, 140);
            }
            else
              myGLCD.printNumI(YEAR, 200, 140);
          }
        }
        if ((x >= 10) && (x <= 50)) // Buttons: SAVE
        {
          if ((y >= 129) && (y <= 239))
          {
            waitForIt(129, 310, 239, 270);
            Hour = HOUR;
            Min = MIN;
            Sec = SEC;
            Day = DAY;
            Month = MONTH;
            Year = 2000 + YEAR;
            rtc.setTime(Hour, Min, SEC);     // Set the time  to 12:00:00 (24hr format)
            rtc.setDate(Day, Month, Year);   // Set the date
            CLOCKFLAG = true;
            myGLCD.setColor (255, 0, 0);
            return;
          }
          if ((y >= 5) && (y <= 115))   //CANCEL
          {
            waitForIt(5, 310, 115, 270);
            rtc.getTimeStr();
            rtc.getDateStr();
            myGLCD.setColor (255, 0, 0);
            return;
          }
        }
      }
    }
  }
}

ISR(TIMER5_COMPA_vect)          // timer compare interrupt service routine
{
  Sec ++;
  if (Sec > 59)
  {
    Sec = 0;
    Min ++;
  }
  if (Min > 59)
  {
    Min = 0 ;
    Hour ++;
  }
  if (Hour > 23)
    Hour = 0;
  if ((Hour == 23) && (Min == 59) && (Sec == 59))
    DayFlag = true;
}

/************************SHOW TIME******************/
void ShowTime()
{
  myGLCD.setFont(BigFont);
  if (EEPROM.read(71))
  {
    myGLCD.print(rtc.getTimeStr(FORMAT_SHORT), LEFT, 215);
    myGLCD.print(rtc.getDateStr(), RIGHT, 215);
  }
  ////////////////////////////////////////////////////////////PORTRAIT TIME////////////////////////////
  else if (!EEPROM.read(71))
  {
    myGLCD.print(rtc.getTimeStr(FORMAT_SHORT), LEFT, 270);
    myGLCD.print(rtc.getDateStr(), LEFT, 300);
  }
}

/**************************************DRWA NUM/KEY PAD*********************************/
void NumPad()
{
  int  timeout ; // variable for suspention function
NUMPAD_LAND:
  myGLCD.clrScr();
  /////////////////////////////////////////////LANDSCAPE KEYPAD//////////////////////////////
  if (EEPROM.read(71))
  {
    // Draw the upper row of buttons
    myGLCD.setFont(BigFont);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.fillScr(255, 255, 255);

    for (x = 0; x < 5; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10 + (x * 60), 10, 60 + (x * 60), 60);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (10 + (x * 60), 10, 60 + (x * 60), 60);
      myGLCD.setColor(255, 255, 255);
      myGLCD.printNumI(x + 1, 27 + (x * 60), 27);
    }
    // Draw the center row of buttons

    for (x = 0; x < 5; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10 + (x * 60), 70, 60 + (x * 60), 120);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (10 + (x * 60), 70, 60 + (x * 60), 120);
      myGLCD.setColor(255, 255, 255);
      if (x < 4)
        myGLCD.printNumI(x + 6, 27 + (x * 60), 87);
    }
    myGLCD.print("0", 267, 87);
    // Draw the lower row of buttons
    myGLCD.setColor(255, 0, 0);
    myGLCD.fillRoundRect (10, 130, 100, 160);
    myGLCD.setColor(255, 255, 0);
    myGLCD.fillRoundRect (110, 130, 200, 160);
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillRoundRect (210, 130, 300, 160);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (10, 130, 100, 160);
    myGLCD.drawRoundRect (110, 130, 200, 160);
    myGLCD.drawRoundRect (210, 130, 300, 160);
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(255, 0, 0);
    myGLCD.print("RST", 30, 140);
    myGLCD.setBackColor(255, 255, 0);
    myGLCD.print("A/Z", 130, 140);
    myGLCD.setBackColor(0, 255, 0);
    myGLCD.print("ENT", 230, 140);
    if (stCurrentLen >= 0)
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.setBackColor(255, 255, 255);
      myGLCD.print(stCurrent, LEFT, 224);
    }
    while (true)
    {
      if (!myTouch.dataAvailable())
      {
        timeout = millis();
        if (timeout == 10000)
        {
          timeoutFlag = true;
          timeout = 0;
          return;
        }
      }
      else
      {
        timeout = 0;
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();

        /******* Detecting Numbers  ***************/
        if ((y >= 10) && (y <= 60)) // Upper row
        {
          if ((x >= 10) && (x <= 60)) // Button: 1
          {
            waitForIt(10, 10, 60, 60);
            updateStr('1');
          }
          if ((x >= 70) && (x <= 120)) // Button: 2
          {
            waitForIt(70, 10, 120, 60);
            updateStr('2');
          }
          if ((x >= 130) && (x <= 180)) // Button: 3
          {
            waitForIt(130, 10, 180, 60);
            updateStr('3');
          }
          if ((x >= 190) && (x <= 240)) // Button: 4
          {
            waitForIt(190, 10, 240, 60);
            updateStr('4');
          }
          if ((x >= 250) && (x <= 300)) // Button: 5
          {
            waitForIt(250, 10, 300, 60);
            updateStr('5');
          }
        }

        if ((y >= 70) && (y <= 120)) // Center row
        {
          if ((x >= 10) && (x <= 60)) // Button: 6
          {
            waitForIt(10, 70, 60, 120);
            updateStr('6');
          }
          if ((x >= 70) && (x <= 120)) // Button: 7
          {
            waitForIt(70, 70, 120, 120);
            updateStr('7');
          }
          if ((x >= 130) && (x <= 180)) // Button: 8
          {
            waitForIt(130, 70, 180, 120);
            updateStr('8');
          }
          if ((x >= 190) && (x <= 240)) // Button: 9
          {
            waitForIt(190, 70, 240, 120);
            updateStr('9');
          }
          if ((x >= 250) && (x <= 300)) // Button: 0
          {
            waitForIt(250, 70, 300, 120);
            updateStr('0');
          }
        }
        /**********A/Z and Reset and Enter Buttons Function ************/

        if ((y >= 130) && (y <= 160)) // Upper row
        {
          if ((x >= 10) && (x <= 100)) // Button: Reset
          {
            waitForIt(10, 130, 100, 160);
            stCurrent[0] = '\0';
            stCurrentLen = 0;
            stLast[0] = '\0';
            myGLCD.setColor(255, 255, 255);
            myGLCD.fillRect(0, 224, 319, 239);
          }
          if ((x >= 110) && (x <= 200)) // Button: A/Z
          {
            waitForIt(110, 130, 200, 160);
            goto KEYPAD_LAND;
          }
          if ((x >= 210) && (x <= 300)) // Button: Enter
          {
            waitForIt(210, 130, 300, 160);
            if (PASSFLAG)
            {
              if (stCurrentLen >= 0)
              {
                for (x = 0; x < stCurrentLen + 1; x++)
                {
                  if (SetFlag)
                  {
                    for (addr = 50; addr < stCurrentLen + 51; addr++)
                      EEPROM.write(addr, stCurrent[addr - 50]);
                  }
                  else
                  {
                    stLast[x] = stCurrent[x];
                    //    myGLCD.print(stLastPass, LEFT, 208);      //uncoment if want to see Current input
                  }
                }

                for (addr = 50; addr < stCurrentLen + 51; addr++)
                  master[addr - 50] = EEPROM.read(addr);
                stCurrent[0] = '\0';
                stCurrentLen = 0;
                if (SetFlag)
                {
                  myGLCD.fillScr (255, 255, 255);
                  myGLCD.setColor(0, 255, 0);
                  myGLCD.fillRoundRect(20, 170, 120, 230);
                  myGLCD.setColor(255, 0, 0);
                  myGLCD.fillRoundRect(180, 170, 280, 230);
                  myGLCD.setBackColor(255, 255, 255);
                  myGLCD.setColor(0, 0, 0);
                  myGLCD.setFont(BigFont);
                  myGLCD.print("DO YOU WISH TO", CENTER, 20);
                  myGLCD.print("SET THIS", CENTER, 50);
                  myGLCD.print("PASSWORD FOR", CENTER, 80);
                  myGLCD.print("CANCEL?", CENTER, 110);
                  myGLCD.setBackColor(0, 255, 0);
                  myGLCD.print("YES", 45, 190);
                  myGLCD.setBackColor(255, 0, 0);
                  myGLCD.print("NO", 215, 190);
                  myGLCD.drawRoundRect (20, 170, 120, 230); // YES button
                  myGLCD.drawRoundRect (180, 170, 280, 230); // NO Button

                  while (true)
                  {
                    if (myTouch.dataAvailable())
                    {
                      myTouch.read();
                      int      x = myTouch.getX();
                      int      y = myTouch.getY();
                      if ((y >= 170) && (y <= 230))
                      {
                        if ((x >= 20) && (x <= 120))
                        {
                          waitForIt(20, 170, 120, 230);
                          EEPROM.write(100 , 1);
                          SETPASS = EEPROM.read(100);
                          break;
                        }
                        if ((x >= 180) && (x <= 280)) //Presiing NO button
                        {
                          waitForIt(180, 170, 280, 230);
                          EEPROM.write(100 , 0);
                          SETPASS = EEPROM.read(100);
                          break;
                        }
                      }
                    }
                  }
                }
              }

            }

            if (ROOMFLAG)
            {
              if (stCurrentLen >= 0)
              {
                for (x = 0; x < stCurrentLen + 1; x++)
                {
                  for (addr = 80; addr < stCurrentLen + 81; addr++)
                  {
                    EEPROM.write(addr, stCurrent[addr - 80]);
                    ROOMNO[addr - 80] = stCurrent[addr - 80];
                  }
                }
                stCurrent[0] = '\0';
                stCurrentLen = 0;
              }
            }
            if (NAMEFLAG)
            {
              if (stCurrentLen >= 0)
              {
                for (x = 0; x < stCurrentLen + 1; x++)
                {
                  for (addr = 0; addr < stCurrentLen + 1; addr++)
                  {
                    EEPROM.write(addr, stCurrent[addr]);
                    stName[addr] = stCurrent[addr];
                  }

                }
                stCurrent[0] = '\0';
                stCurrentLen = 0;
              }
            }

            return;
          }

        }
      }
    }
KEYPAD_LAND:
    myGLCD.setFont(BigFont);
    myGLCD.clrScr();
    myGLCD.fillScr(255, 255, 255);

    // Draw the first row of letters
    for (x = 0; x < 7; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10 + (x * 40), 10, 40 + (x * 40), 40);
      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect (10 + (x * 40), 10, 40 + (x * 40), 40);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.print("A", 18, 18);
      myGLCD.print("B", 59, 18);
      myGLCD.print("C", 99, 18);
      myGLCD.print("D", 139, 18);
      myGLCD.print("E", 179, 18);
      myGLCD.print("F", 219, 18);
      myGLCD.print("G", 259, 18);
    }
    // Draw the second row of letters
    for (x = 0; x < 7; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10 + (x * 40), 50, 40 + (x * 40), 80);
      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect (10 + (x * 40), 50, 40 + (x * 40), 80);
      myGLCD.print("H", 19, 58);
      myGLCD.print("I", 59, 58);
      myGLCD.print("J", 99, 58);
      myGLCD.print("K", 139, 58);
      myGLCD.print("L", 179, 58);
      myGLCD.print("M", 219, 58);
      myGLCD.print("N", 259, 58);
    }
    // Draw the thirth row of letters
    for (x = 0; x < 7; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10 + (x * 40), 90, 40 + (x * 40), 120);
      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect (10 + (x * 40), 90, 40 + (x * 40), 120);
      myGLCD.print("O", 17, 98);
      myGLCD.print("P", 57, 98);
      myGLCD.print("Q", 97, 98);
      myGLCD.print("R", 137, 98);
      myGLCD.print("S", 177, 98);
      myGLCD.print("T", 217, 98);
      myGLCD.print("U", 257, 98);
    }
    // Draw the fourth row of letters
    for (x = 0; x < 7; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10 + (x * 40), 130, 40 + (x * 40), 160);
      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect (10 + (x * 40), 130, 40 + (x * 40), 160);
      myGLCD.print("V", 17, 138);
      myGLCD.print("W", 57, 138);
      myGLCD.print("X", 97, 138);
      myGLCD.print("Y", 137, 138);
      myGLCD.print("Z", 177, 138);
      myGLCD.print(".", 260, 135);
    }
    // Draw the Reset and Enter buttons
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (10, 170, 140, 200);
    myGLCD.setColor(255, 255, 0);
    myGLCD.fillRoundRect (150, 170, 280, 200);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (10, 170, 140, 200);
    myGLCD.drawRoundRect (150, 170, 280, 200);
    myGLCD.setColor(255, 255, 255);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.print("SPC", 50, 177);
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(255, 255, 0);
    myGLCD.print("123", 190, 177);

    if (stCurrentLen >= 0)
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.setBackColor(255, 255, 255);
      myGLCD.print(stCurrent, LEFT, 224);
    }
    while (true)
    {
      if (myTouch.dataAvailable())
      {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();
        /******* Detecting Letters  ***************/
        if ((y >= 10) && (y <= 40)) // FIRST row
        {
          if ((x >= 10) && (x <= 40)) // Button: A
          {
            waitForIt(10, 10, 40, 40);
            updateStr('A');
          }
          if ((x >= 50) && (x <= 80)) // Button: B
          {
            waitForIt(50, 10, 80, 40);
            updateStr('B');
          }
          if ((x >= 90) && (x <= 120)) // Button: C
          {
            waitForIt(90, 10, 120, 40);
            updateStr('C');
          }
          if ((x >= 130) && (x <= 160)) // Button: D
          {
            waitForIt(130, 10, 160, 40);
            updateStr('D');
          }
          if ((x >= 170) && (x <= 200)) // Button: E
          {
            waitForIt(170, 10, 200, 40);
            updateStr('E');
          }
          if ((x >= 210) && (x <= 240)) // Button: F
          {
            waitForIt(210, 10, 240, 40);
            updateStr('F');
          }
          if ((x >= 250) && (x <= 280)) // Button: G
          {
            waitForIt(250, 10, 280, 40);
            updateStr('G');
          }
        }

        if ((y >= 50) && (y <= 80)) // SECOND row
        {
          if ((x >= 10) && (x <= 40)) // Button: H
          {
            waitForIt(10, 50, 40, 80);
            updateStr('H');
          }
          if ((x >= 50) && (x <= 80)) // Button: I
          {
            waitForIt(50, 50, 80, 80);
            updateStr('I');
          }
          if ((x >= 90) && (x <= 120)) // Button: J
          {
            waitForIt(90, 50, 120, 80);
            updateStr('J');
          }
          if ((x >= 130) && (x <= 160)) // Button: K
          {
            waitForIt(130, 50, 160, 80);
            updateStr('K');
          }
          if ((x >= 170) && (x <= 200)) // Button: L
          {
            waitForIt(170, 50, 200, 80);
            updateStr('L');
          }
          if ((x >= 210) && (x <= 240)) // Button: M
          {
            waitForIt(210, 50, 240, 80);
            updateStr('M');
          }
          if ((x >= 250) && (x <= 280)) // Button: N
          {
            waitForIt(250, 50, 280, 80);
            updateStr('N');
          }
        }
        if ((y >= 90) && (y <= 120)) // THIRTH row
        {
          if ((x >= 10) && (x <= 40)) // Button: O
          {
            waitForIt(10, 90, 40, 120);
            updateStr('O');
          }
          if ((x >= 50) && (x <= 80)) // Button: P
          {
            waitForIt(50, 90, 80, 120);
            updateStr('P');
          }
          if ((x >= 90) && (x <= 120)) // Button: Q
          {
            waitForIt(90, 90, 120, 120);
            updateStr('Q');
          }
          if ((x >= 130) && (x <= 160)) // Button: R
          {
            waitForIt(130, 90, 160, 120);
            updateStr('R');
          }
          if ((x >= 170) && (x <= 200)) // Button: S
          {
            waitForIt(170, 90, 200, 120);
            updateStr('S');
          }
          if ((x >= 210) && (x <= 240)) // Button: T
          {
            waitForIt(210, 90, 240, 120);
            updateStr('T');
          }
          if ((x >= 250) && (x <= 280)) // Button: U
          {
            waitForIt(250, 90, 280, 120);
            updateStr('U');
          }
        }

        if ((y >= 130) && (y <= 160)) // FOURTH row
        {
          if ((x >= 10) && (x <= 40)) // Button: V
          {
            waitForIt(10, 130, 40, 160);
            updateStr('V');
          }
          if ((x >= 50) && (x <= 80)) // Button: W
          {
            waitForIt(50, 130, 80, 160);
            updateStr('W');
          }
          if ((x >= 90) && (x <= 120)) // Button: X
          {
            waitForIt(90, 130, 120, 160);
            updateStr('X');
          }
          if ((x >= 130) && (x <= 160)) // Button: Y
          {
            waitForIt(130, 130, 160, 160);
            updateStr('Y');
          }
          if ((x >= 170) && (x <= 200)) // Button: Z
          {
            waitForIt(170, 130, 200, 160);
            updateStr('Z');
          }

          if ((x >= 250) && (x <= 280)) // Button: "."
          {
            waitForIt(250, 130, 280, 160);
            updateStr('.');
          }
        }
        if ((y >= 170) && (y <= 200)) // FIFTH row
        {
          if ((x >= 10) && (x <= 140)) // Button: SPACE
          {
            waitForIt(10, 170, 140, 200);
            updateStr(' ');
          }
          if ((x >= 150) && (x <= 280)) // Button: 123
          {
            waitForIt(150, 170, 280, 200);
            goto NUMPAD_LAND;
          }
        }
      }
    }
  }



  ///////////////////////////////////////////////PORTRAIT//////////////////////////////////
  if (!EEPROM.read(71))
  {
NUMPAD_PORT:

    myGLCD.setFont(BigFont);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.fillScr(255, 255, 255);
    for (x = 0; x < 4; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (9 + (x * 58), 9, 58 + (x * 58), 58);
      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect (9 + (x * 58), 9, 58 + (x * 58), 58);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.printNumI(x + 1, 25 + (x * 58), 25);
    }

    // Draw the center row of buttons
    for (x = 0; x < 4; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (9 + (x * 58), 70, 58 + (x * 58), 119);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (9 + (x * 58), 70, 58 + (x * 58), 119);
      myGLCD.setColor(255, 255, 255);
      if (x < 4)
        myGLCD.printNumI(x + 5, 25 + (x * 58), 85);
    }
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (9, 130, 58, 179);
    myGLCD.fillRoundRect (67, 130, 116, 179);
    myGLCD.setColor(255, 255, 0 );
    myGLCD.fillRoundRect (125, 130, 232, 179);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (9 , 130, 58, 179);
    myGLCD.drawRoundRect (67 , 130, 116, 179);
    myGLCD.drawRoundRect (125, 130, 232, 179);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("9", 25, 145);
    myGLCD.print("0", 83, 145);
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(255, 255, 0);
    myGLCD.print("A/Z", 152, 145);

    // Draw the lower row of buttons
    myGLCD.setColor(255, 0, 0);
    myGLCD.fillRoundRect (10, 189, 115, 238);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (10, 189, 115, 238);
    myGLCD.setBackColor (255, 0, 0);
    myGLCD.print("Reset", 20, 208);
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillRoundRect (125, 189, 230, 238);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (125, 189, 230, 238);
    myGLCD.setBackColor (0, 255, 0);
    myGLCD.print("Enter", 135, 208);
    myGLCD.setBackColor (0, 0, 0);

    if (stCurrentLen >= 0)
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.setBackColor(255, 255, 255);
      myGLCD.print(stCurrent, LEFT, 300);
    }
    while (true)
    {
      if (!myTouch.dataAvailable())
      {
        timeout = millis();

        if (timeout == 10000)
        {
          timeoutFlag = true;
          timeout = 0;
          return;
        }
      }
      else
      {
        timeout = 0;
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();
        Serial.begin(9600);
        Serial.print("x= ");
        Serial.println(x);
        Serial.print("y= ");
        Serial.println(y);
        /******* Detecting Numbers  ***************/
        if ((x >= 266) && (x <= 315)) // First row
        {
          if ((y >= 9) && (y <= 58)) // Button: 1
          {
            waitForIt(9, 9, 58, 58);
            updateStr('1');
          }
          if ((y >= 67) && (y <= 116)) // Button: 2
          {
            waitForIt(67, 9, 116, 58);
            updateStr('2');
          }
          if ((y >= 125) && (y <= 174)) // Button: 3
          {
            waitForIt(125, 9, 174, 58);
            updateStr('3');
          }
          if ((y >= 183) && (y <= 232)) // Button: 4
          {
            waitForIt(183, 9, 232, 58);
            updateStr('4');
          }
        }
        if ((x >= 208) && (x <= 257)) // Second row
        {
          if ((y >= 9) && (y <= 58)) // Button: 5
          {
            waitForIt(9, 70, 58, 119);
            updateStr('5');
          }
          if ((y >= 67) && (y <= 116)) // Button: 6
          {
            waitForIt(67, 70, 116, 119);
            updateStr('6');
          }
          if ((y >= 125) && (y <= 174)) // Button: 7
          {
            waitForIt(125, 70, 174, 119);
            updateStr('7');
          }
          if ((y >= 183) && (y <= 232)) // Button: 8
          {
            waitForIt(183, 70, 232, 119);
            updateStr('8');
          }
        }
        if ((x >= 150) && (x <= 199)) // Third row
        {
          if ((y >= 9) && (y <= 58)) // Button: 9
          {
            waitForIt(9, 130, 58, 179);
            updateStr('9');
          }
          if ((y >= 67) && (y <= 116)) // Button: 0
          {
            waitForIt(67, 130, 116, 179);
            updateStr('0');
          }
          if ((y >= 125) && (y <= 232)) // Button: A/Z
          {
            waitForIt(125, 130, 232, 179);
            goto KEYPAD_PORT;
          }
        }
        /**********Reset and Enter Buttons Function ************/
        if ((x >= 92) && (x <= 141)) // Fifth row
        {
          if ((y >= 10) && (y <= 115)) // Button: Reset
          {
            waitForIt(10, 189, 115, 238);
            stCurrent[0] = '\0';
            stCurrentLen = 0;
            stLast[0] = '\0';
            myGLCD.setColor(255, 255, 255);
            myGLCD.fillRect(239, 240, 0, 319);
          }
          if ((y >= 125) && (y <= 230)) // Button: Enter
          {
            waitForIt(125, 189, 230, 238);
            if (PASSFLAG)
            {
              if (stCurrentLen >= 0)
              {
                for (x = 0; x < stCurrentLen + 1; x++)
                {
                  if (SetFlag)
                  {
                    for (addr = 50; addr < stCurrentLen + 51; addr++)
                      EEPROM.write(addr, stCurrent[addr - 50]);
                  }
                  else
                  {
                    stLast[x] = stCurrent[x];
                    //    myGLCD.print(stLast, LEFT, 210);      //uncoment if want to see Current input
                  }
                }
                for (addr = 50; addr < stCurrentLen + 51; addr++)
                  master[addr - 50] = EEPROM.read(addr);
                stCurrent[0] = '\0';
                stCurrentLen = 0;
                if (SetFlag)
                {
                  myGLCD.fillScr(255, 255, 255);
                  myGLCD.setBackColor(255, 255, 255);
                  myGLCD.setColor(0, 0, 0);
                  myGLCD.print("DO YOU WISH TO", CENTER, 60);
                  myGLCD.print("SET THIS", CENTER, 90);
                  myGLCD.print("PASSWORD FOR", CENTER, 120);
                  myGLCD.print("CANCEL?", CENTER, 150);
                  myGLCD.setColor(0, 255, 0);
                  myGLCD.fillRoundRect(10, 230, 110, 290);
                  myGLCD.setColor(255, 0, 0);
                  myGLCD.fillRoundRect(130, 230, 230, 290);
                  myGLCD.setColor(0, 0, 0);
                  myGLCD.setBackColor(0, 255, 0);
                  myGLCD.print("YES", 35, 250);
                  myGLCD.setBackColor(255, 0, 0);
                  myGLCD.print("NO", 165, 250);
                  myGLCD.drawRoundRect (10, 230, 110, 290); // YES button
                  myGLCD.drawRoundRect (130, 230, 230, 290); // NO Button
                  while (true) {
                    if (myTouch.dataAvailable())
                    {
                      myTouch.read();
                      x = myTouch.getX();
                      y = myTouch.getY();
                      if ((x >= 30) && (x <= 90) )
                      {
                        if ( (y >= 10) && (y <= 110)) //Pressing YES button
                        {
                          waitForIt(10, 230, 110, 290);
                          SETPASS = true;
                          break;
                        }
                        if ((y >= 130) && (y <= 230))//Presiing NO button
                        {
                          waitForIt(130, 230, 230, 290);
                          SETPASS = false;
                          break;
                        }
                      }
                    }
                  }
                }
              }
            }
            if (ROOMFLAG)
            {
              if (stCurrentLen >= 0)
              {
                for (x = 0; x < stCurrentLen + 1; x++)
                {
                  for (addr = 80; addr < stCurrentLen + 81; addr++)
                  {
                    EEPROM.write(addr, stCurrent[addr - 80]);
                    ROOMNO[addr - 80] = stCurrent[addr - 80];
                  }
                }
                stCurrent[0] = '\0';
                stCurrentLen = 0;
              }
            }
            if (NAMEFLAG)
            {
              if (stCurrentLen >= 0)
              {
                for (x = 0; x < stCurrentLen + 1; x++)
                {
                  for (addr = 0; addr < stCurrentLen + 1; addr++)
                  {
                    EEPROM.write(addr, stCurrent[addr]);
                    stName[addr] = stCurrent[addr];
                  }

                }
                stCurrent[0] = '\0';
                stCurrentLen = 0;
              }
            }
            return;
          }
        }
      }
    }

KEYPAD_PORT:

    myGLCD.clrScr();
    myGLCD.fillScr(255, 255, 255);
    if (stCurrentLen >= 0)
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.setBackColor(255, 255, 255);
      myGLCD.print(stCurrent, LEFT, 300);
    }
    // Draw the first row of letters
    for (x = 0; x < 6; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5 + (x * 40), 10, 35 + (x * 40), 40);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5 + (x * 40), 10, 35 + (x * 40), 40);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.print("A", 12, 18);
      myGLCD.print("B", 52, 18);
      myGLCD.print("C", 92, 18);
      myGLCD.print("D", 132, 18);
      myGLCD.print("E", 172, 18);
      myGLCD.print("F", 212, 18);
    }
    // Draw the first row of letters
    for (x = 0; x < 6; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5 + (x * 40), 50, 35 + (x * 40), 80);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5 + (x * 40), 50, 35 + (x * 40), 80);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.print("G", 12, 58);
      myGLCD.print("H", 52, 58);
      myGLCD.print("I", 92, 58);
      myGLCD.print("J", 132, 58);
      myGLCD.print("K", 172, 58);
      myGLCD.print("L", 212, 58);
    }
    for (x = 0; x < 6; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5 + (x * 40), 90, 35 + (x * 40), 120);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5 + (x * 40), 90, 35 + (x * 40), 120);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.print("M", 12, 98);
      myGLCD.print("N", 52, 98);
      myGLCD.print("O", 92, 98);
      myGLCD.print("P", 132, 98);
      myGLCD.print("Q", 172, 98);
      myGLCD.print("R", 212, 98);
    }
    for (x = 0; x < 6; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5 + (x * 40), 130, 35 + (x * 40), 160);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5 + (x * 40), 130, 35 + (x * 40), 160);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.print("S", 12, 138);
      myGLCD.print("T", 52, 138);
      myGLCD.print("U", 92, 138);
      myGLCD.print("V", 132, 138);
      myGLCD.print("W", 172, 138);
      myGLCD.print("X", 212, 138);
    }
    for (x = 0; x < 3; x++)
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5 + (x * 40), 170, 35 + (x * 40), 200);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5 + (x * 40), 170, 35 + (x * 40), 200);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.print("Y", 12, 178);
      myGLCD.print("Z", 52, 178);
      myGLCD.print("*", 92, 178);

    }
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (125, 170, 235, 200);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (125, 170, 235, 200);
    myGLCD.setColor(255, 255, 255);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.setFont(BigFont);
    myGLCD.print("SPACE", 140, 180);
    myGLCD.setColor(255, 255, 0);
    myGLCD.fillRoundRect (5, 210, 235, 240);
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(255, 255, 0);
    myGLCD.drawRoundRect (5, 210, 235, 240);
    myGLCD.print("123", 100, 220);

    while (true)
    {
      if (myTouch.dataAvailable())
      {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();
        /******* Detecting Letters  ***************/
        if ((x >= 285) && (x <= 310)) // FIRST row
        {
          if ((y >= 5) && (y <= 35)) // Button: A
          {
            waitForIt(5, 10, 35, 40);
            updateStr('A');
          }
          if ((y >= 45) && (y <= 75)) // Button: B
          {
            waitForIt(45, 10, 75, 40);
            updateStr('B');
          }
          if ((y >= 85) && (y <= 115)) // Button: C
          {
            waitForIt(85, 10, 115, 40);
            updateStr('C');
          }
          if ((y >= 125) && (y <= 155)) // Button: D
          {
            waitForIt(125, 10, 155, 40);
            updateStr('D');
          }
          if ((y >= 165) && (y <= 195)) // Button: E
          {
            waitForIt(165, 10, 195, 40);
            updateStr('E');
          }
          if ((y >= 205) && (y <= 235)) // Button: F
          {
            waitForIt(205, 10, 235, 40);
            updateStr('F');
          }
        }

        if ((x >= 245) && (x <= 275))                             // SECOND row
        {
          if ((y >= 5) && (y <= 35)) // Button: G
          {
            waitForIt(5, 50, 35, 80);
            updateStr('G');
          }
          if ((y >= 45) && (y <= 75)) // Button: H
          {
            waitForIt(45, 50, 75, 80);
            updateStr('H');
          }
          if ((y >= 85) && (y <= 115)) // Button: I
          {
            waitForIt(85, 50, 115, 80);
            updateStr('I');
          }
          if ((y >= 125) && (y <= 155)) // Button: J
          {
            waitForIt(125, 50, 155, 80);
            updateStr('J');
          }
          if ((y >= 165) && (y <= 195)) // Button: K
          {
            waitForIt(165, 50, 195, 80);
            updateStr('K');
          }
          if ((y >= 205) && (y <= 235)) // Button: L
          {
            waitForIt(205, 50, 235, 80);
            updateStr('L');
          }
        }
        if ((x >= 205) && (x <= 235))              // THIRTH row
        {
          if ((y >= 5) && (y <= 35)) // Button: M
          {
            waitForIt(5, 90, 35, 120);
            updateStr('M');
          }
          if ((y >= 45) && (y <= 75)) // Button: N
          {
            waitForIt(45, 90, 75, 120);
            updateStr('N');
          }
          if ((y >= 85) && (y <= 115)) // Button: O
          {
            waitForIt(85, 90, 115, 120);
            updateStr('O');
          }
          if ((y >= 125) && (y <= 155)) // Button: P
          {
            waitForIt(125, 90, 155, 120);
            updateStr('P');
          }
          if ((y >= 165) && (y <= 195)) // Button: Q
          {
            waitForIt(165, 90, 195, 120);
            updateStr('Q');
          }
          if ((y >= 205) && (y <= 235)) // Button: R
          {
            waitForIt(205, 90, 235, 120);
            updateStr('R');
          }
        }
        if ((x >= 165) && (x <= 195))             // forth row
        {
          if ((y >= 5) && (y <= 35)) // Button: S
          {
            waitForIt(5, 130, 35, 160);
            updateStr('S');
          }
          if ((y >= 45) && (y <= 75)) // Button: T
          {
            waitForIt(45, 130, 75, 160);
            updateStr('T');
          }
          if ((y >= 85) && (y <= 115)) // Button: U
          {
            waitForIt(85, 130, 115, 160);
            updateStr('U');
          }
          if ((y >= 125) && (y <= 155)) // Button: V
          {
            waitForIt(125, 130, 155, 160);
            updateStr('V');
          }
          if ((y >= 165) && (y <= 195)) // Button: W
          {
            waitForIt(165, 130, 195, 160);
            updateStr('W');
          }
          if ((y >= 205) && (y <= 235)) // Button: X
          {
            waitForIt(205, 130, 235, 160);
            updateStr('X');
          }
        }
        if ((x >= 125) && (x <= 155))             // FIFTH row
        {
          if ((y >= 5) && (y <= 35)) // Button: Y
          {
            waitForIt(5, 170, 35, 200);
            updateStr('Y');
          }
          if ((y >= 45) && (y <= 75)) // Button: Z
          {
            waitForIt(45, 170, 75, 200);
            updateStr('Z');
          }
          if ((y >= 85) && (y <= 115)) // Button: *
          {
            waitForIt(85, 170, 115, 200);
            updateStr('*');
          }
          if ((y >= 125) && (y <= 235)) // Button: SPACE
          {
            waitForIt(125, 170, 235, 200);
            updateStr(' ');
          }

        }
        if ((x >= 85) && (x <= 115) && (y >= 5) && (y <= 235)) // sixth row
        {
          waitForIt(5, 210, 235, 240);
          goto NUMPAD_PORT;
        }
      }
    }
  }
}

/**************************************Password function (Page3)****************************/
void PassWord()
{
  int wrongPass = 0;
Password:
  PASSFLAG = true;
  ROOMFLAG = false;
  NumPad();
  PASSFLAG = false;
  if ((!strcmp(stLast, BoardSerial)) or (!strcmp(stLast, master)))
  {
    wrongPass = 0;
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(255, 255, 255);
    if (EEPROM.read(71))
      myGLCD.print("Please Wait...", CENTER, 210);
    if (!EEPROM.read(71))
      myGLCD.print("Please Wait...", CENTER, 250);
    delay(1000);
    stLast[0] = '\0';
    PassWordFlag = true;
    wrongPass = 0;
    timeoutFlag = false;
    return;
  }
  else {
    PassWordFlag = false;
    wrongPass ++ ;
    myGLCD.setColor(255, 0, 0);
    myGLCD.setBackColor(255, 255, 255);
    if (EEPROM.read(71))
    {
      myGLCD.print("Wrong Password", CENTER, 210);
      delay(500);
      myGLCD.print("              ", CENTER, 210);
      delay(500);
      myGLCD.print("Wrong Password", CENTER, 210);
      delay(500);
      myGLCD.print("               ", CENTER, 210);
    }
    if (!EEPROM.read(71))
    {
      myGLCD.print("Wrong Password", CENTER, 250);
      delay(500);
      myGLCD.print("              ", CENTER, 250);
      delay(500);
      myGLCD.print("Wrong Password", CENTER, 250);
      delay(500);
      myGLCD.print("               ", CENTER, 250);
    }
    stLast[0] = '\0';
    if ((wrongPass == 3) or timeoutFlag)
    {
      myGLCD.clrScr();
      wrongPass = 0;
      timeoutFlag = false;
      wrongpassFlag = true;
      return;
    }
    else
      goto Password;
  }
}
/******************************* Select Page************************/
void SetupPage()
{
  int suspendTIME = 15;
  myGLCD.clrScr();
  if (EEPROM.read(71))
  {
    myGLCD.InitLCD(LANDSCAPE);
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (0, 0, 319, 110);
    myGLCD.fillRoundRect (0, 120, 319, 239);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (0, 0, 319, 110);
    myGLCD.drawRoundRect (0, 120, 319, 239);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("Setup Page", CENTER, 55);
    myGLCD.print("Suspend", CENTER, 179);
  }
  if (!EEPROM.read(71))
  {
    myGLCD.InitLCD(PORTRAIT);
    myGLCD.setFont(BigFont);
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (10, 10, 230, 150);
    myGLCD.fillRoundRect (10, 160, 230, 310);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (10, 10, 230, 150);
    myGLCD.drawRoundRect (10, 160, 230, 310);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("Setup Page", CENTER, 70);
    myGLCD.print("Suspend", CENTER, 230);
  }
  while (true)
  {
    if (myTouch.dataAvailable())
    {
      if (EEPROM.read(71))
      {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();
        if ((x >= 0) && (x <= 319))
        {
          if ((y >= 120) && (y <= 239))       //suspend is selected
          {
            waitForIt(0, 120, 319, 239);
            myGLCD.clrScr();
            myGLCD.setBackColor(0, 0, 0);
            myGLCD.setColor(255, 255, 255);
            myGLCD.print("System will be", CENTER, 80);
            myGLCD.print("back in", CENTER, 120);
            while (suspendTIME > 0)
            {
              myGLCD.print("    ", CENTER, 160);
              suspendTIME = suspendTIME - 1;
              myGLCD.printNumI(suspendTIME, CENTER, 160);
              delay(1000);
            }
            drawLayer();
            return;
          }
          if ((y >= 0) && (y <= 110))     // setup is selected
          {
            waitForIt(0, 0, 319, 110);
            SetupFlag = true;
            PAGE = 1;
            goto Setup;
          }
        }
      }
      if (!EEPROM.read(71))
      {
        myTouch.read();
        x = myTouch.getX();
        y = myTouch.getY();
        if ((y >= 1) && (y <= 239))
        {
          if ((x >= 10) && (x <= 150))       //suspend is selected
          {
            waitForIt(10, 160, 230, 310);
            myGLCD.clrScr();
            myGLCD.setBackColor(0, 0, 0);
            myGLCD.setColor(255, 255, 255);
            myGLCD.print("System will be", CENTER, 80);
            myGLCD.print("back in", CENTER, 120);
            while (suspendTIME > 0)
            {
              myGLCD.print("    ", CENTER, 160);
              suspendTIME = suspendTIME - 1;
              myGLCD.printNumI(suspendTIME, CENTER, 160);
              delay(1000);
            }
            drawLayer();
            return;
          }
          if ((x >= 160) && (x <= 310))     // setup is selected
          {
            waitForIt(10, 10, 230, 150);
            SetupFlag = true;
            PAGE = 1;
            goto Setup;
          }
        }
      }
    }
  }

Setup:

  if (!EEPROM.read(71))
  {
    myGLCD.InitLCD(PORTRAIT);
    myGLCD.setFont(BigFont);
    myGLCD.clrScr();
    myGLCD.fillScr(255, 255, 255);

    //NEXT PAGE  and RETURNBUTTON
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (10, 211, 229.8, 256);
    myGLCD.fillRoundRect (10, 263, 229.8, 308);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (10, 211, 229.8, 256);
    myGLCD.drawRoundRect (10, 263, 229.8, 308);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("NEXT PAGE", CENTER, 225);
    myGLCD.print("RETURN", CENTER, 276);
  }
  if (EEPROM.read(71))
  {
    myGLCD.InitLCD(LANDSCAPE);
    myGLCD.setFont(BigFont);
    myGLCD.clrScr();
    myGLCD.fillScr(255, 255, 255);

    //NEXT PAGE  and RETURN BUTTON
    myGLCD.setColor(0, 0, 255);
    myGLCD.fillRoundRect (5, 185, 155, 235);
    myGLCD.fillRoundRect (160, 185, 315, 235);
    myGLCD.setColor(0, 0, 0);
    myGLCD.drawRoundRect (5, 185, 155, 235);
    myGLCD.drawRoundRect (160, 185, 315, 235);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("NEXT PAGE", 165, 205);
    myGLCD.print("RETURN", 30, 205);
  }
  //////////////////////////////////////PAGE 1//////////////////////////////////
  if (PAGE == 1)
  {
    if (!EEPROM.read(71))
    {
      //OPERATION MODE
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10, 5, 76.6, 50);
      myGLCD.fillRoundRect (86.6, 5, 153.2, 50);
      myGLCD.fillRoundRect (163.2, 5, 229.8, 50);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (10, 5, 76.6, 50);
      myGLCD.drawRoundRect (86.6, 5, 153.2, 50);
      myGLCD.drawRoundRect (163.2, 5, 229.8, 50);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("MOM", 15.2, 20);
      myGLCD.print("LCH", 92.8, 20);
      myGLCD.print("DLCH", 164.4, 20);

      // LAYOUT  Select
      myGLCD.setColor(0, 255, 0);
      myGLCD.fillRoundRect (10, 57, 76.6, 102);
      myGLCD.setColor(255, 255, 0);
      myGLCD.fillRoundRect (86.6, 57, 153.2, 102);
      myGLCD.setColor(255, 0, 0);
      myGLCD.fillRoundRect (163.2, 57, 229.8, 102);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (10, 57, 76.6, 102);
      myGLCD.drawRoundRect (86.6, 57, 153.2, 102);
      myGLCD.drawRoundRect (163.2, 57, 229.8, 102);
      myGLCD.setFont(BigFont);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setBackColor(0, 255, 0);
      myGLCD.print("CALL", 11.2, 73.5);
      myGLCD.setBackColor(255, 255, 0);
      myGLCD.print("ASS", 92.8, 73.5);
      myGLCD.setBackColor(255, 0, 0);
      myGLCD.print("EMG", 172.4, 73.5);

      //NAME BUTTON
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10, 109, 229.8, 154);
      myGLCD.fillRoundRect (10, 161, 229.8, 206);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (10, 109, 229.8, 154);
      myGLCD.drawRoundRect (10, 161, 229.8, 206);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("NAME", CENTER, 127);
      myGLCD.print("ROOM NUMBER", CENTER, 180.5);

      while (true)
      {
        if (myTouch.dataAvailable())
        {

          myTouch.read();
          x = myTouch.getX();
          y = myTouch.getY();

          /************************MODE SELSECT***************************/
          if ((x > 270) && (x < 315))
          {
            if ( (y > 9) && (y < 77.6) )   // MOMENTARY Select
            {
              waitForIt(10, 5, 76.6, 50);
              mode = 1;
              modeFlag = true;
              EEPROM.write(103, mode);
            }
            if ( (y > 87.6 ) &&  (y < 156.2) ) //LATCH Select
            {
              waitForIt(85.6, 5, 153.2, 50); //latch
              mode = 2;
              EEPROM.write(103, mode);
              modeFlag = true;
            }
            if ( (y > 162.2) && (y < 230.8) ) // dual latch
            {
              waitForIt(163.2, 5, 229.8, 50);
              mode = 3;
              EEPROM.write(103, mode);
              modeFlag = true;
            }
          }
          /************************LAYOUT SELSECT***************************/
          if ((x > 218) && (x < 263))         //LAYOUT SELSECT
          {
            if ( (y > 9) && (y < 77.6))   // CALL LAYOUT Select
            {
              waitForIt(10, 57, 76.6, 102);
              EEPROM.write(102, 0x01);
            }

            if ( (y > 87.6 ) &&  (y < 154.2) ) //ASSISST LAYOUT Select
            {
              waitForIt(86.6, 57, 153.2, 102);
              EEPROM.write(102, 0x02);
            }
            if ( (y > 162.2) && (y < 230.8)) // EMERGENCY LAYOUT
            {
              waitForIt(163.2, 57, 229.8, 102);
              EEPROM.write(102, 0x03);
            }
          }

          /************************NAME SELSECT***************************/
          if ((x > 166) && (y > 9) &&  (x < 211) && (y < 230.8) )
          {
            waitForIt(10, 109, 229.8, 154);
            PASSFLAG = false;
            SetFlag = false;
            SetRoom = false;
            ROOMFLAG = false;
            NAMEFLAG = true;
            NumPad();
            NAMEFLAG = false;
            goto Setup;
          }

          /************************ROOM NUMBER SELSECT***************************/
          if ((x > 114) && (y > 9) &&  (x < 159) && (y < 230.8) )
          {
            waitForIt(10, 161, 229.8, 206);
            PASSFLAG = false;
            SetFlag = false;
            SetRoom = true;
            ROOMFLAG = true;
            NAMEFLAG = false;
            NumPad();
            SetRoom = false;
            ROOMFLAG = false;
            goto Setup;
          }
          /************************NEXT PAGE SELSECT***************************/
          if ((x > 62) && (y > 9) &&  (x < 107) && (y < 230.8) )
          {
            waitForIt(10, 211, 229.8, 256);
            PAGE++;
            goto Setup;
          }
          /************************RETURN SELSECT***************************/
          if ((x > 10) && (y > 9) &&  (x < 55) && (y < 230.8) )
          {
            waitForIt(10, 263, 229.8, 308);
            if (PAGE > 1)
            {
              PAGE--;
              goto Setup;
            }
            else
            {
              drawLayer();
              return;
            }
          }
        }
      }
    }
    if (EEPROM.read(71))
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5, 5, 105, 45);
      myGLCD.fillRoundRect (110, 5, 210, 45);
      myGLCD.fillRoundRect (215, 5, 315, 45);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5, 5, 105, 45);
      myGLCD.drawRoundRect (110, 5, 210, 45);
      myGLCD.drawRoundRect (215, 5, 315, 45);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("MOM", 35, 20);
      myGLCD.print("LCH", 135, 20);
      myGLCD.print("DLCH", 235, 20);

      myGLCD.setColor(0, 255, 0);
      myGLCD.fillRoundRect (5, 50, 105, 90);
      myGLCD.setColor(255, 255, 0);
      myGLCD.fillRoundRect (110, 50, 210, 90);
      myGLCD.setColor(255, 0, 0);
      myGLCD.fillRoundRect (215, 50, 315, 90);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5, 50, 105, 90);
      myGLCD.drawRoundRect (110, 50, 210, 90);
      myGLCD.drawRoundRect (215, 50, 315, 90);
      myGLCD.setFont(BigFont);
      myGLCD.setColor(255, 255, 255);
      myGLCD.setBackColor(0, 255, 0);
      myGLCD.print("CALL", 25, 62);
      myGLCD.setBackColor(255, 255, 0);
      myGLCD.print("ASSIST", 112, 62);
      myGLCD.setBackColor(255, 0, 0);
      myGLCD.print("EMERG", 225, 62);

      //ROOM NUMBER and NAME BUTTON
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5, 95, 315, 135);
      myGLCD.fillRoundRect (5, 140, 315, 180);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5, 95, 315, 135);
      myGLCD.drawRoundRect (5, 140, 315, 180);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("NAME", CENTER, 110);
      myGLCD.print("ROOM NUMBER", CENTER, 155);

      if (EEPROM.read(71))
      {
        while (true) {

          if (myTouch.dataAvailable())
          {

            myTouch.read();
            x = myTouch.getX();
            y = myTouch.getY();
            /************************MODE SELSECT***************************/
            if ((y >= 5) && (y <= 45 ))
            {
              if ((x >= 5) && (x <= 105))    //MOMENTARY
              {
                waitForIt(5, 5, 105, 45);
                mode = 1;
                modeFlag = true;
                EEPROM.write(103, mode);
              }
              if ((x >= 110) && (x <= 210))   //LATCH
              {
                waitForIt(110, 5, 210, 45);
                mode = 2;
                EEPROM.write(103, mode);
                modeFlag = true;
              }
              if ((x >= 215) && (x <= 315))   //D-LATCH
              {
                waitForIt(215, 5, 315, 45);
                mode = 3;
                EEPROM.write(103, mode);
                modeFlag = true;
              }
            }
            if ((y >= 50) && (y <= 90 ))
            {
              if ((x >= 5) && (x <= 105))     //CALL
              {
                waitForIt(5, 50, 105, 90);
                EEPROM.write(102, 0x01);
              }
              if ((x >= 110) && (x <= 210))     //ASSIST
              {
                waitForIt(110, 50, 210, 90);
                EEPROM.write(102, 0x02);
              }
              if ((x >= 215) && (x <= 315))     //EMERGENCY
              {
                waitForIt(215, 50, 315, 90);
                EEPROM.write(102, 0x03);
              }
            }
            if ((x >= 5) && (y >= 95) && (x <= 315) && (y <= 135 ))   //NAME
            {
              waitForIt(5, 95, 315, 135);
              PASSFLAG = false;
              SetFlag = false;
              SetRoom = false;
              ROOMFLAG = false;
              NAMEFLAG = true;
              NumPad();
              NAMEFLAG = false;
              goto Setup;
            }
            if ((x >= 5) && (y >= 140) && (x <= 315) && (y <= 180 ))   //ROOM NUMBER
            {
              waitForIt(5, 140, 315, 180);
              PASSFLAG = false;
              SetFlag = false;
              SetRoom = true;
              ROOMFLAG = true;
              NAMEFLAG = false;
              NumPad();
              SetRoom = false;
              ROOMFLAG = false;
              goto Setup;
            }
            if ((y >= 185) && (y <= 235 ))              //NEXT AND RETURN
            {
              if ((x >= 160) && (x <= 315))               //NEXT
              {
                waitForIt(160, 185, 315, 235);
                PAGE++;
                goto Setup;
              }
              if ((x >= 5) && (x <= 155))             //RETURN
              {
                waitForIt(5, 185, 155, 235);
                if (PAGE > 1)
                {
                  PAGE--;
                  goto Setup;
                }
                else
                {
                  drawLayer();
                  return;
                }
              }
            }
          }
        }
      }
    }
  }
  //////////////////////////////////PAGE 2//////////////////////////////////
PAGE2:
  if (PAGE == 2)
  {
    if (!EEPROM.read(71))
    {
      //WIRELESS BUTTON
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10, 5, 229.8, 50);
      myGLCD.fillRoundRect (10, 57, 229.8, 102);
      myGLCD.fillRoundRect (10, 109, 229.8, 154);
      myGLCD.fillRoundRect (10, 161, 229.8, 206);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (10, 57, 229.8, 102);
      myGLCD.drawRoundRect (10, 5, 229.8, 50);
      myGLCD.drawRoundRect (10, 109, 229.8, 154);
      myGLCD.drawRoundRect (10, 161, 229.8, 206);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("WIRELESS", CENTER, 20);
      myGLCD.print("TIME", CENTER, 73.5);
      myGLCD.print("PASSWORD", CENTER, 127);
      myGLCD.print(" LANDSCAP ", CENTER, 180.5);

      while (true)
      {
        if (myTouch.dataAvailable())
        {
          myTouch.read();
          x = myTouch.getX();
          y = myTouch.getY();

          /************************WIRELESS SELSECT***************************/
          if ((x > 270) && (y > 9) &&  (x < 315) && (y < 230.8) )
          {
            waitForIt(10, 5, 229.8, 50);
            myGLCD.clrScr();
            myGLCD.fillScr(255, 255, 255);
            myGLCD.setColor(0, 255, 255);
            myGLCD.fillRoundRect (5, 5, 235, 71);
            myGLCD.fillRoundRect (5, 88, 235, 157);
            myGLCD.fillRoundRect (5, 174, 235, 240);
            myGLCD.fillRoundRect (5, 257, 235, 314);
            myGLCD.setColor(0, 0, 0);
            myGLCD.drawRoundRect (5, 5, 235, 71);
            myGLCD.drawRoundRect (5, 88, 235, 157);
            myGLCD.drawRoundRect (5, 174, 235, 240);
            myGLCD.drawRoundRect (5, 257, 235, 314);
            myGLCD.setBackColor(0, 255, 255);
            myGLCD.setColor(0, 0, 0);
            myGLCD.print("Pairing", CENTER, 30);              //Pair Butoon
            myGLCD.print("CLEAR MEMORY", CENTER, 116);  //MEMORY CLEAR BUTTON
            myGLCD.print("DEVICES:", 20, 202);       //RETURN BUTTON
            myGLCD.printNumI((memPointer - 1), 180, 202);          //print number of paired devices
            myGLCD.print("RETURN", CENTER, 280);
            readID5();
            while (true)
            {
              if (myTouch.dataAvailable())
              {
                myTouch.read();
                x = myTouch.getX();
                y = myTouch.getY();
                if ((y >= 5) && (y <= 315))
                {

                  ////////////////////////////Pairing Devices
                  if ((x >= 249) && (x <= 315))
                  {
                    waitForIt(5, 5, 235, 71);
                    myGLCD.clrScr();
                    myGLCD.fillScr(255, 255, 255);
                    myGLCD.setBackColor(255, 255, 255);
                    myGLCD.setColor(0, 0, 0);
                    myGLCD.print("Press Wireless", CENTER, 10);
                    myGLCD.print("Pendant", CENTER, 50);
                    myGLCD.print("OR", CENTER, 90);
                    myGLCD.print("BED/CHAIR MAT", CENTER, 130);
                    vw_rx_start();                      // Start the receiver PLL running
                    for (int i = 5; i >= 0; i--)
                    {
                      myGLCD.printNumI(i, CENTER, 210);
                      delay(1000);
                      if (vw_have_message())
                        break;
                    }
                    delay(500);
                    uint8_t buf[VW_MAX_MESSAGE_LEN];
                    uint8_t buflen = VW_MAX_MESSAGE_LEN;
                    if (vw_get_message(buf, &buflen)) {    // Non-blocking

                      for (int i = 0; i < buflen; i++) {  // Message with a good checksum received, dump it.
                        CharMsg[i] = char(buf[i]);        // Fill CharMsg Char array with corresponding chars from buffer.
                      }
                      CharMsg[buflen] = '\0';
                      rfIDnum = atoi(CharMsg);            // Convert CharMsg Char array to integer
                      for (int i = 0; i < buflen; i++) {  // Empty rf buffer
                        CharMsg[i] = 0;
                      }
                    }
                    // Stop the receiver PLL running
                    readID5();
                    if (rfIDnum != 0x0000 &&                // if not 0000 then store
                        rfIDnum != id1 &&
                        rfIDnum != id2 &&
                        rfIDnum != id3 &&
                        rfIDnum != id4 &&
                        rfIDnum != id5 )                 // Check if any match rfIDnum
                    {
                      storeID5();                         // Store rfIDnum into a mem. location
                      delay(500);
                      readID5();                          // Reload IDs
                      rfIDnum = 0x0000;                   // reset ID number
                    }
                    else if ((rfIDnum != 0x0000) &&               // if not 0000 then store
                             (rfIDnum = id1) ||
                             (rfIDnum = id2) ||
                             (rfIDnum = id3) ||
                             (rfIDnum = id4) ||
                             (rfIDnum = id5 ))
                    {
                      myGLCD.clrScr();
                      myGLCD.setBackColor(0, 0, 0);
                      myGLCD.setColor(255, 255, 255);
                      myGLCD.print("Device is", CENTER, 40);
                      myGLCD.print("Already Paired", CENTER, 80);
                      myGLCD.print("OR", CENTER, 120);
                      myGLCD.print("Not Detected", CENTER, 160);
                      delay(5000);
                    }
                    goto Setup;
                  }

                  /////////////////////// // Clear paired Devices
                  if ((x >= 166) && (x <= 232))
                  {
                    waitForIt(5, 88, 235, 157);
                    myGLCD.clrScr();
                    myGLCD.fillScr (255, 0, 0);
                    myGLCD.setBackColor(255, 0, 0);
                    myGLCD.setColor(255, 255, 255);
                    myGLCD.setFont(BigFont);
                    myGLCD.print("DELETING ALL", CENTER, 30);
                    myGLCD.print("PAIRED DEVICES", CENTER, 60);
                    myGLCD.print("ARE YOU SURE?", CENTER, 120);
                    myGLCD.print("YES", 30, 190);
                    myGLCD.print("NO", 165, 190);
                    myGLCD.drawRoundRect (10, 170, 110, 230); // YES button
                    myGLCD.drawRoundRect (130, 170, 230, 230); // NO Button
                    while (true)
                    {
                      if (myTouch.dataAvailable())
                      {
                        myTouch.read();
                        x = myTouch.getX();
                        y = myTouch.getY();
                        if ((x >= 90) && (x <= 150) )
                        {
                          if ( (y >= 10) && (y <= 110)) //Pressing YES button
                          {
                            waitForIt(10, 170, 110, 230);
                            myGLCD.clrScr();
                            myGLCD.fillScr(255, 255, 255);
                            myGLCD.setBackColor(255, 255, 255);
                            myGLCD.setColor(0, 0, 0);
                            myGLCD.print("CLEARING MEMORY", CENTER, 40);
                            myGLCD.print("Please Wait", CENTER, 120);
                            delay(1000);
                            rfIDnum = 0xFFFF;                       // Erase ID number
                            resetID5();                             // reset all 5 mem.
                            readID5();                              // Reload IDs to static registers
                            for (int i = 0; i < 5; i ++)
                            {
                              myGLCD.print("*", 80 + (i * 20), 160);
                              delay(500);
                            }
                            goto Setup;
                          }
                          if ((y >= 130) && (y <= 230))//Presiing NO button
                          {
                            waitForIt(130, 170, 230, 230);
                            goto Setup;
                          }
                        }
                      }
                    }
                  }
                  if ((x >= 5) && (x <= 66)) // Number of Paired Devices
                  {
                    waitForIt(5, 257, 235, 314);
                    goto Setup;
                  }
                }
              }
            }
          }

          /************************TIME BUTTON***************************/
          if ((x > 218) && (y > 9) &&  (x < 263) && (y < 230.8) )
          {
            waitForIt(10, 57, 229.8, 102);
            TimePad();
            goto Setup;
          }

          /************************PASSWORD SELSECT***************************/
          if ((x > 165) && (y > 9) &&  (x < 210) && (y < 230.8) )
          {
            waitForIt(10, 109, 229.8, 154);
            PASSFLAG = true;
            SetFlag = true;
            SetRoom = false;
            ROOMFLAG = false;
            NAMEFLAG = false;
            NumPad();
            PASSFLAG = false;
            SetFlag = false;
            goto Setup;
          }
          /***********************ORIENTATION*********************************/
          if ((x > 113) && (y > 9) &&  (x < 158) && (y < 230.8) )
          {
            waitForIt(10, 161, 229.8, 206);
            EEPROM.write(71, !EEPROM.read(71));
            goto Setup;
          }

          /************************NEXT PAGE SELSECT***************************/
          if ((x > 61) && (y > 9) &&  (x < 106) && (y < 230.8) )
          {
            waitForIt(10, 211, 229.8, 256);
            PAGE++;
            goto Setup;
          }
          /************************RETURN SELSECT***************************/
          if ((x > 9) && (y > 9) &&  (x < 54) && (y < 230.8) )
          {
            waitForIt(10, 263, 229.8, 308);
            PAGE--;
            goto Setup;
          }
        }
      }
    }

    if (EEPROM.read(71))
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5, 5, 315, 45);      //wireless
      myGLCD.fillRoundRect (5, 50, 315, 90);     //time
      myGLCD.fillRoundRect (5, 95, 315, 135);      //password
      myGLCD.fillRoundRect (5, 140, 315, 180);     //orientation
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5, 5, 315, 45);
      myGLCD.drawRoundRect (5, 50, 315, 90);
      myGLCD.drawRoundRect (5, 95, 315, 135);
      myGLCD.drawRoundRect (5, 140, 315, 180);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("WIRELESS", CENTER, 20);
      myGLCD.print("TIME", CENTER, 65);
      myGLCD.print("PASSWORD", CENTER, 110);
      myGLCD.print("PORTRAIT", CENTER, 150);
      while (true)
      {
        if (myTouch.dataAvailable())
        {

          myTouch.read();
          x = myTouch.getX();
          y = myTouch.getY();

          if ((x >= 5) && (x <= 315 ))
          {
            if ((y >= 5) && (y <= 45))         //wireless
            {
              waitForIt(5, 5, 315, 45);
              myGLCD.clrScr();
              myGLCD.fillScr(255, 255, 255);
              myGLCD.setColor(0, 255, 255);
              myGLCD.fillRoundRect (5, 5, 315, 58.75);
              myGLCD.fillRoundRect (5, 63.75, 315, 117.5);
              myGLCD.fillRoundRect (5, 122.5, 315, 176.25);
              myGLCD.fillRoundRect (5, 181.25, 315, 235);
              myGLCD.setColor(0, 0, 0);
              myGLCD.drawRoundRect (5, 5, 315, 58.75);
              myGLCD.drawRoundRect (5, 63.75, 315, 117.5);
              myGLCD.drawRoundRect (5, 122.5, 315, 176.25);
              myGLCD.drawRoundRect (5, 181.25, 315, 235);
              myGLCD.setBackColor(0, 255, 255);
              myGLCD.setColor(0, 0, 0);
              myGLCD.print("Pairing", CENTER, 25);              //Pair Butoon
              myGLCD.print("CLEAR MEMORY", CENTER, 85);  //MEMORY CLEAR BUTTON
              myGLCD.print("Devices:", 15, 140);       //RETURN BUTTON
              myGLCD.printNumI((EEPROM.read(19) - 1), 200, 140);
              myGLCD.print("RETURN", CENTER, 200);
              readID5();
              while (true)
              {
                if (myTouch.dataAvailable())
                {

                  myTouch.read();
                  x = myTouch.getX();
                  y = myTouch.getY();

                  if ((x >= 5) && (x <= 315 ))
                  {
                    if ((y >= 5) && (y <= 58.75))      //PAIRING
                    {
                      vw_rx_start();
                      waitForIt(5, 5, 315, 58.75);
                      myGLCD.clrScr();
                      myGLCD.fillScr(255, 255, 255);
                      myGLCD.setBackColor(255, 255, 255);
                      myGLCD.setColor(0, 0, 0);
                      myGLCD.print("Press Wireless", CENTER, 10);
                      myGLCD.print("Pendant", CENTER, 50);
                      myGLCD.print("OR", CENTER, 90);
                      myGLCD.print("BED/CHAIR MAT", CENTER, 130);
                      for (int i = 5; i >= 0; i--)
                      {
                        myGLCD.printNumI(i, CENTER, 210);
                        delay(1000);
                      }
                      delay(500);
                      vw_rx_start();                          // Start the receiver PLL running
                      uint8_t buf[VW_MAX_MESSAGE_LEN];
                      uint8_t buflen = VW_MAX_MESSAGE_LEN;
                      if (vw_get_message(buf, &buflen)) {// Non-blocking

                        for (int i = 0; i < buflen; i++) {  // Message with a good checksum received, dump it.
                          CharMsg[i] = char(buf[i]);        // Fill CharMsg Char array with corresponding chars from buffer.
                        }
                        CharMsg[buflen] = '\0';
                        rfIDnum = atoi(CharMsg);            // Convert CharMsg Char array to integer
                        for (int i = 0; i < buflen; i++) {  // Empty rf buffer
                          CharMsg[i] = 0;
                        }
                      }
                      // Stop the receiver PLL running
                      readID5();
                      if (rfIDnum != 0x0000 &&                // if not 0000 then store
                          rfIDnum != id1 &&
                          rfIDnum != id2 &&
                          rfIDnum != id3 &&
                          rfIDnum != id4 &&
                          rfIDnum != id5 )                 // Check if any match rfIDnum
                      {
                        storeID5();                         // Store rfIDnum into a mem. location
                        delay(500);
                        readID5();                          // Reload IDs
                        rfIDnum = 0x0000;                   // reset ID number
                      }
                      else if ((rfIDnum != 0x0000) &&               // if not 0000 then store
                               (rfIDnum = id1) ||
                               (rfIDnum = id2) ||
                               (rfIDnum = id3) ||
                               (rfIDnum = id4) ||
                               (rfIDnum = id5 ))
                      {
                        myGLCD.clrScr();
                        myGLCD.setBackColor(0, 0, 0);
                        myGLCD.setColor(255, 255, 255);
                        myGLCD.print("Device is", CENTER, 40);
                        myGLCD.print("Already Paired", CENTER, 80);
                        myGLCD.print("OR", CENTER, 120);
                        myGLCD.print("Not Detected", CENTER, 160);
                        delay(5000);
                      }
                      goto Setup;
                    }
                    if ((y >= 63.75) && (y <= 117.5))      //CLEAR MEMORY
                    {
                      waitForIt(5, 63.75, 315, 117.5);
                      myGLCD.fillScr (255, 0, 0);
                      myGLCD.setBackColor(255, 0, 0);
                      myGLCD.setColor(255, 255, 255);
                      myGLCD.setFont(BigFont);
                      myGLCD.print("DELETING ALL", CENTER, 30);
                      myGLCD.print("PAIRED DEVICES", CENTER, 60);
                      myGLCD.print("ARE YOU SURE?", CENTER, 120);
                      myGLCD.print("YES", 45, 190);
                      myGLCD.print("NO", 215, 190);
                      myGLCD.drawRoundRect (20, 170, 120, 230); // YES button
                      myGLCD.drawRoundRect (180, 170, 280, 230); // NO Button
                      while (true)
                      {
                        if (myTouch.dataAvailable())
                        {
                          myTouch.read();
                          x = myTouch.getX();
                          y = myTouch.getY();
                          if ((x >= 20) && (x <= 120) && (y >= 170) && (y <= 230)) //Pressing YES button
                          {
                            waitForIt(20, 170, 120, 230);
                            myGLCD.clrScr();
                            myGLCD.setBackColor(0, 0, 0);
                            myGLCD.setColor(255, 255, 255);
                            myGLCD.print("CLEARING MEMORY", CENTER, 40);
                            myGLCD.print("Please Wait", CENTER, 120);
                            delay(1000);
                            rfIDnum = 0xFFFF;                       // Erase ID number
                            resetID5();                             // reset all 5 mem.
                            readID5();                              // Reload IDs to static registers
                            for (int i = 0; i < 5; i ++)
                            {
                              myGLCD.print("*", 80 + (i * 20), 160);
                              delay(500);
                            }
                            goto Setup;
                          }
                          if ((x >= 180) && (x <= 280) && (y >= 170) && (y <= 230)) //Presiing NO button
                          {
                            waitForIt(180, 170, 280, 230);
                            goto Setup;
                          }
                        }
                      }
                    }
                    if ((y >= 181.25) && (y <= 235))
                    {
                      waitForIt(5, 181.25, 315, 235);
                      goto Setup;
                    }
                  }
                }
              }
            }

            if ((y >= 50) && (y <= 90))       //TIME
            {
              waitForIt(5, 50, 315, 90);
              TimePad();
              goto Setup;
            }
            if ((y >= 95) && (y <= 135))      //PASSWORD
            {
              waitForIt(5, 95, 315, 135);
              PASSFLAG = true;
              SetFlag = true;
              NumPad();
              PASSFLAG = false;
              SetFlag = false;
              goto Setup;
            }
            if ((y >= 140) && (y <= 180))     //ORIENTATION
            {
              waitForIt(5, 140, 315, 180);
              EEPROM.write(71, !EEPROM.read(71));
              goto Setup;
            }
          }
          if ((y >= 185) && (y <= 235 ))              //NEXT AND RETURN
          {
            if ((x >= 160) && (x <= 315))               //NEXT
            {
              waitForIt(160, 185, 315, 235);
              PAGE++;
              goto Setup;
            }
            if ((x >= 5) && (x <= 155))             //RETURN
            {
              waitForIt(5, 185, 155, 235);
              if (PAGE > 1)
              {
                PAGE--;
                goto Setup;
              }
              else
              {
                drawLayer();
                return;
              }
            }
          }
        }
      }
    }
  }
  ////////////////////////////////////////////////PAGE 3//////////////////////////////////////////
  if (PAGE == 3)
  {
    if (!EEPROM.read(71))
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (10, 5, 229.8, 50);
      myGLCD.fillRoundRect (10, 57, 229.8, 102);
      myGLCD.fillRoundRect (10, 109, 229.8, 154);
      myGLCD.fillRoundRect (10, 161, 229.8, 206);   ////////RESERVED
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (10, 5, 229.8, 50);
      myGLCD.drawRoundRect (10, 57, 229.8, 102);
      myGLCD.drawRoundRect (10, 109, 229.8, 154);
      myGLCD.drawRoundRect (10, 161, 229.8, 206); //////RESERVED
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("BACKUP", CENTER, 20);
      myGLCD.print("SOUND:", 20, 73.5);
      if (BeepFlag)
        myGLCD.print("OFF", 155, 73.5);
      else
        myGLCD.print("ON ", 155, 73.5);
      myGLCD.print("CONTACTS", CENTER, 127);

      while (true)
      {
        if (myTouch.dataAvailable())
        {
          myTouch.read();
          x = myTouch.getX();
          y = myTouch.getY();

          /************************BACKUP SELSECT****************************/
          if ((x > 270) && (y > 9) &&  (x < 315) && (y < 230.8) )
          {
            waitForIt(10, 5, 229.8, 50);
            myGLCD.clrScr();
            myGLCD.fillScr(255, 255, 255);
            myGLCD.setColor(0, 255, 0);
            myGLCD.fillRoundRect (10, 10, 230, 77.5);
            myGLCD.setColor(255, 255, 0);
            myGLCD.fillRoundRect (10, 87.5, 230, 155);
            myGLCD.setColor(255, 0, 255);
            myGLCD.fillRoundRect (10, 165, 230, 232.5);
            myGLCD.setColor(0, 255, 255);
            myGLCD.fillRoundRect (10, 242.5, 230, 310);
            myGLCD.setColor(0, 0, 0);
            myGLCD.drawRoundRect (10, 10, 230, 77.5);
            myGLCD.drawRoundRect (10, 87.5, 230, 155);
            myGLCD.drawRoundRect (10, 165, 230, 232.5);
            myGLCD.drawRoundRect (10, 242.5, 230, 310);
            myGLCD.setColor(0, 0, 0);
            myGLCD.setBackColor(0, 255, 0);
            myGLCD.print("Latest Log", CENTER, 33);
            myGLCD.setBackColor(255, 255, 0);
            myGLCD.print("30 Days ago", CENTER, 116);
            myGLCD.setBackColor(255, 0, 255);
            myGLCD.print("60 days ago", CENTER, 190);
            myGLCD.setBackColor(0, 255, 255);
            myGLCD.print("RETURN", CENTER, 270);
            while (true) {
              if (myTouch.dataAvailable())
              {
                myTouch.read();
                x = myTouch.getX();
                y = myTouch.getY();

                if ((y >= 5) && ( y <= 235))
                {
                  if ((x >= 242.5) && (x <= 310))
                  {
                    waitForIt(10, 10, 230, 77.5);
                    FileSelect = 1;
                    break;
                  }
                  if ((x >= 165) && (x <= 232.5))
                  {
                    waitForIt(10, 87.5, 230, 155);
                    FileSelect = 2;
                    break;
                  }
                  if ((x >= 87.5) && (x <= 155))
                  {
                    waitForIt(10, 165, 230, 232.5);
                    FileSelect = 3;
                    break;
                  }
                  if ((x >= 10) && (x <= 77.5))
                  {
                    waitForIt(10, 242.5, 230, 310);
                    goto Setup;
                  }
                }
              }
            }
            myGLCD.clrScr();
            if (SD.begin(53))
            {
              switch (FileSelect)
              {
                case (1):
                  myFile = SD.open("LOG.txt");
                  break;
                case (2):
                  myFile = SD.open("30_Days_Ago.txt");
                  break;
                case (3):
                  myFile = SD.open("60_Days_Ago.txt");
                  break;
              }
              // myGLCD.clrScr();

              if (myFile)
              {
                myGLCD.fillScr (0, 255, 0);
                myGLCD.setBackColor(0, 255, 0);
                myGLCD.setColor(0, 0, 0);
                myGLCD.setFont(BigFont);
                myGLCD.print("Transfering", CENTER, 150);
                myGLCD.print("Data...", CENTER, 190);
                while (myFile.available())
                  Serial.write(myFile.read());
                myFile.close();
                goto Setup;
              }
              else
              {
                Serial.println("error opening File or No Log is Available.");
                myGLCD.fillScr(255, 255, 0);
                myGLCD.setColor(0, 0, 0);
                myGLCD.setBackColor(255, 255, 0);
                myGLCD.print("LOG Not", CENTER, 150);
                myGLCD.print("Available", CENTER, 190);
                delay(2000);
                goto Setup;
              }
            }
            else
            {
              Serial.println("error opening SD Card!!");
              myGLCD.fillScr(255, 0, 0);
              myGLCD.setColor(0, 0, 0);
              myGLCD.setBackColor(255, 0, 0);
              myGLCD.print("Error Opening", CENTER, 150);
              myGLCD.print("SD Card!!", CENTER, 190);
              delay(2000);
              goto Setup;
            }
          }

          /************************SOUND BUTTON***************************/
          if ((x > 218) && (y > 9) &&  (x < 263) && (y < 230.8) )
          {
            waitForIt(10, 57, 229.8, 102);
            BeepFlag = !BeepFlag;
            myGLCD.setBackColor(0, 0, 255);
            myGLCD.setColor(255, 255, 255);
            if (BeepFlag)
              myGLCD.print("OFF", 155, 73.5);
            else
              myGLCD.print(" ON ", 155, 73.5);

          }

          /************************CONTACTS SELSECT***************************/
          if ((x > 165) && (y > 9) &&  (x < 210) && (y < 230.8) )
          {
            waitForIt(10, 109, 229.8, 154);
            myGLCD.clrScr();
            myGLCD.fillScr(255, 255, 255);
            myGLCD.setColor(0, 0, 0);
            myGLCD.setFont(BigFont);
            myGLCD.setBackColor(255, 255, 255);
            myGLCD.print("INDIGO CARE", CENTER, 10);
            myGLCD.setFont(SmallFont);
            myGLCD.print("  Phone: +61 3 97632988", LEFT, 40);
            myGLCD.print("  Fax:   +61 3 97633811", LEFT, 60);
            myGLCD.print("  Email: SALES@INDIGOCARE.COM.AU", LEFT, 80);
            myGLCD.print("  WWW.INDIGOCARE.COM.AU", LEFT, 100);
            myGLCD.print("  ADDRESS: Unit 2/71 Rushdale St.", LEFT, 120);
            myGLCD.print("           Knoxfield, VIC 3180", LEFT, 140 );
            myGLCD.print("           Australia", LEFT, 160);
            myGLCD.print("  ABN: 36 007 302 801", LEFT, 180);
            myGLCD.print("  Software Version:  ", LEFT, 200);
            myGLCD.print(SWver, 200, 200);
            myGLCD.print("  Serial Number:  ", LEFT, 220);
            myGLCD.print(BoardSerial, 200, 220);

            while (true) {
              if (myTouch.dataAvailable())
              {
                myTouch.read();
                x = myTouch.getX();
                y = myTouch.getY();
                if ( (x > 0) && (y > 0) && (x < 319) && (y < 239))
                  goto Setup;
              }
            }
            goto Setup;
          }
          /***********************RESERVED*********************************/
          if ((x > 113) && (y > 9) &&  (x < 158) && (y < 230.8) )
          {
            waitForIt(10, 161, 229.8, 206);
          }

          /************************NEXT PAGE RESERVED SELSECT***************************/
          if ((x > 61) && (y > 9) &&  (x < 106) && (y < 230.8) )
          {
            waitForIt(10, 211, 229.8, 256);
          }
          /************************RETURN SELSECT***************************/
          if ((x > 9) && (y > 9) &&  (x < 54) && (y < 230.8) )
          {
            waitForIt(10, 263, 229.8, 308);
            PAGE--;
            goto Setup;
          }
        }
      }
    }
    if (EEPROM.read(71))
    {
      myGLCD.setColor(0, 0, 255);
      myGLCD.fillRoundRect (5, 5, 315, 45);
      myGLCD.fillRoundRect (5, 50, 315, 90);
      myGLCD.fillRoundRect (5, 95, 315, 135);
      myGLCD.fillRoundRect (5, 140, 315, 180);
      myGLCD.setColor(0, 0, 0);
      myGLCD.drawRoundRect (5, 5, 315, 45);
      myGLCD.drawRoundRect (5, 50, 315, 90);
      myGLCD.drawRoundRect (5, 95, 315, 135);
      myGLCD.drawRoundRect (5, 140, 315, 180);
      myGLCD.setFont(BigFont);
      myGLCD.setBackColor(0, 0, 255);
      myGLCD.setColor(255, 255, 255);
      myGLCD.print("BACKUP", CENTER, 20);
      myGLCD.print("CONTACTS", CENTER, 110);
      myGLCD.print("SOUND:", 20, 65);
      if (BeepFlag)
        myGLCD.print("OFF", 155, 65);
      else
        myGLCD.print("ON ", 155, 65);
      while (true)
      {
        if (myTouch.dataAvailable())
        {

          myTouch.read();
          x = myTouch.getX();
          y = myTouch.getY();

          if ((x >= 5) && (x <= 315 ))
          {
            if ((y >= 5) && (y <= 45))        //BACKUP
            {
              waitForIt(5, 5, 315, 45);
              myGLCD.clrScr();
              myGLCD.fillScr(255, 255, 255);
              myGLCD.setColor(0, 255, 0);
              myGLCD.fillRoundRect (10, 10, 310, 57.5);
              myGLCD.setColor(255, 255, 0);
              myGLCD.fillRoundRect (10, 67.5, 310, 115);
              myGLCD.setColor(255, 0, 0);
              myGLCD.fillRoundRect (10, 125, 310, 172.5);
              myGLCD.setColor(0, 255, 255);
              myGLCD.fillRoundRect (10, 182.5, 310, 230);
              myGLCD.setColor(0, 0, 0);
              myGLCD.drawRoundRect (10, 10, 310, 57.5);
              myGLCD.drawRoundRect (10, 67.5, 310, 115);
              myGLCD.drawRoundRect (10, 125, 310, 172.5);
              myGLCD.drawRoundRect (10, 182.5, 310, 230);
              myGLCD.setColor(0, 0, 0);
              myGLCD.setBackColor(0, 255, 0);
              myGLCD.print("Latest Log", CENTER, 25);
              myGLCD.setBackColor(255, 255, 0);
              myGLCD.print("30 Days ago", CENTER, 85);
              myGLCD.setBackColor(255, 0, 0);
              myGLCD.print("60 days ago", CENTER, 140);
              myGLCD.setBackColor(0, 255, 255);
              myGLCD.print("RETURN", CENTER, 200);
              while (true)
              {
                if (myTouch.dataAvailable())
                {
                  myTouch.read();
                  x = myTouch.getX();
                  y = myTouch.getY();

                  if ((x >= 5) && ( x <= 315))
                  {
                    if ((y >= 10) && (y <= 57.5))
                    {
                      waitForIt(10, 10, 310, 57.5);
                      FileSelect = 1;
                      break;
                    }
                    if ((y >= 67.5) && (y <= 115))
                    {
                      waitForIt(10, 67.5, 310, 115);
                      FileSelect = 2;
                      break;
                    }
                    if ((y >= 125) && (y <= 172.5))
                    {
                      waitForIt(10, 125, 310, 172.5);
                      FileSelect = 3;
                      break;
                    }
                    if ((y >= 182.5) && (y <= 230))
                    {
                      waitForIt(10, 182.5, 310, 230);
                      goto Setup;
                    }
                  }
                }
              }
              myGLCD.clrScr();
              if (SD.begin(53))
              {
                switch (FileSelect)
                {
                  case (1):
                    myFile = SD.open("LOG.txt");
                    break;
                  case (2):
                    myFile = SD.open("30_Days_Ago.txt");
                    break;
                  case (3):
                    myFile = SD.open("60_Days_Ago.txt");
                    break;
                }
                // myGLCD.clrScr();

                if (myFile)
                {
                  myGLCD.fillScr (0, 255, 0);
                  myGLCD.setBackColor(0, 255, 0);
                  myGLCD.setColor(0, 0, 0);
                  myGLCD.setFont(BigFont);
                  myGLCD.print("Transfering", CENTER, 80);
                  myGLCD.print("Data...", CENTER, 110);
                  while (myFile.available())
                    Serial.write(myFile.read());
                  myFile.close();
                  goto Setup;
                }
                else
                {
                  Serial.println("error opening File or No Log is Available.");
                  myGLCD.fillScr(255, 255, 0);
                  myGLCD.setColor(0, 0, 0);
                  myGLCD.setBackColor(255, 255, 0);
                  myGLCD.print("LOG Not", CENTER, 80);
                  myGLCD.print("Available", CENTER, 110);
                  delay(2000);
                  goto Setup;
                }
              }
              else
              {
                Serial.println("error opening SD Card!!");
                myGLCD.fillScr(255, 0, 0);
                myGLCD.setColor(0, 0, 0);
                myGLCD.setBackColor(255, 0, 0);
                myGLCD.print("Error Opening", CENTER, 80);
                myGLCD.print("SD Card!!", CENTER, 110);
                delay(2000);
                goto Setup;
              }
            }

            if ((y >= 50) && (y <= 90))       //sound
            {
              waitForIt(5, 50, 315, 90);
              BeepFlag = !BeepFlag;
              myGLCD.setBackColor(0, 0, 255);
              myGLCD.setColor(255, 255, 255);
              if (BeepFlag)
                myGLCD.print("OFF", 155, 65);
              else
                myGLCD.print("ON ", 155, 65);
            }

            if ((y >= 95) && (y <= 135))      //CONTACTS
            {
              waitForIt(5, 95, 315, 135);
              myGLCD.clrScr();
              myGLCD.setColor(255, 255, 255);
              myGLCD.fillRoundRect (0, 0, 319, 239);
              myGLCD.setColor(255, 255, 255);
              myGLCD.drawRoundRect (0, 0, 319, 239);
              myGLCD.setFont(BigFont);
              myGLCD.setBackColor(255, 255, 255);
              myGLCD.setColor(0, 0, 0);
              myGLCD.print("INDIGO CARE", CENTER, 10);
              myGLCD.setFont(SmallFont);
              myGLCD.print("  Phone: +61 3 97632988", LEFT, 40);
              myGLCD.print("  Fax:   +61 3 97633811", LEFT, 60);
              myGLCD.print("  Email: SALES@INDIGOCARE.COM.AU", LEFT, 80);
              myGLCD.print("  WWW.INDIGOCARE.COM.AU", LEFT, 100);
              myGLCD.print("  ADDRESS: Unit 2/71 Rushdale St.", LEFT, 120);
              myGLCD.print("           Knoxfield, VIC 3180", LEFT, 140 );
              myGLCD.print("           Australia", LEFT, 160);
              myGLCD.print("  ABN: 36 007 302 801", LEFT, 180);
              myGLCD.print("  Software Version:  ", LEFT, 200);
              myGLCD.print(SWver, 200, 200);
              myGLCD.print("  Serial Number:  ", LEFT, 220);
              myGLCD.print(BoardSerial, 200, 220);
              while (true) {
                if (myTouch.dataAvailable())
                {
                  myTouch.read();
                  x = myTouch.getX();
                  y = myTouch.getY();
                  if ( (x > 0) && (y > 0) && (x < 319) && (y < 239))
                    goto Setup;
                }
              }
            }

            if ((y >= 140) && (y <= 180))     //RESERVED
              waitForIt(5, 140, 315, 180);
          }
          if ((y >= 185) && (y <= 235 ))              //NEXT AND RETURN
          {
            if ((x >= 160) && (x <= 315))               //NEXT
            {
              waitForIt(160, 185, 315, 235);
            }
            if ((x >= 5) && (x <= 155))             //RETURN
            {
              waitForIt(5, 185, 155, 235);
              if (PAGE > 1)
              {
                PAGE--;
                goto Setup;
              }
              else
              {
                drawLayer();
                return;
              }
            }
          }
        }
      }
    }
  }
}

/*****************************************CHECK RF*****************************/

void checkRf() {                            // Read Rf input buffer
  vw_rx_start();                            // Start the receiver PLL running
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  if (vw_get_message(buf, &buflen)) {       // Non-blocking
    int i;
    for (i = 0; i < buflen; i++) {          // Message with a good checksum received, dump it.
      CharMsg[i] = char(buf[i]);            // Fill CharMsg Char array with corresponding chars from buffer.
    }
    CharMsg[buflen] = '\0';                 // Null terminate the char array. Done to stop problems when the incoming messages has less digits than the one before.
    rfIDnum = atoi(CharMsg);                // Convert CharMsg Char array to integer
    for (int i = 0; i < buflen; i++) {      // Empty Rf buffer
      CharMsg[i] = 0;
    }
    vw_rx_stop();                             // Stop the receiver PLL running
    if ((rfIDnum >= 0 && rfIDnum <= 16383) || (rfIDnum >= 32768 && rfIDnum <= 65535)) {  // Check if its a Call, Mat or AUX number
      whichButton = true;                   // remember its a Call
    }
    else if (rfIDnum >= 16384 && rfIDnum <= 32767) { // Check if its a Bed-light number
      rfIDnum = rfIDnum & 16383;            // Clear MSB to convert back to Call ID number
      whichButton = false;                  // remember its a Bed-light
    }
    if (rfIDnum == id1 ||
        rfIDnum == id2 ||
        rfIDnum == id3 ||
        rfIDnum == id4 ||
        rfIDnum == id5 ) {                  // Check if any match rfIDnum
      switch (whichButton) {
        case true:
          if (mode == 1)
            MOM_Call();
          if (mode == 2)
            LATCH_Call();
          if (mode == 3)
            D_LATCH_Call();
          while (CancelFlag)
          {
            Cancel();
          }
          break;
        case false:
          digitalWrite(bliteOut, HIGH);
          delay(500);                       // 0.5sec wait
          digitalWrite(bliteOut, LOW);
          delay(250);                       // 0.25sec wait
          break;
      }
    }
    delay(500);                             // Wait so there is only one trigger.
  }
}


//////STORE RF ID

void storeID5() {                           // Stores ID no. in EEPROM for 5 locations
  memPointer = EEPROM.read(19);             // Read pointer from addr 19
  if (memPointer >= 6) {                    // Are memory locations full
    myGLCD.clrScr();
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print("Memory is Full", CENTER, 120);
    delay(5000);                            // OFF for 5 sec.
    EEPROM.write(19, memPointer);           // Store memPointer in addr 19
  }
  else if (memPointer == 1) {             // Loc. 1
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc1H, idHigh);          // Store high in addr 24
    EEPROM.write(loc1H + 1, idLow);       // Store low in addr 25
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19
  }
  else if (memPointer == 2) {             // Loc. 2
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc2H, idHigh);          // Store high in addr 26
    EEPROM.write(loc2H + 1, idLow);       // Store low in addr 27
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19
  }
  else if (memPointer == 3) {             // Loc. 3
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc3H, idHigh);          // Store high in addr 28
    EEPROM.write(loc3H + 1, idLow);       // Store low in addr 29
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19
  }
  else if (memPointer == 4) {             // Loc. 4
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc4H, idHigh);          // Store high in addr 30
    EEPROM.write(loc4H + 1, idLow);       // Store low in addr 31
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19
  }
  else if (memPointer == 5) {             // Loc. 5
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc5H, idHigh);          // Store high in addr 32
    EEPROM.write(loc5H + 1, idLow);       // Store low in addr 33
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19
  }
  myGLCD.clrScr();
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Device Stored", CENTER, 80);
  myGLCD.print("in Mememory ", CENTER, 120);
  myGLCD.printNumI((memPointer - 1), CENTER, 160);
  tone(beep, 1000, 2000);
  delay(3000);
}

//READ RF ID
void readID5() {                            // Reads ID no. x5 from EEPROM
  memPointer = EEPROM.read(19);             // Read pointer from addr 19
  idLow = EEPROM.read(loc1L);               // Read low from addr 25
  idHigh = EEPROM.read(loc1H);              // Read high from addr 24
  id1 = word(idHigh, idLow);                // convert to one word

  idLow = EEPROM.read(loc2L);               // Read low from addr 27
  idHigh = EEPROM.read(loc2H);              // Read high from addr 26
  id2 = word(idHigh, idLow);                // convert to one word

  idLow = EEPROM.read(loc3L);               // Read low from addr 29
  idHigh = EEPROM.read(loc3H);              // Read high from addr 28
  id3 = word(idHigh, idLow);                // convert to one word

  idLow = EEPROM.read(loc4L);               // Read low from addr 31
  idHigh = EEPROM.read(loc4H);              // Read high from addr 30
  id4 = word(idHigh, idLow);                // convert to one word

  idLow = EEPROM.read(loc5L);               // Read low from addr 33
  idHigh = EEPROM.read(loc5H);              // Read high from addr 32
  id5 = word(idHigh, idLow);                // convert to one word
}

//RESET RF ID
void resetID5()
{ // Resets ID no. in EEPROM
  rfIDnum = 0xFFFF;                       // Erase ID number
  idLow = lowByte(rfIDnum);                 // break up into 2 bytes
  idHigh = highByte(rfIDnum);

  EEPROM.write(loc1L, idLow);               // Store low in addr 25
  EEPROM.write(loc1H, idHigh);              // Store high in addr 24
  EEPROM.write(loc2L, idLow);               // Store low in addr 27
  EEPROM.write(loc2H, idHigh);              // Store high in addr 26
  EEPROM.write(loc3L, idLow);               // Store low in addr 29
  EEPROM.write(loc3H, idHigh);              // Store high in addr 28
  EEPROM.write(loc4L, idLow);               // Store low in addr 31
  EEPROM.write(loc4H, idHigh);              // Store high in addr 30
  EEPROM.write(loc5L, idLow);               // Store low in addr 33
  EEPROM.write(loc5H, idHigh);              // Store high in addr 32
  memPointer = 1;                           // reset mem pointer
  EEPROM.write(19, memPointer);             // Store memPointer in addr 19
}

/////////////////////////////////////////// Initial setup //////////////////////////////
void setup()
{
  pinMode(13, OUTPUT);                  // sets the digital pin as input
  digitalWrite(13, LOW);

  pinMode(rfData, INPUT);                   // sets the digital pin as input
  pinMode(ledIn, INPUT);                    // sets the digital pin as input
  pinMode(pendIn, INPUT);                   // sets the digital pin as input
  pinMode(extIn, INPUT);                    // sets the digital pin as input
  pinMode(poAlarm, INPUT);                  // sets the digital pin as input
  digitalWrite(ledIn, HIGH);

  //  pinMode(bliteOut, OUTPUT);                // sets the digital pin as output
  pinMode(cancelOut, OUTPUT);               // sets the digital pin as output
  pinMode(callOut, OUTPUT);                 // sets the digital pin as output
  pinMode(beep, OUTPUT);                    // sets the digital pin as output

  digitalWrite(callOut, LOW);                // Set callOut to Low - Idle
  digitalWrite(cancelOut, LOW);             // Set cancelOut to Low - Idle
  //  digitalWrite(bliteOut, LOW);              // Set bliteOut to Low - Idle

  // SW DATE AND Version
  if (EEPROM.read(21) != SWDAY)
    EEPROM.write(21, SWDAY);                       // F/w date - 18 day
  if (EEPROM.read(22) != SWMOTH)
    EEPROM.write(22, SWMOTH);                       // F/w date - 01 month
  if (EEPROM.read(23) != SWYR)
    EEPROM.write(23, SWYR);                       // F/w date - 2017 year
  //CANCEL PAGE SET PASSWORD
  SETPASS = EEPROM.read(100);

  //  TIMER INTERRUP ENABLE
  TCCR5A = 0;
  TCCR5B = 0;
  TCNT5  = 0;
  OCR5A = 63000;            // compare match register 16MHz/256/1Hz
  TCCR5B |= (1 << WGM12);   // CTC mode
  TCCR5B |= (1 << CS12);    // 256 prescaler
  TIMSK5 |= (1 << OCIE1A);  // enable timer compare interrupt


  //READ NAME AND OPERATING MODE
  mode = EEPROM.read(70);
  for (addr = 50; addr < 67; addr ++)
    master[addr - 50] = EEPROM.read(addr);

  for (x = 0; x < 17; x++)
    stName[x] = EEPROM.read(x);

  for (addr = 80; addr < 97; addr ++)
    ROOMNO[addr - 80] = EEPROM.read(addr);


  //EEPROM.write(16, OSCCAL);                 // Store original osccal in addr 16
  //  OSCCAL = EEPROM.read(17);                 // read new fron addr 17
  // if (EEPROM.read(18) != OSCCAL)
  //  EEPROM.write(18, OSCCAL);                 // write new osccal value to addr 18

  // VirtualWire - Initialise the IO and ISR
  vw_setup(2400);                           // Bits per sec
  vw_set_rx_pin(rfData);                    // Set RX pin to PC2(arduino pin 16).


  Serial.begin(115200);                  // for SDCARD reading

  switch (EEPROM.read(71)) {                // PAGE ORIENTATION
    case 0:
      myGLCD.InitLCD(PORTRAIT);
      break;
    case 1:
      myGLCD.InitLCD(LANDSCAPE);
      break;
  }
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setFont(BigFont);
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_HI);

  if (!SD.begin(SD_CS_PIN))
  {
    myGLCD.print("SD Card failed!", CENTER, 80);
    myGLCD.print("Log NOT", CENTER, 140);
    myGLCD.print("Availbe!", CENTER, 170);
    SDflag = false;
    delay(4000);
  }
  else
  {
    SDflag = true;
    myGLCD.print("SD Card Pass.", CENTER, 110);
    delay(1000);
  }

  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Select Mode", CENTER, 90);
  myGLCD.print("from", CENTER, 120);
  myGLCD.print("Setting Page", CENTER, 150);
  delay(2000);

  rtc.begin();
  rtc.setDOW(SUNDAY);     // Set Day-of-Week to SUNDAY

  if (!CLOCKFLAG)
  {
    TimePad();
    CLOCKFLAG = true;
  }

  BeepFlag = true;
  digitalWrite(beep, HIGH);
  delay(200);
  digitalWrite(beep, LOW);
  delay(200);
  digitalWrite(beep, HIGH);
  delay(100);
  digitalWrite(beep, LOW);
  readID5();
  drawLayer();
}

///////////////////////////////////////////////////////////////////////MAIN LOOP ///////////////////////////////////////////////////////////////
void loop()
{

  boolean pendStat;
  boolean extInStat;


  //myGLCD.print(master, CENTER, 30);     // uncomment if want to read EEProm password

  ShowTime();
  FileRemove();
  checkRf();
  extInStat = digitalRead(extIn);
  pendStat = digitalRead(pendIn);
  poAStat = digitalRead(poAlarm);

  if (poAStat)
  {
    switch (mode)
    {
      case 1:
        MOM_Call();
        break;
      case 2:
        LATCH_Call();
        break;
      case 3:
        D_LATCH_Call();
        break;
    }
    while (CancelFlag)
    {
      Cancel();
    }
  }
  else
  {
    digitalWrite(beep, LOW);
  }
  if (pendStat or extInStat)
  {
    switch (mode)
    {
      case 1:
        MOM_Call();
        break;
      case 2:
        LATCH_Call();
        break;
      case 3:
        dualPendCall();
        break;
    }
    while (CancelFlag)
    {
      Cancel();
    }
  }

  if (myTouch.dataAvailable())
  {
    myTouch.read();
    x = myTouch.getX();
    y = myTouch.getY();
    while ( myTouch.dataAvailable() && (CancelFlag == false) && (x >= 0) && (x <= 320) && (y >= 0) && (y <= 240)) // CALL Button pressed
    {
      cnt ++;
      delay(1000);
      hold = cnt;
      if (myTouch.dataAvailable() && ( hold > holdtime) )
      {
        while (myTouch.dataAvailable()) {
          switch (EEPROM.read(102))
          {
            case 1:
              {
                myGLCD.setBackColor(0, 255, 0);
                myGLCD.setColor(0, 0, 0);
                break;
              }
            case 2:
              {
                myGLCD.setBackColor(255, 255, 0);
                myGLCD.setColor(0, 0, 0);
                break;
              }
            case 3:
              {
                myGLCD.setBackColor(255, 0, 0);
                myGLCD.setColor(255, 255, 255);
                break;
              }
          }
          myGLCD.setFont(BigFont);
          if (EEPROM.read(71))
          {
            myGLCD.print("Release To Setup", LEFT, 160);
            digitalWrite(beep, HIGH);
            delay(1000);
            myGLCD.print("                    ", LEFT, 160);
            digitalWrite(beep, LOW);
            delay(1000);
          }
          else if (!EEPROM.read(71))
          {
            myGLCD.print("Release To", CENTER, 200);
            myGLCD.print("Setup", CENTER, 220);
            digitalWrite(beep, HIGH);
            delay(1000);
            myGLCD.print("          ", CENTER, 200);
            myGLCD.print("     ", CENTER, 220);
            digitalWrite(beep, LOW);
            delay(1000);
          }
        }
        cnt = 0;
        PassWord();
        if (PassWordFlag)
          SetupPage();
        else
        {
          drawLayer();
        }
      }
      if (!(myTouch.dataAvailable()) && (hold <= holdtime))
      {
        cnt = 0;
        if (mode == 1)
          MOM_Call();
        if (mode == 2)
          LATCH_Call();
        if (mode == 3)
          D_LATCH_Call();
        while (CancelFlag)
        {
          Cancel();
        }
      }
    }
  }
}

