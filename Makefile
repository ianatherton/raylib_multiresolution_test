CC = gcc
CFLAGS = -Wall -std=c99 -D_DEFAULT_SOURCE -Wno-missing-braces
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = .

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c) \
          $(wildcard $(SRC_DIR)/core/*.c) \
          $(wildcard $(SRC_DIR)/renderer/*.c) \
          $(wildcard $(SRC_DIR)/world/*.c) \
          $(wildcard $(SRC_DIR)/utils/*.c)

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Executable
EXECUTABLE = $(BIN_DIR)/dangerous_forest

# Default target
all: directories $(EXECUTABLE)

# Create build directories
directories:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/core
	mkdir -p $(BUILD_DIR)/renderer
	mkdir -p $(BUILD_DIR)/world
	mkdir -p $(BUILD_DIR)/utils
	mkdir -p $(BIN_DIR)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)

# Run the game
run: all
	$(EXECUTABLE)

.PHONY: all directories clean run
