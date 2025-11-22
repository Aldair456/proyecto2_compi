//
// Created by semin on 21/11/2025.
//

#include "optimizer.h"


#include <iostream>

// ========== CONSTRUCTOR ==========
// Se ejecuta cuando creas un Optimizer
Optimizer::Optimizer() {
    // Por ahora no necesitamos inicializar nada
    // Pero aquí podrías inicializar variables si las necesitas
}

// ========== MÉTODO PRINCIPAL: optimize ==========
// Este es el punto de entrada, optimiza todo el programa
void Optimizer::optimize(Program* program) {
    cout << "  Applying optimizations..." << endl;

    // Recorrer todos los statements del programa (funciones, declaraciones globales)
    for (auto& stmt : program->statements) {
        optimizeStmt(stmt.get());
    }

    cout << "  Optimizations complete!" << endl;
}

// ========== OPTIMIZAR STATEMENTS ==========
// Recibe un statement y lo optimiza según su tipo
void Optimizer::optimizeStmt(Stmt* stmt) {
    // Intentamos hacer un "cast" para ver qué tipo de statement es

    // ¿Es una declaración de variable? (int x = 2 + 3;)
    // ¿Es una declaración de variable? (int x = 2 + 3;)
    if (VarDecl* varDecl = dynamic_cast<VarDecl*>(stmt)) {
        // Si tiene un inicializador, optimizarlo
        if (varDecl->initializer) {
            // Optimizar la expresión y reemplazarla
            varDecl->initializer = optimizeExpr(varDecl->initializer.get());

            // CONSTANT PROPAGATION: Si el inicializador es un literal, guardarlo
            int value;
            if (isIntLiteral(varDecl->initializer.get(), value)) {
                constantValues[varDecl->name] = value;
                cout << "    Propagating constant: " << varDecl->name << " = " << value << endl;
            }
        }
    }

    // ¿Es una asignación? (x = 2 + 3;)
    else if (AssignStmt* assign = dynamic_cast<AssignStmt*>(stmt)) {
        // Optimizar el valor que se está asignando
        assign->value = optimizeExpr(assign->value.get());

        // CONSTANT PROPAGATION: Si el valor es un literal, guardarlo
        int value;
        if (isIntLiteral(assign->value.get(), value)) {
            constantValues[assign->varName] = value;
            cout << "    Propagating constant: " << assign->varName << " = " << value << endl;
        } else {
            // Si no es literal, eliminar del mapa (ya no es constante)
            constantValues.erase(assign->varName);
        }
    }

    // ¿Es un bloque de código? ({ ... })
    else if (Block* block = dynamic_cast<Block*>(stmt)) {
        optimizeBlock(block);
    }

    // ¿Es un if-statement? (if (condition) { ... })
    else if (IfStmt* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        // Optimizar la condición
        ifStmt->condition = optimizeExpr(ifStmt->condition.get());

        // Optimizar la rama then
        optimizeStmt(ifStmt->thenBranch.get());

        // Si hay rama else, optimizarla también
        if (ifStmt->elseBranch) {
            optimizeStmt(ifStmt->elseBranch.get());
        }
    }

    // ¿Es un while-loop? (while (condition) { ... })
    else if (WhileStmt* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        // Optimizar la condición
        whileStmt->condition = optimizeExpr(whileStmt->condition.get());

        // Optimizar el cuerpo
        optimizeStmt(whileStmt->body.get());
    }

    // ¿Es un for-loop? (for (init; cond; inc) { ... })
    else if (ForStmt* forStmt = dynamic_cast<ForStmt*>(stmt)) {
        // Optimizar inicializador
        if (forStmt->initializer) {
            optimizeStmt(forStmt->initializer.get());
        }

        // Optimizar condición
        if (forStmt->condition) {
            forStmt->condition = optimizeExpr(forStmt->condition.get());
        }

        // Optimizar incremento
        if (forStmt->increment) {
            forStmt->increment = optimizeExpr(forStmt->increment.get());
        }

        // Optimizar el cuerpo
        optimizeStmt(forStmt->body.get());
    }

    // ¿Es un return? (return 2 + 3;)
    else if (ReturnStmt* returnStmt = dynamic_cast<ReturnStmt*>(stmt)) {
        if (returnStmt->value) {
            returnStmt->value = optimizeExpr(returnStmt->value.get());
        }
    }

    // ¿Es una declaración de función?
    // ¿Es una declaración de función?
    else if (FunctionDecl* funcDecl = dynamic_cast<FunctionDecl*>(stmt)) {
        // Limpiar valores constantes (nueva función = nuevo scope)
        constantValues.clear();

        // Optimizar el cuerpo de la función
        optimizeBlock(funcDecl->body.get());
    }

    // ¿Es un expression statement? (printf(...); suma(a,b);)
    else if (ExprStmt* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        exprStmt->expression = optimizeExpr(exprStmt->expression.get());
    }
}

