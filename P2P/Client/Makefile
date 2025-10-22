# Makefile
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -lpthread -lm -Wno-unused-result
LDFLAGS = -lssl -lcrypto

TARGET = client

SOURCES = client.cpp clientsocket.cpp clientutils.cpp commandhandler.cpp FileTransferHandler.cpp

OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild