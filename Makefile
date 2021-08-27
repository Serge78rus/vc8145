TARGET = vc8145
PREFIX = /usr/local/bin
SRCS = main.c vc8145.c options.c
OBJS = $(SRCS:.c=.o)
CFLAGS = -O2 -Wall

.PHONY: all clean install uninstall

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS)

.c.o:
	$(CC) $(CFLAGS)  -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJS)
install:
	install $(TARGET) $(PREFIX)
uninstall:
	rm -rf $(PREFIX)/$(TARGET)

