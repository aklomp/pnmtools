CFLAGS += -std=c99 -Wall -Werror -pedantic -O3
LDFLAGS +=

.PHONY: all clean

PROG = \
  test-reader \
  imgsize \
  simplecopy \
  pnmcopy

all: $(PROG)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

test-reader: test-reader.o ../pnmreader/pnmreader.o
	$(CC) $(LDFLAGS) -o $@ $^

imgsize: imgsize.o ../pnmreader/pnmreader.o
	$(CC) $(LDFLAGS) -o $@ $^

simplecopy: simplecopy.o ../pnmreader/pnmreader.o
	$(CC) $(LDFLAGS) -o $@ $^

pnmcopy: pnmcopy.o ../pnmreader/pnmreader.o ../pnmwriter/pnmwriter.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f \
	  *.o \
	  $(PROG) \
	  ../pnmreader/pnmreader.o \
	  ../pnmwriter/pnmwriter.o
