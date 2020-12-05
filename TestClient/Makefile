INPUT1 = Asst3
INPUT2 = client
OUTPUT=client
CFLAGS=-g -pthread -Wall
LFLAGS=-lm

%: %.c %.h
	gcc $(CFLAGS) -o $@ $< $(LFLAGS)

%: %.c
	gcc $(CFLAGS) -o $(OUTPUT) $< $(LFLAGS)

all: $(INPUT2)

clean:
	rm -f *.o $(INPUT2)