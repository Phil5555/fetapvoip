LD_FLAGS=-l wiringPi

fetapvoip: fetapvoip.o
	${CC} ${LD_FLAGS} -o $@ $< 

