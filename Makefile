LD_FLAGS=-l wiringPi

main: main.o
	${CC} ${LD_FLAGS} -o $@ $< 
