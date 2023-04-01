CC = gcc
CFLAGS=
LDFLAGS=


SOURCES=writer.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=writer

CROSS_COMPILE=
     
writer: $(SOURCES)  $(OBJECTS)

$(EXECUTABLE): $(OBJECTS)
	$(CROSS_COMPILE)$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJECTS): $(SOURCES)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $<

#writer.o: writer.c
#	$(PREFIX)$(CC) $(CFLAGS) $(SOURCES)


clean:
	rm writer writer.o