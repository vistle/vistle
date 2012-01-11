CC=mpic++

CXXFLAGS=-g -Wall -DHAVE_MPI
LDFLAGS=

.SUFFIXES:

all: communicator2 module

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

%: %.o
	$(CC) $(LDFLAGS) $< -o $@

clean:
	rm -f communicator.o communicator2.o module.o communicator communicator2 module *~