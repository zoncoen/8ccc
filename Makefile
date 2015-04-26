CFLAGS=-Wall -std=gnu99

8ccc: 8ccc.o

test:
	./test.sh

clean:
	rm -f 8ccc *.o tmp.*
