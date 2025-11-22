#include <iostream>
#include <fstream>
#include <sstream>
#include "scanner/scanner.h"
#include "parser/parser.h"
#include "visitors/codegen.h"

using namespace std;

string readFile(string filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        exit(1);
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(string filename, string content) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not write to file " << filename << endl;
        exit(1);
    }
    
    file << content;
    file.close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.c> [output.asm]" << endl;
        return 1;
    }
    
    string inputFile = argv[1];
    string outputFile = argc >= 3 ? argv[2] : "output.asm";
    
    cout << "Compiling " << inputFile << "..." << endl;
    
    // 1. Leer archivo fuente
    string source = readFile(inputFile);
    
    // 2. Análisis léxico (Scanner)
    cout << "Phase 1: Lexical analysis..." << endl;
    Scanner scanner(source);
    vector<Token> tokens = scanner.scanTokens();
    
    cout << "  Tokens generated: " << tokens.size() << endl;
    
    // Debug: Mostrar tokens (opcional - descomentar para debug)
    /*
    for (const Token& token : tokens) {
        cout << "  " << token.toString() << endl;
    }
    */
    
    // 3. Análisis sintáctico (Parser)
    cout << "Phase 2: Syntax analysis..." << endl;
    Parser parser(tokens);
    unique_ptr<Program> ast = parser.parse();
    
    cout << "  AST built successfully" << endl;
    
    // 4. Generación de código (CodeGen)
    cout << "Phase 3: Code generation..." << endl;
    CodeGen codegen;
    codegen.generate(ast.get());
    
    string asmCode = codegen.getOutput();
    
    // 5. Escribir archivo ensamblador
    writeFile(outputFile, asmCode);
    
    cout << "Success! Assembly code written to " << outputFile << endl;
    cout << "\nTo assemble and link:" << endl;
    cout << "  nasm -f elf64 " << outputFile << " -o output.o" << endl;
    cout << "  gcc output.o -o program -no-pie" << endl;
    cout << "  ./program" << endl;
    
    return 0;
}

