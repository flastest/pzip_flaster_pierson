CC        = gcc
CC_STD    = -std=c11
CC_W      = -Wall -Wextra -Wpedantic
CC_GDB    = -ggdb3 -Og
CC_NODB   = $(CC_STD) $(CC_W) $(CC_GDB) -pthread
CC_DB     = $(CC_NODB) -DDEBUG
CC_SAN    = $(CC_DB) -fsanitize=address,leak,undefined
SRC       = pzip
EXE       = wzip
TESTS     = tests/1.in tests/4.in tests/5.in

.c.o:
	$(CC) $(CC_NODB) -c -O3 $<

all: pzip

pzip: pzip.o
	$(CC) $(CC_NODB) -o $(EXE) $(SRC).o

valgrind: all
	valgrind -s --leak-check=full --show-leak-kinds=all ./$(EXE)

%.o: %.c %.h
	$(CC) $(CC_NODB) -c -o $@ $<

clean:
	rm -fv *.o $(EXE)

test:
	$(CC) -o $(EXE) $(SRC).c $(CC_NODB)
	./test-wzip.sh

sanitize:
	$(CC) -o $(EXE) $(SRC).c $(CC_SAN)
	./$(EXE) $(TESTS)

grind:
	$(CC) -o $(EXE) $(SRC).c $(CC_DB)
	valgrind -s --track-origins=yes --leak-check=full --show-leak-kinds=all ./$(EXE) $(TESTS)

tidy:
	clang-tidy $(SRC).c

tidy-fix:
	clang-tidy --fix $(SRC).c

format:
	clang-format -style="{BasedOnStyle: google, IndentWidth: 4}" -i --verbose $(SRC).c

lint:
	# ignore C++ specific warnings
	cpplint --filter=-legal/copyright,-readability/todo $(SRC).c

warn:
	clang -Weverything -o $(EXE) $(SRC).c $(CC_SAN)
