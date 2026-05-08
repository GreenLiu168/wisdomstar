# Raspberry Pi Camera Info Display - Makefile

# 編譯器設定
CC = gcc
CXX = g++

# 編譯選項
CFLAGS = -Wall -Wextra -O2
CXXFLAGS = -Wall -Wextra -O2

# 目標文件
TARGET = camera_info

# 源文件
SRC = camera_info.c

# 使用 pkg-config 獲取 OpenCV 和 ncurses 的編譯選項
OPENCV_CFLAGS := $(shell pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv)
OPENCV_LIBS := $(shell pkg-config --libs opencv4 2>/dev/null || pkg-config --libs opencv)
NCURSES_LIBS = -lncurses

# 最終編譯命令 (需要 C++ 鏈接器因為使用了 OpenCV C++ API)
LDFLAGS = $(OPENCV_LIBS) $(NCURSES_LIBS) -lstdc++

.PHONY: all clean install-deps help

all: $(TARGET)

$(TARGET): $(SRC)
	@echo "Compiling $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "Clean complete."

install-deps:
	@echo "Installing required dependencies..."
	sudo apt-get update
	sudo apt-get install -y libopencv-dev libncurses5-dev pkg-config
	@echo "Dependencies installed."

help:
	@echo "Raspberry Pi Camera Info Display - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build the camera_info executable (default)"
	@echo "  clean         - Remove built files"
	@echo "  install-deps  - Install required system dependencies"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make                  - Build the program"
	@echo "  make clean            - Clean build artifacts"
	@echo "  make install-deps     - Install dependencies (requires sudo)"
	@echo ""
	@echo "After building, run with:"
	@echo "  ./camera_info"
	@echo ""
	@echo "Note: This program requires a Raspberry Pi with a connected camera."
