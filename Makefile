# Modular Makefile: auto-detect modules, include dirs and sources

CXX := g++
STD := -std=c++17
CXXFLAGS := -Wall -Wextra $(STD)

# find all include directories (any folder named "include")
INC_DIRS := $(shell find . -type d -name include 2>/dev/null | sed 's|^./||')
CPPFLAGS := $(patsubst %,-I%,$(INC_DIRS))

# collect all .cpp sources (skip build dir)
SRCS := $(shell find . -name '*.cpp' ! -path './build/*' -print | sed 's|^./||')

# place objects under build/ preserving directory structure
OBJS := $(patsubst %.cpp,build/%.o,$(SRCS))

TARGET := nes

.PHONY: all clean show

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo Linking $@
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# generic rule: compile source -> object under build/
# ensures directory exists before compiling
build/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo Compiling $<
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

show:
	@echo "Include dirs: $(INC_DIRS)"
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"

clean:
	@echo Cleaning build artifacts
	@rm -rf build/ $(TARGET)

