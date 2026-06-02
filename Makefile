CC      := gcc

ifeq ($(OS),Windows_NT)
    SANITIZE :=
else
    SANITIZE := -fsanitize=address
endif

CFLAGS  := -Wall -Wextra -O2 -g $(SANITIZE) $(shell pkg-config --cflags sdl3)
LDFLAGS := $(SANITIZE) $(shell pkg-config --libs sdl3)

TARGET  := main
SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe