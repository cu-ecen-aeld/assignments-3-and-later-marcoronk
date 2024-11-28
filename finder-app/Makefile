CROSS_COMPILE ?=

CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -Wextra -std=c11

TARGET = writer

SRC = writer.c

OBJ = writer.o

all: $(TARGET)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

distclean: clean
	rm -f $(TARGET)

.PHONY: all clean distclean

