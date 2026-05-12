//Jayson Viles

#include "RTClib.h"
RTC_DS1307 rtc;

#define RDA 0x80
#define TBE 0x20  
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

unsigned char* ddr_b = (unsigned char*) 0x24;
unsigned char* port_b = (unsigned char*) 0x25;

unsigned char* ddr_e = (unsigned char*) 0x2D;
unsigned char* port_e = (unsigned char*) 0x2E;
unsigned char* pin_e = (unsigned char*) 0x2C;

unsigned char* ddr_h = (unsigned char*) 0x101;
unsigned char* port_h = (unsigned char*) 0x102;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

volatile bool wait;
volatile bool systemOn;
int waterLevel;
unsigned long timeOfFirstRecording;
volatile unsigned long timeOfLastBounce;

void setup() 
  {
  systemOn = 0;
  rtc.begin();
  rtc.adjust(DateTime(2026, 5, 11, 12, 0, 0));
  U0init(9600);
  *ddr_b |= 0x01 << 7;
  //pinMode(13, OUTPUT);
  *ddr_b |= 0x01 << 6;
  //pinMode(12, OUTPUT);
  *ddr_b |= 0x01 << 5;
  //pinMode(11, OUTPUT);
  *ddr_b |= 0x01 << 4;
  //pinMode(10, OUTPUT);
  *ddr_h |= 0x01 << 6;
  //pinMode(9, OUTPUT);
  *ddr_h |= 0x01 << 5;
  //pinMode(8, OUTPUT);
  *ddr_h |= 0x01 << 4;
  //pinMode(7, OUTPUT);
  *ddr_e &= ~(0x01 << 5);
  *port_e |= 0x01 << 5;
  //pinMode(3, INPUT_PULLUP);
  *ddr_e &= ~(0x01 << 4);
  *port_e |= 0x01 << 4;
  //pinMode(2, INPUT_PULLUP);
  wait = 0;
  waterLevel = 0;
  timeOfFirstRecording = millis();
  timeOfLastBounce = 0;
  attachInterrupt(digitalPinToInterrupt(3), reset, FALLING);
  attachInterrupt(digitalPinToInterrupt(2), offOn, FALLING);
  }

void loop() 
  {
  if (systemOn == 0)
    {
    adjustLightsAndFan(1);
    }
  else
    {
    if (waterLevel == readSensor())
      {
      if (millis()-timeOfFirstRecording > 30000)
        {
        wait = 1;
        U0putstring("Water level has not changed for over five minutes - an error is assumed.");
        timeOfFirstRecording = 0;
        U0putchar('\n');
        adjustLightsAndFan(4);
        while (wait == 1)
          {
          }
        }
      }
    else
      {
      waterLevel = readSensor();
      timeOfFirstRecording = millis();
      }
    if (waterLevel < 100)
      {
      printDate();
      U0putstring("Water level: ");
      U0putint(waterLevel);
      U0putstring(". Below expected threshold.");
      U0putchar('\n');
      //Serial.print("Water level: ");
      //Serial.print(waterLevel);
      //Serial.println(". Below expected threshold.");
      adjustLightsAndFan(3);
      }
    else
      {
      printDate();
      U0putstring("Water level: ");
      U0putint(waterLevel);
      U0putchar('\n');
      //Serial.print("Water level: ");
      //Serial.println(waterLevel);
      adjustLightsAndFan(2);
      }
    unsigned long minuteStart = millis();
    while (((millis() - minuteStart) < 60000) && systemOn == 1)
      {
      }
    }
  }

