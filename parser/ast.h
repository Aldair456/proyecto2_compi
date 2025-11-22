#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include "../scanner/token.h"

using namespace std;

// Forward declarations
class Visitor;

// ========== TIPOS DE DATOS ==========
enum class DataType {
    INT,
    FLOAT,
    LONG,
    UNSIGNED_INT,
    VOID,
    UNKNOWN
};

string dataTypeToString(DataType type);

// ========== CLASE BASE PARA EXPRESIONES ==========
class Expr {
public:
    virtual ~Expr() = default;
    virtual void accept(Visitor* visitor) = 0;
    DataType inferredType = DataType::UNKNOWN;
};

// ========== CLASE BASE PARA STATEMENTS ==========
class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void accept(Visitor* visitor) = 0;
};

// ========== EXPRESIONES ==========

// Literales numéricos
class IntLiteral : public Expr {
public:
    int value;
    IntLiteral(int value);
    void accept(Visitor* visitor) override;
};

class FloatLiteral : public Expr {
public:
    float value;
    FloatLiteral(float value);
    void accept(Visitor* visitor) override;
};

class LongLiteral : public Expr {
public:
    long value;
    LongLiteral(long value);
    void accept(Visitor* visitor) override;
};

// String literal
class StringLiteral : public Expr {
public:
    string value;
    StringLiteral(string value);
    void accept(Visitor* visitor) override;
};

// Variable (identificador)
class Variable : public Expr {
public:
    string name;
    Variable(string name);
    void accept(Visitor* visitor) override;
};

// Operación binaria: +, -, *, /, ==, <, >, etc.
class BinaryOp : public Expr {
public:
    unique_ptr<Expr> left;
    Token op;
    unique_ptr<Expr> right;
    
    BinaryOp(unique_ptr<Expr> left, Token op, unique_ptr<Expr> right);
    void accept(Visitor* visitor) override;
};

// Operación unaria: -, !
class UnaryOp : public Expr {
public:
    Token op;
    unique_ptr<Expr> operand;
    
    UnaryOp(Token op, unique_ptr<Expr> operand);
    void accept(Visitor* visitor) override;
};

// Casting explícito: (float)x, (int)y
class CastExpr : public Expr {
public:
    DataType targetType;
    unique_ptr<Expr> expr;
    
    CastExpr(DataType targetType, unique_ptr<Expr> expr);
    void accept(Visitor* visitor) override;
};

// Operador ternario: condition ? exprTrue : exprFalse
class TernaryExpr : public Expr {
public:
    unique_ptr<Expr> condition;
    unique_ptr<Expr> exprTrue;
    unique_ptr<Expr> exprFalse;
    
    TernaryExpr(unique_ptr<Expr> condition, unique_ptr<Expr> exprTrue, unique_ptr<Expr> exprFalse);
    void accept(Visitor* visitor) override;
};

// Llamada a función: suma(a, b)
class CallExpr : public Expr {
public:
    string functionName;
    vector<unique_ptr<Expr>> arguments;
    
    CallExpr(string functionName, vector<unique_ptr<Expr>> arguments);
    void accept(Visitor* visitor) override;
};

// Acceso a array: arr[i] o matriz[i][j]
class ArrayAccess : public Expr {
public:
    string arrayName;
    vector<unique_ptr<Expr>> indices; // Para multidimensional
    
    ArrayAccess(string arrayName, vector<unique_ptr<Expr>> indices);
    void accept(Visitor* visitor) override;
};

// ========== STATEMENTS ==========

// Declaración de variable: int x; float y = 3.14;
class VarDecl : public Stmt {
public:
    DataType type;
    string name;
    unique_ptr<Expr> initializer; // Puede ser nullptr
    
    // Para arrays
    bool isArray;
    vector<int> dimensions; // [3][4] -> {3, 4}
    vector<unique_ptr<Expr>> arrayInitializer; // {1, 2, 3}
    
    VarDecl(DataType type, string name, unique_ptr<Expr> initializer = nullptr);
    VarDecl(DataType type, string name, vector<int> dimensions); // Array
    void accept(Visitor* visitor) override;
};

// Asignación: x = 10; arr[i] = 5;
class AssignStmt : public Stmt {
public:
    string varName;
    unique_ptr<Expr> value;
    
    // Para arrays
    bool isArrayAssign;
    vector<unique_ptr<Expr>> indices;
    
