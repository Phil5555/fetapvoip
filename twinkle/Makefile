LD_FLAGS=-l wiringPi


all: callIncoming callInactive callActive

callIncoming: callIncoming.o
	${CC} ${LD_FLAGS} -o $@ $< 

callInactive: callInactive.o
	${CC} ${LD_FLAGS} -o $@ $<

callActive: callActive.o
	${CC} ${LD_FLAGS} -o $@ $<
