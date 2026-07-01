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
GENERATOR = generate_uniform
PREPARE_DATASET = prepare_dataset

# Build all
all:
	$(CXX) $(CXXFLAGS) *.cpp other/*.c other/*.cpp $(LDFLAGS) -Ofast -o $(TARGET)

existing-alg:
	cmake -S ExistingAlg -B ExistingAlg/build
	cmake --build ExistingAlg/build

$(GENERATOR): experiments/generate_uniform.cpp
	$(CXX) --std=c++17 -O2 experiments/generate_uniform.cpp -o $(GENERATOR)

$(PREPARE_DATASET): experiments/prepare_dataset.cpp
	$(CXX) --std=c++17 -O2 experiments/prepare_dataset.cpp -o $(PREPARE_DATASET)

experiments: all existing-alg $(GENERATOR) $(PREPARE_DATASET)

test-experiments: experiments
	python3 -m unittest discover -s experiments/tests -v

# Build with Valgrind
valgrind:
	$(CXX) $(CXXFLAGS) *.cpp other/*.c other/*.cpp $(LDFLAGS) -g -O0 -o $(TARGET)

# Build with GDB
gdb:
	$(CXX) -g $(CXXFLAGS) *.cpp other/*.c other/*.cpp $(LDFLAGS) -o $(TARGET)

# Clean up
clean:
	rm -f $(TARGET) $(GENERATOR) $(PREPARE_DATASET)

.PHONY: clean existing-alg experiments test-experiments