void adjustLightsAndFan(int choice)
  {
  switch (choice)
    {
    case 1:
      *port_b |= (0x01 << 7);
      //digitalWrite(13, 1);
      *port_b &= ~(0x01 << 6);
      //digitalWrite(12, 0);
      *port_b &= ~(0x01 << 5);
      //digitalWrite(11, 0);
      *port_b &= ~(0x01 << 4);
      //digitalWrite(10, 0);
      *port_h &= ~(0x01 << 5);
      //digitalWrite(8, 0);
      *port_h &= ~(0x01 << 4);
      //digitalWrite(7, 0);
      break;
    case 2:
      *port_b &= ~(0x01 << 7);
      *port_b |= (0x01 << 6);
      *port_b &= ~(0x01 << 5);
      *port_b &= ~(0x01 << 4);
      *port_h &= ~(0x01 << 5);
      *port_h &= ~(0x01 << 4);
      /*digitalWrite(13, 0);
      digitalWrite(12, 1);
      digitalWrite(11, 0);
      digitalWrite(10, 0);
      digitalWrite(8, 0);
      digitalWrite(7, 0);*/
      break;
    case 3:
      *port_b &= ~(0x01 << 7);
      *port_b &= ~(0x01 << 6);
      *port_b |= (0x01 << 5);
      *port_b &= ~(0x01 << 4);
      *port_h |= (0x01 << 5);
      *port_h &= ~(0x01 << 4);
      /*digitalWrite(13, 0);
      digitalWrite(12, 0);
      digitalWrite(11, 1);
      digitalWrite(10, 0);
      digitalWrite(8, 1);
      digitalWrite(7, 0);*/
      break;
    case 4:
      *port_b &= ~(0x01 << 7);
      *port_b &= ~(0x01 << 6);
      *port_b &= ~(0x01 << 5);
      *port_b |= (0x01 << 4);
      *port_h &= ~(0x01 << 5);
      *port_h &= ~(0x01 << 4);
      /*digitalWrite(13, 0);
      digitalWrite(12, 0);
      digitalWrite(11, 0);
      digitalWrite(10, 1);
      digitalWrite(8, 0);
      digitalWrite(7, 0);*/
      break;
    }
  }

int readSensor()
  {
  int val;
  *port_h |= (0x01 << 6);
  //digitalWrite(9, HIGH);
  delay(10);
  val = analogRead(A3);
  *port_h &= ~(0x01 << 6);
  //digitalWrite(9, LOW);
  return val;             
  }

void printDate()
  {
  DateTime now = rtc.now();
  U0putint(now.year());
  U0putchar('/');
  U0putint(now.month());
  U0putchar('/');
  U0putint(now.day());
  U0putchar(' ');
  U0putchar('(');
  U0putstring(daysOfTheWeek[now.dayOfTheWeek()]);
  U0putchar(')');
  U0putchar(' ');
  U0putint(now.hour());
  U0putchar(':');
  U0putint(now.minute());
  U0putchar(':');
  U0putint(now.second());
  U0putchar(' ');
  /*Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print(' ');*/
  }

void offOn()
  {
  if (millis() - timeOfLastBounce > 1000)
    {
    timeOfLastBounce = millis();
    if (systemOn == 0)
      {
      systemOn = 1;
      }
    else if (systemOn == 1)
      {
      systemOn = 0;
      }
    }
  }

void reset()
  {
  if (millis() - timeOfLastBounce > 1000)
    {
    timeOfLastBounce = millis();
    wait = 0;
    }
  }

void U0init(unsigned long U0baud)
  {
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0  = tbaud;
  }

void U0putchar(unsigned char U0pdata)
  { 
  while (!(*myUCSR0A & TBE))
    {
    }
  *myUDR0 = U0pdata;
  }

void U0putstring(String printString)
  {
  for (int index = 0; index < printString.length(); index++)
    {
    U0putchar(printString[index]);
    }
  }

void U0putint(unsigned int integer)
  {
  if (integer >= 10)
    {
    U0putint(integer/10);
    }
  unsigned int target = integer%10;
  switch (target)
    {
    case 0:
      U0putchar('0');
      break;
    case 1:
      U0putchar('1');
      break;
    case 2:
      U0putchar('2');
      break;
    case 3:
      U0putchar('3');
      break;
    case 4:
      U0putchar('4');
      break;
    case 5:
      U0putchar('5');
      break;
    case 6:
      U0putchar('6');
      break;
    case 7:
      U0putchar('7');
      break;
    case 8:
      U0putchar('8');
      break;
    case 9:
      U0putchar('9');
      break;
    }
  }