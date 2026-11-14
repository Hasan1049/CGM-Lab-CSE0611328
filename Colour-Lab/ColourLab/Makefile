# Makefile for Colour Lab (MinGW g++ on Windows)
# Just type "make" in the terminal: it builds the game and runs it.
# Type "make clean" to delete the build files.

CXX = g++
CC = gcc
CXXFLAGS = -Wall -Iinclude
LDFLAGS = -Llib -lglfw3 -lgdi32 -lopengl32 -static-libgcc -static-libstdc++

TARGET = build/colourlab.exe
OBJS = build/main.o build/glad.o

ifeq ($(OS),Windows_NT)
    CLEAN = del /Q build\*.o build\colourlab.exe
else
    CLEAN = rm -f build/*.o build/colourlab.exe
endif

run: $(TARGET)
	"$(CURDIR)/$(TARGET)"

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

build/main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o build/main.o

build/glad.o: src/glad.c
	$(CC) -Iinclude -c src/glad.c -o build/glad.o

clean:
	-$(CLEAN)

.PHONY: run clean
