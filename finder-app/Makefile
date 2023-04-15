CC = gcc
CFLAGS=
LDFLAGS=


SOURCES=writer.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=writer

CROSS_COMPILE=
     
all: $(SOURCES)  $(OBJECTS)

$(EXECUTABLE): $(OBJECTS)
	$(CROSS_COMPILE)$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJECTS): $(SOURCES)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $<


clean:
	rm -f writer writer.o