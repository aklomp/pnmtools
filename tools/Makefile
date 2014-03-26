CFLAGS += -std=c99 -Wall -Werror -pedantic -O3
LDFLAGS +=

.PHONY: clean

all: \
  pnmtoplainpnm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

pnmtoplainpnm: pnmtoplainpnm.o ../pnmreader/pnmreader.o ../pnmwriter/pnmwriter.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f \
	  *.o \
	  pnmtoplainpnm \
	  ../pnmreader/pnmreader.o \
	  ../pnmwriter/pnmwriter.o
