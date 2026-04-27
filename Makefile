CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = 

# Source files
SOURCES = maro_malloc.c
TEST_SOURCES = test_maro_malloc.c munit.c
OBJECTS = $(SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Targets
TEST_EXECUTABLE = test_maro_malloc

.PHONY: all test clean

all: $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(OBJECTS) $(TEST_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_EXECUTABLE)
	./$(TEST_EXECUTABLE)

clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(TEST_EXECUTABLE)
