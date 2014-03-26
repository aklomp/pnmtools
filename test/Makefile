CFLAGS += -std=c99 -Wall -Werror -pedantic -O3
LDFLAGS +=

.PHONY: clean

all: \
  test \
  simplecopy \
  pnmcopy

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

test: test.o ../pnmreader/pnmreader.o
	$(CC) $(LDFLAGS) -o $@ $^

simplecopy: simplecopy.o ../pnmreader/pnmreader.o
	$(CC) $(LDFLAGS) -o $@ $^

pnmcopy: pnmcopy.o ../pnmreader/pnmreader.o ../pnmwriter/pnmwriter.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f \
	  *.o \
	  test simplecopy pnmcopy \
	  ../pnmreader/pnmreader.o \
	  ../pnmwriter/pnmwriter.o
