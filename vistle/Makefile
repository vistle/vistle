CC=mpic++

CXXFLAGS=-g -Wall -DHAVE_MPI
LDFLAGS=-lrt

.SUFFIXES:

all: communicator module

communicator.o: communicator.cpp communicator.h
	$(CC) $(CXXFLAGS) -c $< -o $@

module.o: module.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

message.o: message.cpp message.h
	$(CC) $(CXXFLAGS) -c $< -o $@

module: module.o
	$(CC) $(LDFLAGS) $^ -o $@

communicator: communicator.o message.o
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f communicator.o module.o message.o communicator module *~