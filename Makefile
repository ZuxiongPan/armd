CC ?= gcc

CFLAGS := -Wall -O2 -I./include -I./3rd_party -std=c11 -D_GNU_SOURCE
LDFLAGS := 

OBJS-y := core/armd_event_loop.o \
	core/armd_timer.o \
	core/armd_unix_socket.o \
	core/armd_uevent.o \
	modules/block_mgr.o \
	modules/ver_mgr.o \
	3rd_party/cJSON/cJSON.o \
	init/main.o

TARGET := armd

.PHONY: all clean sendcmd

all: $(TARGET)

$(TARGET): $(OBJS-y)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

sendcmd:
	$(CC) $(CFLAGS) $(LDFLAGS) -o sendcmd debug/main.c 3rd_party/cJSON/cJSON.c

clean:
	rm -f $(OBJS-y) $(TARGET) sendcmd

