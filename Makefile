# Compilador C++
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Directorios
SRC_DIRS = scanner parser visitors
OBJ_DIR = obj

# Archivos fuente
SOURCES = main.cpp \
          scanner/token.cpp scanner/scanner.cpp \
          parser/ast.cpp parser/parser.cpp \
          visitors/codegen.cpp

# Archivos objeto
OBJECTS = $(SOURCES:.cpp=.o)

# Ejecutable
TARGET = compiler

# Regla principal
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilar archivos .cpp a .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f tests/*.asm tests/*.o tests/program
	rm -f output.asm output.o program

# Limpiar todo incluyendo tests compilados
cleanall: clean
	find tests -type f -executable -delete

# Ejecutar un test
test: $(TARGET)
	@echo "Running test..."
	./$(TARGET) tests/base/test1.c output.asm
	nasm -f elf64 output.asm -o output.o
	gcc output.o -o program -no-pie
	./program

# Help
help:
	@echo "Makefile for C Compiler"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the compiler (default)"
	@echo "  clean     - Remove generated files"
	@echo "  cleanall  - Remove all generated files including test executables"
	@echo "  test      - Build and run a test case"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build compiler"
	@echo "  make test         # Run test"
	@echo "  ./compiler input.c output.asm"

.PHONY: all clean cleanall test help

