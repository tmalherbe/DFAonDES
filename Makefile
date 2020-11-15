des_core:
	gcc -g -Wall des_core.c -c

DFAonDES:
	gcc -g -Wall DFAonDES.c -o DFAonDES des_core.o

all:	des_core DFAonDES 

clean:
	rm *~ DFAonDES des_core.o