    AssignStmt(string varName, unique_ptr<Expr> value);
    AssignStmt(string varName, vector<unique_ptr<Expr>> indices, unique_ptr<Expr> value);
    void accept(Visitor* visitor) override;
};

// Bloque de código: { ... }
class Block : public Stmt {
public:
    vector<unique_ptr<Stmt>> statements;
    
    Block(vector<unique_ptr<Stmt>> statements);
    void accept(Visitor* visitor) override;
};

// If-else
class IfStmt : public Stmt {
public:
    unique_ptr<Expr> condition;
    unique_ptr<Stmt> thenBranch;
    unique_ptr<Stmt> elseBranch; // Puede ser nullptr
    
    IfStmt(unique_ptr<Expr> condition, unique_ptr<Stmt> thenBranch, unique_ptr<Stmt> elseBranch = nullptr);
    void accept(Visitor* visitor) override;
};

// While loop
class WhileStmt : public Stmt {
public:
    unique_ptr<Expr> condition;
    unique_ptr<Stmt> body;
    
    WhileStmt(unique_ptr<Expr> condition, unique_ptr<Stmt> body);
    void accept(Visitor* visitor) override;
};

// For loop
class ForStmt : public Stmt {
public:
    unique_ptr<Stmt> initializer; // int i = 0
    unique_ptr<Expr> condition;   // i < 10
    unique_ptr<Expr> increment;   // i++
    unique_ptr<Stmt> body;
    
    ForStmt(unique_ptr<Stmt> initializer, unique_ptr<Expr> condition, 
            unique_ptr<Expr> increment, unique_ptr<Stmt> body);
    void accept(Visitor* visitor) override;
};

// Return statement
class ReturnStmt : public Stmt {
public:
    unique_ptr<Expr> value; // Puede ser nullptr para void
    
    ReturnStmt(unique_ptr<Expr> value = nullptr);
    void accept(Visitor* visitor) override;
};

// Expression statement: printf(...); suma(a,b);
class ExprStmt : public Stmt {
public:
    unique_ptr<Expr> expression;
    
    ExprStmt(unique_ptr<Expr> expression);
    void accept(Visitor* visitor) override;
};

// Declaración de función
class FunctionDecl : public Stmt {
public:
    DataType returnType;
    string name;
    vector<pair<DataType, string>> parameters; // (tipo, nombre)
    unique_ptr<Block> body;
    
    FunctionDecl(DataType returnType, string name, 
                 vector<pair<DataType, string>> parameters, 
                 unique_ptr<Block> body);
    void accept(Visitor* visitor) override;
};

// Programa completo
class Program {
public:
    vector<unique_ptr<Stmt>> statements; // Funciones y declaraciones globales
    
    Program(vector<unique_ptr<Stmt>> statements);
};

// ========== VISITOR PATTERN ==========
class Visitor {
public:
    virtual ~Visitor() = default;
    
    // Expresiones
    virtual void visitIntLiteral(IntLiteral* node) = 0;
    virtual void visitFloatLiteral(FloatLiteral* node) = 0;
    virtual void visitLongLiteral(LongLiteral* node) = 0;
    virtual void visitStringLiteral(StringLiteral* node) = 0;
    virtual void visitVariable(Variable* node) = 0;
    virtual void visitBinaryOp(BinaryOp* node) = 0;
    virtual void visitUnaryOp(UnaryOp* node) = 0;
    virtual void visitCastExpr(CastExpr* node) = 0;
    virtual void visitTernaryExpr(TernaryExpr* node) = 0;
    virtual void visitCallExpr(CallExpr* node) = 0;
    virtual void visitArrayAccess(ArrayAccess* node) = 0;
    
    // Statements
    virtual void visitVarDecl(VarDecl* node) = 0;
    virtual void visitAssignStmt(AssignStmt* node) = 0;
    virtual void visitBlock(Block* node) = 0;
    virtual void visitIfStmt(IfStmt* node) = 0;
    virtual void visitWhileStmt(WhileStmt* node) = 0;
    virtual void visitForStmt(ForStmt* node) = 0;
    virtual void visitReturnStmt(ReturnStmt* node) = 0;
    virtual void visitExprStmt(ExprStmt* node) = 0;
    virtual void visitFunctionDecl(FunctionDecl* node) = 0;
};

#endif