SRC = $(wildcard src/*.cpp)

.PHONY: all verify clean

all: clean sim

sim: $(SRC)
	g++ -g -O2 $^ -o $@

clean:
	rm -rf *.o *~ sim

