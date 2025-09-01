# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -lpthread -lm -Wno-unused-result

# Target executable name
TARGET = tracker

# Source files for the tracker (modularized version)
SOURCES = tracker.cpp TrackerCommandHandler.cpp utils.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target - build tracker
all: $(TARGET)

# Build the tracker executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Rule to compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild everything from scratch
rebuild: clean all

# Phony targets (not actual files)
.PHONY: all clean rebuild