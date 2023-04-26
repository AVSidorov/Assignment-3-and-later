# TODO
# modify for Yocto
# exmp CC ?= $(CROSS_COMPILE)$(CC)
CC = gcc
CFLAGS= -Wall -Werror
LDFLAGS= -lpthread -lrt
CROSS_COMPILE=
INCLUDE=


TARGET=aesdsocket
SOURCES=aesdsocket.c
OBJECTS=$(SOURCES:.c=.o)


all: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CROSS_COMPILE)$(CC) $(INCLUDE) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJECTS): $(SOURCES)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $<


clean:
	rm -f $(TARGET) $(OBJECTS)