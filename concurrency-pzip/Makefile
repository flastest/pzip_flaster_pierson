CXX = clang
CXXFLAGS = -g -pthread -O3 -Wall -Wextra -pedantic -Werror

.c.o:
	$(CXX) $(CXXFLAGS) -c -O3 $<

all: pzip

pzip: pzip.o
	$(CXX) $(CXXFLAGS) -o pzip pzip.o

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all ./pzip

%.o: %.c %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o
	