// ========== OPTIMIZAR BLOQUES ==========
// Un bloque es una lista de statements: { stmt1; stmt2; stmt3; }
// ========== OPTIMIZAR BLOQUES (con Dead Code Elimination) ==========
// ========== OPTIMIZAR BLOQUES (con Dead Code Elimination y Loop Unrolling) ==========
void Optimizer::optimizeBlock(Block* block) {
    // Crear un nuevo vector para statements optimizados
    vector<unique_ptr<Stmt>> optimizedStmts;

    // Recorrer cada statement del bloque
    for (auto& stmt : block->statements) {
        // LOOP UNROLLING: Verificar si es un for-loop desenrollable
        if (ForStmt* forStmt = dynamic_cast<ForStmt*>(stmt.get())) {
            // Intentar desenrollar el loop
            if (tryUnrollLoop(forStmt, optimizedStmts)) {
                // Loop fue desenrollado exitosamente, ya se agregó a optimizedStmts
                continue;
            }
            // Si no se pudo desenrollar, optimizar normalmente
        }

        // DEAD CODE ELIMINATION: Verificar si es un if con condición constante
        if (IfStmt* ifStmt = dynamic_cast<IfStmt*>(stmt.get())) {
            // Optimizar la condición primero
            ifStmt->condition = optimizeExpr(ifStmt->condition.get());

            int condValue;
            if (isIntLiteral(ifStmt->condition.get(), condValue)) {
                if (condValue == 0) {
                    // if (0) - La rama then NUNCA se ejecuta
                    cout << "    Eliminated dead code: if (0) { ... } block removed" << endl;

                    // Si hay else, mantener solo el else
                    if (ifStmt->elseBranch) {
                        cout << "    Keeping else branch" << endl;
                        optimizeStmt(ifStmt->elseBranch.get());
                        optimizedStmts.push_back(move(ifStmt->elseBranch));
                    }
                    continue;
                }
                else {
                    // if (1) - Siempre verdadero
                    cout << "    Eliminated dead code: condition always true, removed else branch" << endl;

                    // Mantener solo la rama then
                    optimizeStmt(ifStmt->thenBranch.get());
                    optimizedStmts.push_back(move(ifStmt->thenBranch));
                    continue;
                }
            }
        }

        // Statement normal: optimizarlo y agregarlo
        optimizeStmt(stmt.get());
        optimizedStmts.push_back(move(stmt));
    }

    // Reemplazar los statements del bloque con los optimizados
    block->statements = move(optimizedStmts);
}
// ========== OPTIMIZAR EXPRESIONES ==========
// Recibe cualquier expresión y la optimiza según su tipo
unique_ptr<Expr> Optimizer::optimizeExpr(Expr* expr) {
    // ¿Es un literal entero? (5, 10, 42)
    if (IntLiteral* lit = dynamic_cast<IntLiteral*>(expr)) {
        // Los literales ya están optimizados, devolver una copia
        return make_unique<IntLiteral>(lit->value);
    }

    // ¿Es un literal float? (3.14, 2.5)
    else if (FloatLiteral* lit = dynamic_cast<FloatLiteral*>(expr)) {
        return make_unique<FloatLiteral>(lit->value);
    }

    // ¿Es un literal long?
    else if (LongLiteral* lit = dynamic_cast<LongLiteral*>(expr)) {
        return make_unique<LongLiteral>(lit->value);
    }

    // ¿Es un string literal? ("hello")
    else if (StringLiteral* lit = dynamic_cast<StringLiteral*>(expr)) {
        return make_unique<StringLiteral>(lit->value);
    }

    // ¿Es una variable? (x, y, count)
    // ¿Es una variable? (x, y, count)
    else if (Variable* var = dynamic_cast<Variable*>(expr)) {
        // CONSTANT PROPAGATION: Si conocemos el valor, reemplazarlo
        if (constantValues.find(var->name) != constantValues.end()) {
            int value = constantValues[var->name];
            cout << "    Replacing variable " << var->name << " with " << value << endl;
            return make_unique<IntLiteral>(value);
        }

        // Si no conocemos el valor, devolver la variable
        return make_unique<Variable>(var->name);
    }

    // ¿Es una operación binaria? (2 + 3, x * 4)
    else if (BinaryOp* binOp = dynamic_cast<BinaryOp*>(expr)) {
        // ¡AQUÍ APLICAMOS CONSTANT FOLDING!
        return optimizeBinaryOp(binOp);
    }

    // ¿Es una operación unaria? (-5, !true)
    else if (UnaryOp* unOp = dynamic_cast<UnaryOp*>(expr)) {
        // Optimizar el operando
        auto optimizedOperand = optimizeExpr(unOp->operand.get());

        // Intentar constant folding si el operando es literal
        int value;
        if (isIntLiteral(optimizedOperand.get(), value)) {
            if (unOp->op.type == TokenType::MINUS) {
                // -5 → literal(-5)
                return make_unique<IntLiteral>(-value);
            }
        }

        // Si no se puede optimizar, devolver el nodo original
        return make_unique<UnaryOp>(unOp->op, move(optimizedOperand));
    }

    // ¿Es un cast? ((float)x)
    else if (CastExpr* cast = dynamic_cast<CastExpr*>(expr)) {
        auto optimizedExpr = optimizeExpr(cast->expr.get());
        return make_unique<CastExpr>(cast->targetType, move(optimizedExpr));
    }

    // ¿Es un operador ternario? (cond ? a : b)
    else if (TernaryExpr* ternary = dynamic_cast<TernaryExpr*>(expr)) {
        auto optimizedCond = optimizeExpr(ternary->condition.get());
        auto optimizedTrue = optimizeExpr(ternary->exprTrue.get());
        auto optimizedFalse = optimizeExpr(ternary->exprFalse.get());

        return make_unique<TernaryExpr>(
            move(optimizedCond),
            move(optimizedTrue),
            move(optimizedFalse)
        );
    }

    // ¿Es una llamada a función? (suma(a, b))
    else if (CallExpr* call = dynamic_cast<CallExpr*>(expr)) {
        // Optimizar cada argumento
        vector<unique_ptr<Expr>> optimizedArgs;
        for (auto& arg : call->arguments) {
            optimizedArgs.push_back(optimizeExpr(arg.get()));
        }

        return make_unique<CallExpr>(call->functionName, move(optimizedArgs));
    }

    // ¿Es un acceso a array? (arr[i])
    else if (ArrayAccess* arrAccess = dynamic_cast<ArrayAccess*>(expr)) {
        // Optimizar cada índice
        vector<unique_ptr<Expr>> optimizedIndices;
        for (auto& index : arrAccess->indices) {
            optimizedIndices.push_back(optimizeExpr(index.get()));
        }

        return make_unique<ArrayAccess>(arrAccess->arrayName, move(optimizedIndices));
    }

    // Si no reconocemos el tipo, devolver nullptr
    return nullptr;
}

