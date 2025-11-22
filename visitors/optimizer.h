//
// Created by semin on 21/11/2025.
//

#ifndef PROYECTO_OPTIMIZER_H
#define PROYECTO_OPTIMIZER_H

#include "../parser/ast.h"
#include <map>
#include <memory>

using namespace std;

// ========== CLASE OPTIMIZER ==========
class Optimizer {
public:
    // Constructor: se ejecuta cuando creas un Optimizer
    Optimizer();

    // Método principal: optimiza todo el programa
    // Recibe un puntero al programa (AST completo)
    map<string, int> constantValues;
    void optimize(Program* program);

private:
    // ========== MÉTODOS PRIVADOS (solo para uso interno) ==========
    // Intenta desenrollar un for-loop si cumple ciertas condiciones
    unique_ptr<Stmt> tryUnrollLoop(ForStmt* forStmt);

    // Clona un statement (necesario para duplicar el cuerpo del loop)
    unique_ptr<Stmt> cloneStmt(Stmt* stmt);

    // Clona una expresión
    unique_ptr<Expr> cloneExpr(Expr* expr);
    // Intenta desenrollar un for-loop si cumple ciertas condiciones
    // Retorna true si fue desenrollado, y agrega los statements a output
    bool tryUnrollLoop(ForStmt* forStmt, vector<unique_ptr<Stmt>>& output);
    // Intenta optimizar una expresión binaria (2 + 3, x * 4, etc.)
    // Devuelve un nuevo nodo optimizado (o el mismo si no se puede optimizar)
    unique_ptr<Expr> optimizeBinaryOp(BinaryOp* node);

    // Optimiza cualquier tipo de expresión recursivamente
    // Devuelve la versión optimizada de la expresión
    unique_ptr<Expr> optimizeExpr(Expr* expr);

    // Optimiza un statement (VarDecl, AssignStmt, IfStmt, etc.)
    void optimizeStmt(Stmt* stmt);

    // Optimiza un bloque de código (lista de statements)
    void optimizeBlock(Block* block);

    // ========== HELPER FUNCTIONS ==========

    // Verifica si una expresión es un literal entero
    // Si lo es, devuelve true y guarda el valor en 'value'
    bool isIntLiteral(Expr* expr, int& value);

    // Aplica constant folding a dos enteros con un operador
    // Ejemplo: calcular(2, TokenType::PLUS, 3) = 5
    int calculate(int left, TokenType op, int right);
};
#endif //PROYECTO_OPTIMIZER_H