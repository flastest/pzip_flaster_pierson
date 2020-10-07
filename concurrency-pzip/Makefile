CC        = gcc
CC_STD    = -std=c11
CC_W      = -Wall -Wextra -Wpedantic
CC_DB     = -ggdb3 -Og -DDEBUG
CC_FLAGS  = -pthread $(CC_STD) $(CC_W) $(CC_DB)
CC_SAN    = $(CXX_NOSAN) -fsanitize=address,leak,undefined
SRC       = pzip
EXE       = wzip

.c.o:
	$(CC) $(CC_FLAGS) -c -O3 $<

all: pzip

pzip: pzip.o
	$(CC) $(CC_FLAGS) -o $(EXE) $(SRC).o

valgrind: all
	valgrind -s --leak-check=full --show-leak-kinds=all ./$(EXE)

%.o: %.c %.h
	$(CC) $(CC_FLAGS) -c -o $@ $<

clean:
	rm -fv *.o $(EXE)

test:
	$(CC) -o $(EXE) $(SRC).c $(CC_FLAGS)
	./test-wzip

sanitize:
	$(CC) -o $(EXE) $(SRC).c $(CC_SAN)

grind:
	$(CC) -o $(EXE) $(SRC).c $(CC_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all ./$(EXE)

tidy:
	clang-tidy $(SRC).c

tidy-fix:
	clang-tidy --fix $(SRC).c

format:
	clang-format -style="{BasedOnStyle: google, IndentWidth: 4}" -i --verbose $(SRC).c

lint:
	# ignore C++ specific warnings
	cpplint --filter=-legal/copyright $(SRC).c

warn:
	clang -Weverything -o $(EXE) $(SRC).c $(CC_FLAGS)