#ifndef DEBUGGEN_H
#define DEBUGGEN_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

// Estructura para una instrucción de debug
struct DebugInstruction {
    int id;
    string assembly;
    int sourceLine;
    string varName;      // Variable asociada (si aplica)
    string description;  // Descripción de la instrucción
};

// Estructura para información del stack frame
struct StackFrameInfo {
    string varName;      // Nombre de la variable
    int offset;          // Offset desde RBP (ej: 8, 16, 24)
    string type;         // Tipo: "int", "float", "long"
    bool isArray;        // Si es array
    int sourceLine;      // Línea donde se declaró
};

class DebugGen {
private:
    vector<DebugInstruction> instructions;
    int instructionCounter;
    string sourceCode;  // Código fuente completo
    vector<string> sourceLines;  // Líneas del código fuente
    vector<StackFrameInfo> stackFrames;  // Información del stack frame
    
public:
    DebugGen();
    
    // Configurar código fuente
    void setSourceCode(const string& source);
    
    // Registrar una instrucción
    void logInstruction(const string& assembly, int sourceLine, 
                       const string& varName = "", 
                       const string& description = "");
    
    // Registrar variable en el stack frame
    void logStackVariable(const string& varName, int offset, 
                         const string& type, bool isArray, int sourceLine);
    
    // Limpiar stack frame (cuando termina una función)
    void clearStackFrame();
    
    // Generar archivo JSON
    void generateJSON(const string& filename);
    
    // Obtener JSON como string
    string getJSON();
};

#endif

