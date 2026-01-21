# -----------------------------
# Project settings
# -----------------------------
CC       := gcc
CFLAGS   := -Wall -Wextra -Wno-unused-parameter -O2
DBGFLAGS := -Wall -Wextra -Wno-unused-parameter -g -O0
LDFLAGS  :=

SRC_DIR  := src
OUT_DIR  := bin
TARGET   := $(OUT_DIR)/galo
DEBUG_TARGET := $(OUT_DIR)/galo_debug

# -----------------------------
# Automatic source discovery
# -----------------------------
SOURCES := $(wildcard $(SRC_DIR)/*.c)

OBJECTS       := $(patsubst $(SRC_DIR)/%.c,$(OUT_DIR)/%.o,$(SOURCES))
DEBUG_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OUT_DIR)/%.debug.o,$(SOURCES))

# -----------------------------
# Default targets
# -----------------------------
all: $(TARGET)

debug: $(DEBUG_TARGET)

# -----------------------------
# Link
# -----------------------------
$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(DEBUG_TARGET): $(DEBUG_OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

# -----------------------------
# Compile rules
# -----------------------------
$(OUT_DIR)/%.o: $(SRC_DIR)/%.c | $(OUT_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR)/%.debug.o: $(SRC_DIR)/%.c | $(OUT_DIR)
	$(CC) $(DBGFLAGS) -c $< -o $@

# -----------------------------
# Create output directory
# -----------------------------
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# -----------------------------
# Utility targets
# -----------------------------
clean:
	rm -rf $(OUT_DIR)

run: $(TARGET)
	./$(TARGET) $(filter-out $@,$(MAKECMDGOALS))

run-debug: $(DEBUG_TARGET)
	gdb ./$(DEBUG_TARGET)

.PHONY: all debug clean run run-debug
