CFLAGS=-Wall -std=gnu99 -g
OBJS=main.o lex.o string.o

8ccc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

test:
	./test.sh

clean:
	rm -rf 8ccc *.o tmp.*
