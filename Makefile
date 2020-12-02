INPUT1 = Asst3
INPUT2 = client
OUTPUT=KKJserver
CFLAGS=-g -pthread -Wall
LFLAGS=-lm

%: %.c %.h
	gcc $(CFLAGS) -o $@ $< $(LFLAGS)

%: %.c
	gcc $(CFLAGS) -o $(OUTPUT) $< $(LFLAGS)

all: $(INPUT1)

clean:
	rm -f *.o $(OUTPUT)