CFLAGS += -std=c99 -Wall -Werror -pedantic -O3
LDFLAGS +=

.PHONY: clean

all: \
  pnmratio \
  pnmtoplainpnm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

pnmtoplainpnm: pnmtoplainpnm.o ../pnmreader/pnmreader.o ../pnmwriter/pnmwriter.o
	$(CC) $(LDFLAGS) -o $@ $^

pnmratio: pnmratio.o ../pnmreader/pnmreader.o ../pnmwriter/pnmwriter.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f \
	  *.o \
	  pnmratio \
	  pnmtoplainpnm \
	  ../pnmreader/pnmreader.o \
	  ../pnmwriter/pnmwriter.o
