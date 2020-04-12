// Use GPIO Pin 17, which is Pin 0 for wiringPi library
/*GPIO 27 is wiringPi 2*/
#define GPIO_DIAL             2

/*GPIO 4 is wiringPi 7:*/
#define GPIO_HOOK             7

/*GPIO 17 is wiringPi 0*/
#define GPIO_DIGIT            0

/*GPIO 23 is wiringPi 4 */
#define GPIO_SPEAKER          4

#define MAX_CMD_LEN         500
#define MAX_DIGIT_COUNT      20
#define SECONDS_BEFORE_DIAL   3


#define SHMEM_KEY  8328
#define SHMEM_SIZE (2*sizeof(CallState))

enum CallState {
  CALL_UNINITIALIZED=0,
  CALL_INACTIVE=1,
  CALL_PREPARATION=2,
  CALL_ACTIVE=3,


  CALL_MAX
};