// ========== OPTIMIZAR OPERACIONES BINARIAS (CONSTANT FOLDING) ==========
//
// ========== OPTIMIZAR OPERACIONES BINARIAS (CONSTANT FOLDING + ALGEBRAIC SIMPLIFICATION) ==========
unique_ptr<Expr> Optimizer::optimizeBinaryOp(BinaryOp* node) {
    // Paso 1: Optimizar recursivamente los operandos izquierdo y derecho
    auto left = optimizeExpr(node->left.get());
    auto right = optimizeExpr(node->right.get());

    // Paso 2: Verificar si ambos operandos son literales enteros
    int leftValue, rightValue;
    bool leftIsLiteral = isIntLiteral(left.get(), leftValue);
    bool rightIsLiteral = isIntLiteral(right.get(), rightValue);

    // Paso 3: CONSTANT FOLDING - Si AMBOS son literales
    if (leftIsLiteral && rightIsLiteral) {
        int result = calculate(leftValue, node->op.type, rightValue);

        cout << "    Folded: " << leftValue << " "
             << node->op.lexeme << " " << rightValue
             << " -> " << result << endl;

        return make_unique<IntLiteral>(result);
    }

    // Paso 4: ALGEBRAIC SIMPLIFICATION
    // Solo si UNO de los operandos es literal

    // Simplificaciones con multiplicación
    if (node->op.type == TokenType::MULTIPLY) {
        // x * 0 = 0
        if (rightIsLiteral && rightValue == 0) {
            cout << "    Simplified: x * 0 -> 0" << endl;
            return make_unique<IntLiteral>(0);
        }
        if (leftIsLiteral && leftValue == 0) {
            cout << "    Simplified: 0 * x -> 0" << endl;
            return make_unique<IntLiteral>(0);
        }

        // x * 1 = x
        if (rightIsLiteral && rightValue == 1) {
            cout << "    Simplified: x * 1 -> x" << endl;
            return left;
        }
        if (leftIsLiteral && leftValue == 1) {
            cout << "    Simplified: 1 * x -> x" << endl;
            return right;
        }

        // x * 2 = x << 1 (shift optimization)
        // x * 4 = x << 2, x * 8 = x << 3, etc.
        if (rightIsLiteral && (rightValue & (rightValue - 1)) == 0 && rightValue > 0) {
            // Es potencia de 2
            int shiftAmount = 0;
            int temp = rightValue;
            while (temp > 1) {
                temp >>= 1;
                shiftAmount++;
            }

            cout << "    Optimized: x * " << rightValue << " -> x << " << shiftAmount << endl;

            // Crear token para shift left
            Token shiftToken(TokenType::UNKNOWN, "<<", 0, 0);
            return make_unique<BinaryOp>(
                move(left),
                shiftToken,
                make_unique<IntLiteral>(shiftAmount)
            );
        }
    }

    // Simplificaciones con suma
    if (node->op.type == TokenType::PLUS) {
        // x + 0 = x
        if (rightIsLiteral && rightValue == 0) {
            cout << "    Simplified: x + 0 -> x" << endl;
            return left;
        }
        if (leftIsLiteral && leftValue == 0) {
            cout << "    Simplified: 0 + x -> x" << endl;
            return right;
        }
    }

    // Simplificaciones con resta
    if (node->op.type == TokenType::MINUS) {
        // x - 0 = x
        if (rightIsLiteral && rightValue == 0) {
            cout << "    Simplified: x - 0 -> x" << endl;
            return left;
        }
    }

    // Simplificaciones con división
    if (node->op.type == TokenType::DIVIDE) {
        // x / 1 = x
        if (rightIsLiteral && rightValue == 1) {
            cout << "    Simplified: x / 1 -> x" << endl;
            return left;
        }

        // x / 2, x / 4, x / 8 -> shift right (solo para potencias de 2)
        if (rightIsLiteral && (rightValue & (rightValue - 1)) == 0 && rightValue > 0) {
            int shiftAmount = 0;
            int temp = rightValue;
            while (temp > 1) {
                temp >>= 1;
                shiftAmount++;
            }

            cout << "    Optimized: x / " << rightValue << " -> x >> " << shiftAmount << endl;

            Token shiftToken(TokenType::UNKNOWN, ">>", 0, 0);
            return make_unique<BinaryOp>(
                move(left),
                shiftToken,
                make_unique<IntLiteral>(shiftAmount)
            );
        }
    }

    // Paso 5: Si no se puede optimizar, devolver el BinaryOp con operandos optimizados
    return make_unique<BinaryOp>(move(left), node->op, move(right));
}

