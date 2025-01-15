CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS = -ljson-c -lpcap -lmicrohttpd

SRC = src/main.c src/traffic_shaper.c src/config.c src/network_monitor.c
OBJ = obj/main.o obj/traffic_shaper.o obj/config.o obj/network_monitor.o
TARGET = traffic_shaper

all: $(TARGET)

obj:
	mkdir -p obj

obj/main.o: src/main.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/traffic_shaper.o: src/traffic_shaper.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/config.o: src/config.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/network_monitor.o: src/network_monitor.c | obj
	$(CC) $(CFLAGS) -c $< -o $@


$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)
