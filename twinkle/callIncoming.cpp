#include "../globals.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//pState [0] contains Twinkle status, [1] is fetap aparatus status (hook state)

int main(int argc, char* argv[]) {
  int shmId;

  if (wiringPiSetup () < 0) {
      fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
      return 1;
  }

  shmId=shmget(SHMEM_KEY, SHMEM_SIZE, IPC_CREAT | 0666 );
  if ( shmId != -1 ) {
    CallState *pState;
    pState=(CallState *)shmat(shmId, NULL, 0);
    if(pState != (void *) -1) {
      /*We were able to accquire information from shared memory*/
      if(pState[0] < CALL_PREPARATION) {
        if ( pState[1] > CALL_INACTIVE ) {
          fprintf(stderr, "Another call is active. Drop incoming one\n");
          printf("action=reject\n");
          printf("reason=Other call ongoing\n");
        } else {
          fprintf(stderr, "Activating speaker");
          /*Enable loudspeaker and indicate incoming call*/
          pinMode(GPIO_SPEAKER, OUTPUT);
          digitalWrite(GPIO_SPEAKER, HIGH);
          pState[0]=CALL_PREPARATION;
        }
      }
      shmdt(pState);
      pState=NULL;
    } else {
      perror("Failed to fetch memory\n");
    }
  } else {
    perror("Failed to accquire shared memory\n");
  }
  return 0;
}

/* CALL__INACTIVE=0,
   CALL_RINGING=1,
   CALL_ACTIVE
*/