// ========== HELPER: Verificar si es IntLiteral ==========
bool Optimizer::isIntLiteral(Expr* expr, int& value) {
    // Intentar hacer cast a IntLiteral
    if (IntLiteral* lit = dynamic_cast<IntLiteral*>(expr)) {
        value = lit->value;  // Guardar el valor
        return true;
    }
    return false;
}

// ========== HELPER: Calcular operación ==========
int Optimizer::calculate(int left, TokenType op, int right) {
    switch(op) {
        case TokenType::PLUS:
            return left + right;

        case TokenType::MINUS:
            return left - right;

        case TokenType::MULTIPLY:
            return left * right;

        case TokenType::DIVIDE:
            if (right == 0) {
                cerr << "Warning: Division by zero in constant folding" << endl;
                return 0;
            }
            return left / right;

        case TokenType::MODULO:
            if (right == 0) {
                cerr << "Warning: Modulo by zero in constant folding" << endl;
                return 0;
            }
            return left % right;

        default:
            // Para operadores relacionales o que no podemos calcular
            cerr << "Warning: Cannot fold operator" << endl;
            return 0;
    }
}


// ========== LOOP UNROLLING ==========
// Retorna true si el loop fue desenrollado exitosamente
bool Optimizer::tryUnrollLoop(ForStmt* forStmt, vector<unique_ptr<Stmt>>& output) {
    // Solo desenrollar loops muy simples:
    // - Inicializador: i = 0
    // - Condición: i < N (donde N es literal)
    // - Incremento: i = i + 1
    // - Cuerpo simple (sin breaks, continues, etc.)

    // 1. Verificar inicializador: i = 0 (o cualquier literal)
    if (!forStmt->initializer) return false;

    VarDecl* initDecl = dynamic_cast<VarDecl*>(forStmt->initializer.get());
    AssignStmt* initAssign = dynamic_cast<AssignStmt*>(forStmt->initializer.get());

    string loopVar;
    int startValue;

    if (initDecl) {
        // int i = 0;
        if (!initDecl->initializer) return false;
        if (!isIntLiteral(initDecl->initializer.get(), startValue)) return false;
        loopVar = initDecl->name;
    } else if (initAssign) {
        // i = 0;
        if (!isIntLiteral(initAssign->value.get(), startValue)) return false;
        loopVar = initAssign->varName;
    } else {
        return false;
    }

    // 2. Verificar condición: i < N
    if (!forStmt->condition) return false;

    BinaryOp* condition = dynamic_cast<BinaryOp*>(forStmt->condition.get());
    if (!condition || condition->op.type != TokenType::LT) return false;

    Variable* condVar = dynamic_cast<Variable*>(condition->left.get());
    if (!condVar || condVar->name != loopVar) return false;

    int endValue;
    if (!isIntLiteral(condition->right.get(), endValue)) return false;

    // 3. Verificar incremento: i = i + 1
    if (!forStmt->increment) return false;

    // El incremento puede estar como ExprStmt que contiene una expresión
    AssignStmt* incAssign = dynamic_cast<AssignStmt*>(forStmt->increment.get());

    if (!incAssign || incAssign->varName != loopVar) return false;

    BinaryOp* incExpr = dynamic_cast<BinaryOp*>(incAssign->value.get());
    if (!incExpr || incExpr->op.type != TokenType::PLUS) return false;

    Variable* incVar = dynamic_cast<Variable*>(incExpr->left.get());
    if (!incVar || incVar->name != loopVar) return false;

    int incValue;
    if (!isIntLiteral(incExpr->right.get(), incValue)) return false;
    if (incValue != 1) return false;

    // 4. Calcular número de iteraciones
    int iterations = (endValue - startValue) / incValue;

    // Solo desenrollar si es pequeño (≤ 10 iteraciones)
    if (iterations <= 0 || iterations > 10) {
        return false;
    }

    cout << "    Unrolling loop: " << iterations << " iterations" << endl;

    // 5. Agregar el inicializador
    output.push_back(cloneStmt(forStmt->initializer.get()));

    // 6. Desenrollar el cuerpo
    for (int i = startValue; i < endValue; i += incValue) {
        // Guardar valor actual de la variable de loop en constantValues
        int savedValue = 0;
        bool hadValue = false;

        if (constantValues.find(loopVar) != constantValues.end()) {
            savedValue = constantValues[loopVar];
            hadValue = true;
        }

        // Establecer valor actual de i
        constantValues[loopVar] = i;

        // Clonar y optimizar el cuerpo con el valor actual de i
        auto bodyClone = cloneStmt(forStmt->body.get());
        if (bodyClone) {
            optimizeStmt(bodyClone.get());

            // Si el cuerpo es un Block, aplanar sus statements
            if (Block* bodyBlock = dynamic_cast<Block*>(bodyClone.get())) {
                for (auto& s : bodyBlock->statements) {
                    output.push_back(move(s));
                }
            } else {
                output.push_back(move(bodyClone));
            }
        }

        // Restaurar valor anterior
        if (hadValue) {
            constantValues[loopVar] = savedValue;
        } else {
            constantValues.erase(loopVar);
        }
    }

    // Eliminar la variable de loop del mapa (ya no existe después del loop)
    constantValues.erase(loopVar);

    return true;
}

