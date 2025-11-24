#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <iostream>

using namespace std;

enum class TokenType {
    // Keywords
    INT, FLOAT, LONG, UNSIGNED,
    IF, ELSE, WHILE, FOR, RETURN,
    PRINTF, INCLUDE,
    // Operadores aritméticos
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    INCREMENT, DECREMENT,  // ++ --
    
    // Operadores de asignación
    ASSIGN, PLUSEQ, MINUSEQ,
    
    // Operadores relacionales
    EQ, NE, LT, GT, LE, GE,
    
    // Operadores lógicos
    AND, OR, NOT,
    
    // Delimitadores
    LPAREN, RPAREN,       // ( )
    LBRACE, RBRACE,       // { }
    LBRACKET, RBRACKET,   // [ ]  <- Para arrays
    SEMICOLON, COMMA,     // ; ,
    
    // Literales
    INT_LITERAL,
    FLOAT_LITERAL,
    LONG_LITERAL,
    STRING_LITERAL,
    
    // Otros
    IDENTIFIER,
    END_OF_FILE,
    UNKNOWN
};

class Token {
public:
    TokenType type;
    string lexeme;
    int line;
    int column;
    
    // Constructor
    Token(TokenType type, string lexeme, int line, int column);
    
    // Constructor por defecto
    Token();
    
    // Método para imprimir el token (útil para debugging)
    string toString() const;
    
    // Método helper para obtener el nombre del tipo
    static string typeToString(TokenType type);
};

#endif