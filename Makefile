EXEC    = logtrunc
CFLAGS  = -W -Wall -O2 -g
LDFLAGS = 

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

release: CFLAGS += -DRELEASE -O2
release: clean $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -fv *.o

mrproper: clean
	rm -fv $(EXEC)