// ========== CLONACIÓN DE NODOS ==========
unique_ptr<Stmt> Optimizer::cloneStmt(Stmt* stmt) {
    if (!stmt) return nullptr;

    // VarDecl
    if (VarDecl* varDecl = dynamic_cast<VarDecl*>(stmt)) {
        if (varDecl->initializer) {
            return make_unique<VarDecl>(
                varDecl->type,
                varDecl->name,
                cloneExpr(varDecl->initializer.get())
            );
        } else {
            return make_unique<VarDecl>(
                varDecl->type,
                varDecl->name,
                nullptr
            );
        }
    }

    // AssignStmt
    if (AssignStmt* assign = dynamic_cast<AssignStmt*>(stmt)) {
        return make_unique<AssignStmt>(
            assign->varName,
            cloneExpr(assign->value.get())
        );
    }

    // Block
    if (Block* block = dynamic_cast<Block*>(stmt)) {
        vector<unique_ptr<Stmt>> clonedStmts;
        for (auto& s : block->statements) {
            auto cloned = cloneStmt(s.get());
            if (cloned) {
                clonedStmts.push_back(move(cloned));
            }
        }
        return make_unique<Block>(move(clonedStmts));
    }

    // ExprStmt
    if (ExprStmt* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        return make_unique<ExprStmt>(cloneExpr(exprStmt->expression.get()));
    }

    return nullptr;
}

unique_ptr<Expr> Optimizer::cloneExpr(Expr* expr) {
    if (!expr) return nullptr;

    // IntLiteral
    if (IntLiteral* lit = dynamic_cast<IntLiteral*>(expr)) {
        return make_unique<IntLiteral>(lit->value);
    }

    // Variable
    if (Variable* var = dynamic_cast<Variable*>(expr)) {
        // IMPORTANTE: Si la variable está en constantValues, reemplazarla
        if (constantValues.find(var->name) != constantValues.end()) {
            return make_unique<IntLiteral>(constantValues[var->name]);
        }
        return make_unique<Variable>(var->name);
    }

    // BinaryOp
    if (BinaryOp* binOp = dynamic_cast<BinaryOp*>(expr)) {
        auto leftClone = cloneExpr(binOp->left.get());
        auto rightClone = cloneExpr(binOp->right.get());

        if (leftClone && rightClone) {
            return make_unique<BinaryOp>(
                move(leftClone),
                binOp->op,
                move(rightClone)
            );
        }
    }

    return nullptr;
}