#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "scanner/scanner.h"
#include "parser/parser.h"
#include "visitors/codegen.h"
#include "visitors/optimizer.h"
#include "visitors/debuggen.h"

using namespace std;

// Función helper para leer un archivo
string readFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir el archivo " << filename << endl;
        exit(1);
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Función helper para escribir un archivo
void writeFile(const string& filename, const string& content) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: No se pudo crear el archivo " << filename << endl;
        exit(1);
    }
    file << content;
    file.close();
}

int main(int argc, char* argv[]) {
    // Verificar argumentos
    if (argc < 3) {
        cerr << "Uso: " << argv[0] << " <archivo_entrada.c> <archivo_salida.asm> [--debug] [--optimize]" << endl;
        cerr << "  --debug    : Genera archivo debug.json para ejecución paso a paso" << endl;
        cerr << "  --optimize  : Activa optimizaciones del compilador" << endl;
        cerr << "  Nota: Por defecto NO se optimiza (para preservar debug línea por línea)" << endl;
        return 1;
    }
    
    string inputFile = argv[1];
    string outputFile = argv[2];
    
    // Parsear flags opcionales
    bool debugMode = false;
    bool optimizeMode = false;
    
    for (int i = 3; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--debug") {
            debugMode = true;
        } else if (arg == "--optimize") {
            optimizeMode = true;
        }
    }
    
    // 1. Leer código fuente
    string source = readFile(inputFile);
    
    // 2. Análisis léxico (Scanner)
    Scanner scanner(source);
    vector<Token> tokens = scanner.scanTokens();
    
    // 3. Análisis sintáctico (Parser)
    Parser parser(tokens);
    unique_ptr<Program> ast = parser.parse();
    
    if (!ast) {
        cerr << "Error: Fallo en el parsing" << endl;
        return 1;
    }
    
    // 3.5. Optimización (Optimizer) - SOLO si se especifica --optimize
    if (optimizeMode) {
        cout << "🔧 Optimization mode: ENABLED" << endl;
        Optimizer optimizer;
        optimizer.optimize(ast.get());
    } else {
        cout << "🔍 Optimization mode: DISABLED (preserving line-by-line debug experience)" << endl;
    }
    
    // 4. Generación de código (CodeGen)
    CodeGen codegen;
    DebugGen debugGen;
    
    if (debugMode) {
        debugGen.setSourceCode(source); // Set source code for debuggen
        codegen.setDebugGen(&debugGen);
    }
    codegen.generate(ast.get());
    
    string asmCode = codegen.getOutput();
    writeFile(outputFile, asmCode);
    
    if (debugMode) {
        debugGen.generateJSON("output.debug.json");
    }
    
    return 0;
}

