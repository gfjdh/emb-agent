# Test Application Makefile
# Compile: gcc -o test_led test_led.c

CC := gcc
CFLAGS := -Wall -O2
TARGET := test_led

all: $(TARGET)

$(TARGET): test_led.c
	$(CC) $(CFLAGS) -o $(TARGET) test_led.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
