# all:
# 	g++ -w *.c *.cpp -lglpk -lm -o run


# Paths
INCLUDE_PATH = /opt/local/include
LIBRARY_PATH = /opt/local/lib

# Compiler
CXX = clang++

# Compiler flags
CXXFLAGS = -w -I$(INCLUDE_PATH)
CXXFLAGS += --std=c++17 -Wall -Werror -pedantic -g # -fsanitize=address -fsanitize=undefined
LDFLAGS = -L$(LIBRARY_PATH) -lglpk -lm

# Target executable
TARGET = run

# Build all
all:
	$(CXX) $(CXXFLAGS) *.cpp other/*.c other/*.cpp $(LDFLAGS) -Ofast -o $(TARGET)

# Build with Valgrind
valgrind:
	$(CXX) $(CXXFLAGS) *.cpp other/*.c other/*.cpp $(LDFLAGS) -g -O0 -o $(TARGET)

# Build with GDB
gdb:
	$(CXX) -g $(CXXFLAGS) *.cpp other/*.c other/*.cpp $(LDFLAGS) -o $(TARGET)

# Clean up
clean:
	rm -f $(TARGET)

.PHONY: clean