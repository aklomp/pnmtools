CFLAGS += -std=c99 -Wall -Werror -pedantic -O3
LDFLAGS +=

.PHONY: test clean

PROG = \
  test-reader \
  imgsize \
  simplecopy \
  pnmcopy

# This phony target makes and runs all tests:
test: clean test-reader
	./test-reader

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
