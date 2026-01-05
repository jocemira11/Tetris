SRC_DIR := src
BIN_DIR := bin

SFML := -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

TARGET := $(BIN_DIR)/tetris.exe

CPP := $(SRC_DIR)/tetris.cpp

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(BIN_DIR) $(CPP)
	g++ $(CPP) -o $(TARGET) $(SFML) -Iinclude -std=c++17

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
