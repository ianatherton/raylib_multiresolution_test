# Makefile for Raylib project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/usr/local/include -DPLATFORM_DESKTOP
LDFLAGS = -L/usr/local/lib -lraylib -lm -lpthread -ldl -lrt -lX11

# Source files
SRCS = main.c
# If you have other .c files in src/ that need to be compiled and linked:
# SRCS += $(wildcard src/**/*.c)

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = game

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

# Run rule
run: $(TARGET)
	./$(TARGET)
