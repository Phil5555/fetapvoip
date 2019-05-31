#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>

// Use GPIO Pin 17, which is Pin 0 for wiringPi library
/*GPIO 27 is wiringPi 2*/
#define GPIO_DIAL 2

/*GPIO 4 is wiringPi 7:*/
#define GPIO_HOOK 7

/*GPIO 17 is wiringPi 0*/
#define GPIO_DIGIT  0

#define MAX_DIGIT_COUNT 20
#define SECONDS_BEFORE_DIAL 3

volatile int digit = 0;
volatile int digitLast=0;
volatile bool bDialing =0;
volatile unsigned char number[MAX_DIGIT_COUNT+1];
volatile unsigned int uLastDial;
unsigned int uLastCall;
volatile unsigned uNumberOffset;

// -------------------------------------------------------------------------

void itrDigit(void) {
  static unsigned long lastMilli;
  bDialing=!digitalRead(GPIO_DIAL); /*Ignore if not dialing*/
  if(!bDialing) return;
  if(lastMilli + 70 < millis())
  {
    lastMilli=millis();
    digit++;
  }
}

void itrDialingToggle(void) {
  static unsigned long lastMilli;
  bDialing=!digitalRead(GPIO_DIAL);
  if(bDialing) {
    digit=0;
  } else {
    if(uNumberOffset<MAX_DIGIT_COUNT) {
      number[uNumberOffset+1]=0;
      if(digit==10) {
        number[uNumberOffset]='0';
        ++uNumberOffset;
      } else if (digit<10 && digit > 0) {
        number[uNumberOffset]=digit+'0';
        ++uNumberOffset;
      }
      uLastDial=time(NULL);
      digit=0;
    }
  }
}

// -------------------------------------------------------------------------

int main(void) {
  bDialing=0;
  // sets up the wiringPi library
  if (wiringPiSetup () < 0) {
      fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
      return 1;
  }

  // set Pin 17/0 generate an interrupt on high-to-low transitions
  // and attach myInterrupt() to the interrupt
  if ( wiringPiISR (GPIO_DIAL, INT_EDGE_BOTH, &itrDialingToggle) < 0 ) {
      fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
      return 1;
  }

  if ( wiringPiISR (GPIO_DIGIT, INT_EDGE_FALLING, &itrDigit) < 0 ) {
      fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
      return 1;
  }

  unsigned uNO=0;
  // display counter value every second.
  while ( 1 ) {
    if(uNO<uNumberOffset)
    {
      uNO=uNumberOffset;
      printf( "\rPreparing Number: %s", number);
      fflush(stdout);
      delay( 300 ); // wait 1 second
    }
    if (uLastCall<uLastDial &&
        uLastDial + SECONDS_BEFORE_DIAL < time(NULL) &&
        uNumberOffset)
    {
      printf("\nDialing: %s\n", number);
      uLastCall=uLastDial;

      uNumberOffset=0;
      uNO=0;
    }
  }
  return 0;
}
