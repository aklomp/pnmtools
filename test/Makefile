CFLAGS += -std=c99 -Wall -Werror -pedantic -O3
LDFLAGS +=

.PHONY: all analyze test clean

PROG = \
  test-reader \
  imgsize \
  simplecopy \
  pnmcopy

# This phony target makes and runs all tests:
test: clean test-reader
	./test-reader

# This target is called recursively by `make analyze`:
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

analyze: clean
	scan-build --use-analyzer=`which clang` --status-bugs make all

clean:
	rm -f \
	  *.o \
	  $(PROG) \
	  ../pnmreader/pnmreader.o \
	  ../pnmwriter/pnmwriter.o
