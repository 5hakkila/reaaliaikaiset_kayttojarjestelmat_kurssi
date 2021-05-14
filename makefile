CC = gcc
CFLAGS = -g -Wall -pthread `mysql_config --cflags --libs`

project: main.o bcm2835.o
	$(CC) -o project main.o bcm2835.o $(CFLAGS)

main.o: main.c 
	gcc -c main.c $(CFLAGS)

bcm2835.o: lib/bcm2835.c
	gcc -c lib/bcm2835.c $(CFLAGS)

clean:
	rm -f *.o