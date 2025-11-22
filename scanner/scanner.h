#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"
#include <string>
#include <vector>
#include <map>

using namespace std;

class Scanner {
private:
    string source;           // Código fuente completo
    vector<Token> tokens;    // Lista de tokens generados
    int start;               // Inicio del lexema actual
    int current;             // Posición actual en el source
    int line;                // Línea actual
    int column;              // Columna actual
    
    // Mapa de palabras reservadas
    map<string, TokenType> keywords;
    
    // Métodos auxiliares
    bool isAtEnd();
    char advance();
    char peek();
    char peekNext();
    bool match(char expected);
    
    void addToken(TokenType type);
    void addToken(TokenType type, string lexeme);
    
    void scanToken();
    void identifier();
    void number();
    void scanString();
    
    bool isDigit(char c);
    bool isAlpha(char c);
    bool isAlphaNumeric(char c);
    
    void skipWhitespace();
    void skipComment();
    
    void initKeywords();

public:
    Scanner(string source);
    vector<Token> scanTokens();
};

#endif