CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -g
TARGET = aesdsocket

all: $(TARGET)

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c

clean:
	rm -f $(TARGET)