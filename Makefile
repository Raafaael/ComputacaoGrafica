
TARGET = build/simple_scene

.PHONY: all run clean build build-all help

all: $(TARGET)

UNAME_S := $(shell uname -s)

# Defaults
CXX ?= g++
CXXFLAGS ?= -g -Wall -std=c++17 -DGL_SILENCE_DEPRECATION
INC := -I. -Isrc -Ithird_party -Ithird_party/stb
LIB := -L.

# Windows settings
ifeq ($(OS),Windows_NT)
  # Adjust these paths to where you installed GLFW and GLM
  # Example if using vcpkg: set GLFW_DIR to the package lib/include location
  GLFW_DIR ?= C:/libs/glfw
  GLM_DIR ?= C:/libs/glm
  INC += -I$(GLFW_DIR)/include -I$(GLM_DIR)/include
  LIB += -L$(GLFW_DIR)/lib
  # Choose the appropriate import library name: glfw3 for static, glfw3dll for DLL import
  GLFW_LIB ?= glfw3
  LDLIBS := -l$(GLFW_LIB) -lopengl32 -lgdi32
endif

# Linux/WSL settings
ifeq ($(UNAME_S),Linux)
  # Requires: sudo apt install build-essential pkg-config libglfw3-dev libassimp-dev libglm-dev libglew-dev
  INC += $(shell pkg-config --cflags glfw3 2>/dev/null)
  LDLIBS := $(shell pkg-config --libs glfw3 2>/dev/null) -lGLEW -lGL -ldl -lpthread
endif

SRC = \
  src/arcball.cpp \
  src/camera3d.cpp \
  src/color.cpp \
  src/cube.cpp \
  src/disk.cpp \
  src/cylinder.cpp \
  src/error.cpp \
  src/image.cpp \
  src/light.cpp \
  src/material.cpp \
  src/cone.cpp \
  src/mesh.cpp \
  src/node.cpp \
  src/quad.cpp \
  src/polyoffset.cpp \
  src/lamp.cpp \
  src/table.cpp \
  src/scene.cpp \
  src/shader.cpp \
  src/sphere.cpp \
  src/state.cpp \
  src/grid.cpp \
  src/texture.cpp \
  src/transform.cpp \
  src/main_3d.cpp \

OBJ = $(patsubst src/%.cpp,build/%.o,$(SRC))

# Build objects into build/
build/%.o: src/%.cpp | build_dir
	@mkdir -p $(dir $@)
	$(CXX) $(INC) $(CXXFLAGS) -c $< -o $@ 

# Create build directory (separate target to avoid name clash with 'build' alias)
build_dir:
	mkdir -p build

$(TARGET): $(OBJ) Makefile 
	$(CXX) $(LIB) -o $@ $(OBJ) $(LDLIBS)

# Convenience target to build and run the demo
run: $(TARGET)
	./$(TARGET)

# Alias targets to build the app
build: $(TARGET)
build-all: $(TARGET)

# Show available targets
help:
	@echo Available targets:
	@echo   make\t\tBuild default target ($(TARGET))
	@echo   make run\tBuild and run
	@echo   make clean\tRemove build artifacts
	@echo   make build\tBuild the application (alias)
	@echo   make build-all\tBuild the application explicitly

clean:
	rm -rf build
