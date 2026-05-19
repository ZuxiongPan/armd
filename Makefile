CC := gcc

CFLAGS := -Wall -O2 -I./include -std=c11
LDFLAGS := 

OBJS-y := core/armd_event_loop.o \
	init/main.o

TARGET := armd

.PHONY: all clean sendcmd

all: $(TARGET)

$(TARGET): $(OBJS-y)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

sendcmd:
	$(CC) $(CFLAGS) $(LDFLAGS) -o sendcmd debug/main.c

clean:
	rm -f $(OBJS-y) $(TARGET) sendcmd

