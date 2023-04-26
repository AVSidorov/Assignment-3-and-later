# TODO
# modify for Yocto
# exmp CC ?= $(CROSS_COMPILE)$(CC)
CC = gcc
CFLAGS= -Wall -Werror
LDFLAGS=
CROSS_COMPILE=
INCLUDE= -lpthread -lrt


TARGET=aesdsocket
SOURCES=aesdsocket.c
OBJECTS=$(SOURCES:.c=.o)


all: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CROSS_COMPILE)$(CC) $(LDFLAGS) $(INCLUDE) $(OBJECTS) -o $@

$(OBJECTS): $(SOURCES)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $<


clean:
	rm -f $(TARGET) $(OBJECTS)