#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>
#include <unistd.h>
#include "globals.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


volatile int digit = 0;
volatile int digitLast=0;
volatile bool bDialing =0;
volatile unsigned char number[MAX_DIGIT_COUNT+1];
volatile unsigned int uLastDial;
unsigned int uLastCall;
volatile unsigned uNumberOffset;
volatile CallState *pState; // [0] contains Twinkle status, [1] is local status

// -------------------------------------------------------------------------

void acceptCall(void) {
  printf("Accepting incoming call\n");
  /*Disable loudspeaker*/
  pinMode(GPIO_SPEAKER, OUTPUT);
  digitalWrite(GPIO_SPEAKER, LOW);
  /*Mark local call as active*/
  pState[1]=CALL_ACTIVE;
}

void hangupCall(void) {
  printf("Hanging up call\n");
  /*End any ongoing/incoming call in twinkle:*/
  if( pState[0] >= CALL_PREPARATION || 
      pState[1] >= CALL_PREPARATION ) {
    printf("Asking twinkle to hangup");
    system("/usr/bin/twinkle --cmd bye");
  }
  /*Mark call as inactive in shmem*/
  pState[1]=CALL_INACTIVE;
}


// -------------------------------------------------------------------------

void itrDigit(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);

  //fprintf(stderr,"itrDigit %ld\n",t.tv_nsec);
  static unsigned long lastMilli;
  bDialing=!digitalRead(GPIO_DIAL); /*Ignore if not dialing*/
  if(!bDialing) return;
  if(lastMilli + 80 < millis())
  {
    lastMilli=millis();
    digit++;
  }
}

void itrDialingToggle(void) {
  //fprintf(stderr,"itrDialing\n");
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

void itrHookToggle(void) {
  bool bHook=digitalRead(GPIO_HOOK);
  printf("Hook: %d, state=%d\n" , bHook, *pState);

  if(bHook ) { /*Hook up*/
    /*Reset dialed number*/
    uNumberOffset=0;

    /*Check why the hook was picked up.*/
    if (pState[0]==CALL_PREPARATION) { /*phone is ringing*/
      acceptCall();
      /*Acept incoming call*/
      system("/usr/bin/twinkle --cmd answer");
    } else { /*User wants to phone out.*/
      printf("Preparing for outgoing call\n");
      pState[1]=CALL_PREPARATION; /*Block incoming calls to Twinkle while we dial*/
    }
  } else { /*Hook down*/
    hangupCall();
  }
}

// -------------------------------------------------------------------------

int main(void) {
  int shmId;
  char cTwinkleCmdBuf[MAX_CMD_LEN+1];

  bDialing=0;
  // sets up the wiringPi library
  if (wiringPiSetup () < 0) {
      fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
      return 1;
  }

  shmId=shmget(SHMEM_KEY, SHMEM_SIZE, IPC_CREAT | 0666 );
  if ( shmId != -1 ) {
    pState=(CallState *)shmat(shmId, NULL, 0);
    if(pState == (void *) -1) {
      fprintf (stderr, "Unable to bind shared variable: %s\n", strerror (errno));
      return 1;
    }
  } else {
    fprintf (stderr, "Unable to open shared memory: %s\n", strerror (errno));
    return 1;
  }

  // set Pin 17/0 generate an interrupt on high-to-low transitions
  // and attach myInterrupt() to the interrupt
  if ( wiringPiISR (GPIO_DIAL, INT_EDGE_BOTH, &itrDialingToggle) < 0 ) {
      fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
      return 1;
  }

  if ( wiringPiISR (GPIO_DIGIT, INT_EDGE_RISING, &itrDigit) < 0 ) {
      fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
      return 1;
  }

  if ( wiringPiISR (GPIO_HOOK, INT_EDGE_BOTH, &itrHookToggle) < 0 ) {
      fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno));
      return 1;
  }

  /*Disable loudspeaker*/
  pinMode(GPIO_SPEAKER, OUTPUT);
  digitalWrite(GPIO_SPEAKER, LOW);

  printf("Ohai. Let's twinkle.\n");

  nice(-20);
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
      if(pState[1] == CALL_PREPARATION) { /*Initiate outgoing call*/
        printf("Dialing: %s\n", number);
        pState[1] = CALL_ACTIVE;
        snprintf (cTwinkleCmdBuf, MAX_CMD_LEN+1, "/usr/bin/twinkle --immediate --cmd 'call %s'", number );
        system(cTwinkleCmdBuf);
      } else if(pState[1] == CALL_ACTIVE) { /*Send dialed number as inband information*/
        printf("Sending: %s\n", number);
        snprintf (cTwinkleCmdBuf, MAX_CMD_LEN+1, "/usr/bin/twinkle --cmd dtmf %s", number );
        system(cTwinkleCmdBuf);
      } else {
        printf("Test: %s\n", number);
      }

      uLastCall=uLastDial;
      uNumberOffset=0;
      uNO=0;
    }
  }
  return 0;
}
