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
      if(*pState > CALL_INACTIVE) {
        fprintf(stderr, "Deactivating speaker");
        /*Disable loudspeaker and indicate inactivity*/
        pinMode(GPIO_SPEAKER, OUTPUT);
        digitalWrite(GPIO_SPEAKER, LOW);
        pState[0]=CALL_INACTIVE;
      }
      shmdt(pState);
      pState=NULL;
    } else {
      perror("Failed to fetch memory");
    }
  } else {
    perror("Failed to accquire shared memory");
  }
  return 0;
}
