#include "codegen.h"
#include <iostream>

CodeGen::CodeGen() : stackOffset(0), labelCounter(0), lastExprWasFloat(false) {}

string CodeGen::getOutput() {
    return output.str();
}

string CodeGen::newLabel(string prefix) {
    return prefix + to_string(labelCounter++);
}

void CodeGen::emit(string code) {
    output << "    " << code << "\n";
}

void CodeGen::emitLabel(string label) {
    output << label << ":\n";
}

string CodeGen::allocReg(DataType type) {
    // Para float usamos registros XMM, para enteros usamos RAX
    if (type == DataType::FLOAT) {
        lastExprWasFloat = true;
        return "xmm0";
    } else {
        lastExprWasFloat = false;
        return "rax";
    }
}

void CodeGen::freeReg(string reg) {
    // Por simplicidad, no gestionamos pool de registros
}

void CodeGen::emitTypeConversion(DataType from, DataType to, string reg) {
    if (from == to) return;

    // INT -> FLOAT
    if (from == DataType::INT && to == DataType::FLOAT) {
        emit("cvtsi2ss xmm0, eax");
    }
    // FLOAT -> INT
    else if (from == DataType::FLOAT && to == DataType::INT) {
        emit("cvttss2si eax, xmm0");
    }
    // INT -> LONG
    else if (from == DataType::INT && to == DataType::LONG) {
        emit("movsx rax, eax");
    }
    // LONG -> INT
    else if (from == DataType::LONG && to == DataType::INT) {
        // Ya está en eax (parte baja de rax)
    }
    // UNSIGNED_INT -> INT (no hacer nada)
    else if (from == DataType::UNSIGNED_INT && to == DataType::INT) {
        // No hacer nada
    }
}

void CodeGen::emitFunctionProlog(string funcName, int stackSize) {
    emit("push rbp");
    emit("mov rbp, rsp");
    if (stackSize > 0) {
        emit("sub rsp, " + to_string(stackSize));
    }
}

void CodeGen::emitFunctionEpilog() {
    emit("mov rsp, rbp");
    emit("pop rbp");
    emit("ret");
}

int CodeGen::calculateArrayOffset(vector<int>& dimensions, int dimIndex) {
    int offset = 1;
    for (size_t i = dimIndex + 1; i < dimensions.size(); i++) {
        offset *= dimensions[i];
    }
    return offset;
}

void CodeGen::generate(Program* program) {
    // Header del archivo ensamblador
    output << "section .data\n";
    output << "    fmt_int: db \"%d\", 10, 0\n";
    output << "    fmt_float: db \"%.2f\", 10, 0\n";
    output << "    fmt_long: db \"%ld\", 10, 0\n";
    output << "\n";

    output << "section .bss\n";
    output << "\n";

    output << "section .text\n";
    output << "    extern printf\n";
    output << "    global main\n";
    output << "\n";

    // Generar código para cada declaración
    for (auto& stmt : program->statements) {
        stmt->accept(this);
    }
}

// ========== EXPRESIONES ==========

void CodeGen::visitIntLiteral(IntLiteral* node) {
    emit("mov eax, " + to_string(node->value));
    lastExprWasFloat = false;
}

void CodeGen::visitFloatLiteral(FloatLiteral* node) {
    // Para floats, necesitamos declarar constantes en .data
    string label = newLabel("float_const_");

    // Agregar a sección .data (hacemos trampa: lo emitimos al inicio)
    stringstream temp;
    temp << output.str();

    // Insertar después de "section .data"
    string currentOutput = output.str();
    size_t dataPos = currentOutput.find("section .data\n");
    if (dataPos != string::npos) {
        size_t insertPos = currentOutput.find("\n", dataPos + 14) + 1;
        currentOutput.insert(insertPos, "    " + label + ": dd " + to_string(node->value) + "\n");
        output.str("");
        output << currentOutput;
    }

    emit("movss xmm0, [" + label + "]");
    lastExprWasFloat = true;
}

void CodeGen::visitLongLiteral(LongLiteral* node) {
    // Limpiar RAX completamente antes de cargar el valor
    // Esto asegura que los 64 bits estén limpios
    emit("xor rax, rax");
    emit("mov eax, " + to_string(node->value));
    lastExprWasFloat = false;
}
void CodeGen::visitStringLiteral(StringLiteral* node) {
    // Para strings, creamos una constante en .data
    string label = newLabel("str_const_");

    // Insertar en sección .data
    string currentOutput = output.str();
    size_t dataPos = currentOutput.find("section .data\n");
    if (dataPos != string::npos) {
        size_t insertPos = currentOutput.find("\n", dataPos + 14) + 1;

        // Construir el string en formato NASM
        string escaped = node->value;
        string nasmStr = "";
        bool hasNewline = false;

        // Procesar caracteres especiales
        for (size_t i = 0; i < escaped.length(); i++) {
            if (escaped[i] == '\\' && i + 1 < escaped.length()) {
                if (escaped[i+1] == 'n') {
                    // Newline - lo manejamos después del string
                    hasNewline = true;
                    i++;  // Saltar el
                    continue;
                } else if (escaped[i+1] == '\\') {
                    nasmStr += "\\\\";
                    i++;  // Saltar el segundo '\'
                } else if (escaped[i+1] == '"') {
                    nasmStr += "\\\"";
                    i++;  // Saltar el '"'
                } else if (escaped[i+1] == 't') {
                    nasmStr += "\\t";
                    i++;
                } else {
                    nasmStr += escaped[i];
                }
            } else if (escaped[i] == '"') {
                nasmStr += "\\\"";
            } else {
                nasmStr += escaped[i];
            }
        }

        // Construir la declaración NASM
        string dbLine = "    " + label + ": db \"" + nasmStr + "\"";
        if (hasNewline) {
            dbLine += ", 10";  // Agregar newline como byte separado
        }
        dbLine += ", 0\n";

        currentOutput.insert(insertPos, dbLine);
        output.str("");
        output << currentOutput;
    }

    // Cargar dirección del string en rax
    emit("lea rax, [" + label + "]");
    lastExprWasFloat = false;
}

void CodeGen::visitVariable(Variable* node) {
    // Buscar variable en local o global
    if (localVars.find(node->name) != localVars.end()) {
        VarInfo& var = localVars[node->name];

        if (var.type == DataType::FLOAT) {
            emit("movss xmm0, [rbp - " + to_string(var.offset) + "]");
            lastExprWasFloat = true;
        } else if (var.type == DataType::LONG) {
            emit("mov rax, [rbp - " + to_string(var.offset) + "]");
            lastExprWasFloat = false;
        } else {
            // Cargar y extender directamente desde memoria (más eficiente)
            emit("mov eax, [rbp - " + to_string(var.offset) + "]");
            emit("movsx rax, eax");
            lastExprWasFloat = false;
        }
    } else if (globalVars.find(node->name) != globalVars.end()) {
        VarInfo& var = globalVars[node->name];
        emit("mov eax, [" + node->name + "]");

        emit("movsx rax, eax");
        lastExprWasFloat = false;
    }
}

void CodeGen::visitBinaryOp(BinaryOp* node) {
    // Evaluar operando derecho primero
    node->right->accept(this);
    bool rightWasFloat = lastExprWasFloat;

    // Guardar resultado en stack
    if (rightWasFloat) {
        emit("sub rsp, 8");
        emit("movss [rsp], xmm0");
    } else {
        emit("push rax");
    }

    // Evaluar operando izquierdo
    node->left->accept(this);
    bool leftWasFloat = lastExprWasFloat;

    // Si alguno es float, la operación será float
    bool isFloatOp = leftWasFloat || rightWasFloat;

    // Recuperar operando derecho
    if (rightWasFloat) {
        emit("movss xmm1, [rsp]");
        emit("add rsp, 8");
    } else {
        emit("pop rbx");
        // Si left es float pero right no, convertir right a float
        if (isFloatOp && !rightWasFloat) {
            emit("cvtsi2ss xmm1, ebx");
        }
    }

    // Si right es float pero left no, convertir left a float
    if (isFloatOp && !leftWasFloat) {
        emit("cvtsi2ss xmm0, eax");
    }

    // Realizar operación
    switch (node->op.type) {
        case TokenType::PLUS:
            if (isFloatOp) {
                emit("addss xmm0, xmm1");
                lastExprWasFloat = true;
            } else {
                emit("add rax, rbx");
                lastExprWasFloat = false;
            }
            break;

        case TokenType::MINUS:
            if (isFloatOp) {
                emit("subss xmm0, xmm1");
                lastExprWasFloat = true;
            } else {
                emit("sub rax, rbx");
                lastExprWasFloat = false;
            }
            break;

        case TokenType::MULTIPLY:
            if (isFloatOp) {
                emit("mulss xmm0, xmm1");
                lastExprWasFloat = true;
            } else {
                emit("imul rax, rbx");
                lastExprWasFloat = false;
            }
            break;

        case TokenType::DIVIDE:
            if (isFloatOp) {
                emit("divss xmm0, xmm1");
                lastExprWasFloat = true;
            } else {
                emit("xor rdx, rdx");  // Clear RDX para división
                emit("idiv rbx");
                lastExprWasFloat = false;
            }
            break;

        // Operadores relacionales
        case TokenType::EQ:
            emit("cmp rax, rbx");
            emit("sete al");
            emit("movzx eax, al");
            lastExprWasFloat = false;
            break;

        case TokenType::NE:
            emit("cmp rax, rbx");
            emit("setne al");
            emit("movzx eax, al");
            lastExprWasFloat = false;
            break;

        case TokenType::LT:
            emit("cmp rax, rbx");
            emit("setl al");
            emit("movzx eax, al");
            lastExprWasFloat = false;
            break;

        case TokenType::GT:
            emit("cmp rax, rbx");
            emit("setg al");
            emit("movzx eax, al");
            lastExprWasFloat = false;
            break;

        case TokenType::LE:
            emit("cmp rax, rbx");
            emit("setle al");
            emit("movzx eax, al");
            lastExprWasFloat = false;
            break;

        case TokenType::GE:
            emit("cmp rax, rbx");
            emit("setge al");
            emit("movzx eax, al");
            lastExprWasFloat = false;
            break;

        default:
            break;
    }
}

void CodeGen::visitUnaryOp(UnaryOp* node) {
    node->operand->accept(this);

    if (node->op.type == TokenType::MINUS) {
        if (lastExprWasFloat) {
            // Negar float: xor con bit de signo
            emit("movss xmm1, xmm0");
            emit("xorps xmm0, xmm0");
            emit("subss xmm0, xmm1");
        } else {
            emit("neg rax");
        }
    } else if (node->op.type == TokenType::NOT) {
        emit("test rax, rax");
        emit("setz al");
        emit("movzx eax, al");
    }
}

void CodeGen::visitCastExpr(CastExpr* node) {
    node->expr->accept(this);

    // Obtener tipo origen
    DataType fromType = node->expr->inferredType;
    DataType toType = node->targetType;

    // Realizar la conversión y actualizar el flag
    if (fromType != toType) {
        if (fromType == DataType::INT && toType == DataType::FLOAT) {
            emit("cvtsi2ss xmm0, eax");
            lastExprWasFloat = true;
        } else if (fromType == DataType::FLOAT && toType == DataType::INT) {
            emit("cvttss2si eax, xmm0");
            lastExprWasFloat = false;
        } else if (fromType == DataType::INT && toType == DataType::LONG) {
            emit("movsx rax, eax");
            lastExprWasFloat = false;
        }
    }
}

void CodeGen::visitTernaryExpr(TernaryExpr* node) {
    string labelFalse = newLabel("ternary_false_");
    string labelEnd = newLabel("ternary_end_");

    // Evaluar condición
    node->condition->accept(this);
    emit("test rax, rax");
    emit("jz " + labelFalse);

    // Rama verdadera
    node->exprTrue->accept(this);
    emit("jmp " + labelEnd);

    // Rama falsa
    emitLabel(labelFalse);
    node->exprFalse->accept(this);

    emitLabel(labelEnd);
}

void CodeGen::visitCallExpr(CallExpr* node) {
    // Caso especial: printf
    if (node->functionName == "printf") {
        if (node->arguments.size() > 0) {
            // El primer argumento es el formato (string)
            // Verificar si es StringLiteral
            StringLiteral* fmtStr = dynamic_cast<StringLiteral*>(node->arguments[0].get());

            if (fmtStr) {
                // Es un string literal - usar como formato directamente
                fmtStr->accept(this);  // Esto carga la dirección del string en rax
                emit("mov rdi, rax");  // Primer argumento: formato

                // Si hay más argumentos, pasarlos
                if (node->arguments.size() > 1) {
                    // CRÍTICO: Guardar rdi antes de evaluar argumentos que puedan ser llamadas a función
                    emit("push rdi");  // Guardar formato en stack

                    node->arguments[1]->accept(this);

                    // Determinar formato basado en tipo del segundo argumento
                    if (lastExprWasFloat) {
                        emit("cvtss2sd xmm0, xmm0");
                        emit("pop rdi");  // Recuperar formato
                        emit("mov rax, 1");  // 1 registro XMM usado
                    } else {
                        // El valor ya está en rax (extendido si era int)
                        // Para printf en x86-64, pasamos el valor en rsi
                        emit("mov rsi, rax");  // Mover valor completo de rax a rsi
                        emit("pop rdi");  // Recuperar formato
                        emit("xor rax, rax");  // 0 registros XMM usados (printf varargs)
                    }
                } else {
                    emit("xor rax, rax");  // Sin argumentos adicionales
                }
            } else {
                // No es string literal - asumir que es un valor numérico
                node->arguments[0]->accept(this);

                // Determinar formato basado en tipo
                if (lastExprWasFloat) {
                    emit("cvtss2sd xmm0, xmm0");
                    emit("lea rdi, [fmt_float]");
                    emit("mov rax, 1");
                } else {
                    emit("mov rsi, rax");
                    emit("lea rdi, [fmt_int]");
                    emit("xor rax, rax");
                }
            }

            // Stack ya está alineado por visitFunctionDecl, solo llamar
            emit("call printf");

        }
    } else {
        // Llamada a función normal

        // Pasar argumentos (convención x86-64: rdi, rsi, rdx, rcx, r8, r9)
        vector<string> argRegs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

        for (size_t i = 0; i < node->arguments.size() && i < 6; i++) {
            node->arguments[i]->accept(this);
            emit("mov " + argRegs[i] + ", rax");
        }

        emit("call " + node->functionName);
    }
}

void CodeGen::visitArrayAccess(ArrayAccess* node) {
    // Buscar info del array
    VarInfo* varInfo = nullptr;

    if (localVars.find(node->arrayName) != localVars.end()) {
        varInfo = &localVars[node->arrayName];
    } else if (globalVars.find(node->arrayName) != globalVars.end()) {
        varInfo = &globalVars[node->arrayName];
    }

    if (!varInfo || !varInfo->isArray) {
        cerr << "Error: " << node->arrayName << " is not an array" << endl;
        return;
    }

    // Calcular offset para arrays multidimensionales
    // arr[i][j] en arr[3][4] = arr[i * 4 + j]

    if (node->indices.size() == 1) {
        // Array 1D simple: arr[i]
        node->indices[0]->accept(this);

        // Multiplicar por tamaño del tipo
        int typeSize = 4; // int, float = 4 bytes
        if (varInfo->type == DataType::LONG) typeSize = 8;

        emit("imul rax, " + to_string(typeSize));
        emit("mov rbx, rbp");
        emit("sub rbx, " + to_string(varInfo->offset));
        emit("add rbx, rax");

        // Cargar valor
        if (varInfo->type == DataType::FLOAT) {
            emit("movss xmm0, [rbx]");
            lastExprWasFloat = true;
        } else {
            emit("mov eax, [rbx]");
            emit("movsx rax, eax");
            lastExprWasFloat = false;
        }
    } else if (node->indices.size() == 2) {
        // Array 2D: arr[i][j]
        // offset = (i * cols + j) * typeSize

        node->indices[0]->accept(this);  // i
        emit("imul rax, " + to_string(varInfo->dimensions[1]));
        emit("push rax");

        node->indices[1]->accept(this);  // j
        emit("pop rbx");
        emit("add rax, rbx");

        int typeSize = 4;
        if (varInfo->type == DataType::LONG) typeSize = 8;

        emit("imul rax, " + to_string(typeSize));
        emit("mov rbx, rbp");
        emit("sub rbx, " + to_string(varInfo->offset));
        emit("add rbx, rax");

        if (varInfo->type == DataType::FLOAT) {
            emit("movss xmm0, [rbx]");
            lastExprWasFloat = true;
        } else {
            emit("mov eax, [rbx]");
            lastExprWasFloat = false;
        }
    }
}

// ========== STATEMENTS ==========

void CodeGen::visitVarDecl(VarDecl* node) {
    if (currentFunction.empty()) {
        // Variable global
        // TODO: Implementar variables globales en .bss
    } else {
        // Variable local
        int size = 4; // Por defecto int, float

        if (node->type == DataType::LONG) {
            size = 8;
        }

        if (node->isArray) {
            // Calcular tamaño total del array
            int totalSize = size;
            for (int dim : node->dimensions) {
                totalSize *= dim;
            }
            stackOffset += totalSize;
        } else {
            stackOffset += size;
        }

        VarInfo varInfo;
        varInfo.type = node->type;
        varInfo.offset = stackOffset;
        varInfo.isArray = node->isArray;
        varInfo.dimensions = node->dimensions;

        localVars[node->name] = varInfo;

        // Si hay inicializador
        if (node->initializer) {
            node->initializer->accept(this);

            if (node->type == DataType::FLOAT) {
                emit("movss [rbp - " + to_string(stackOffset) + "], xmm0");
            } else if (node->type == DataType::LONG) {
                emit("mov [rbp - " + to_string(stackOffset) + "], rax");
            } else {
                emit("mov [rbp - " + to_string(stackOffset) + "], eax");
            }
        }
    }
}

void CodeGen::visitAssignStmt(AssignStmt* node) {
    if (node->isArrayAssign) {
        // Asignación a array: arr[i] = value

        // Evaluar valor primero
        node->value->accept(this);
        emit("push rax");  // Guardar valor

        // Calcular dirección del array
        VarInfo* varInfo = nullptr;
        if (localVars.find(node->varName) != localVars.end()) {
            varInfo = &localVars[node->varName];
        }

        if (!varInfo) return;

        if (node->indices.size() == 1) {
            // Array 1D
            node->indices[0]->accept(this);

            int typeSize = 4;
            if (varInfo->type == DataType::LONG) typeSize = 8;

            emit("imul rax, " + to_string(typeSize));
            emit("mov rbx, rbp");
            emit("sub rbx, " + to_string(varInfo->offset));
            emit("add rbx, rax");

            emit("pop rax");  // Recuperar valor

            if (varInfo->type == DataType::FLOAT) {
                emit("movss [rbx], xmm0");
            } else if (varInfo->type == DataType::LONG) {
                // Para long, asegurarse de extender correctamente
                emit("movsx rax, eax");
                emit("mov [rbx], rax");
            } else {
                emit("mov [rbx], eax");
            }
        } else if (node->indices.size() == 2) {
            // Array 2D
            node->indices[0]->accept(this);
            emit("imul rax, " + to_string(varInfo->dimensions[1]));
            emit("push rax");

            node->indices[1]->accept(this);
            emit("pop rbx");
            emit("add rax, rbx");

            int typeSize = 4;
            if (varInfo->type == DataType::LONG) typeSize = 8;

            emit("imul rax, " + to_string(typeSize));
            emit("mov rbx, rbp");
            emit("sub rbx, " + to_string(varInfo->offset));
            emit("add rbx, rax");

            emit("pop rax");  // Recuperar valor

            if (varInfo->type == DataType::FLOAT) {
                emit("movss [rbx], xmm0");
            } else if (varInfo->type == DataType::LONG) {
                // Para long, asegurarse de extender correctamente
                emit("movsx rax, eax");
                emit("mov [rbx], rax");
            } else {
                emit("mov [rbx], eax");
            }
        }
    } else {
        // Asignación simple: x = value
        node->value->accept(this);

        if (localVars.find(node->varName) != localVars.end()) {
            VarInfo& var = localVars[node->varName];

            if (var.type == DataType::FLOAT) {
                emit("movss [rbp - " + to_string(var.offset) + "], xmm0");
            } else if (var.type == DataType::LONG) {
                // Si el valor viene de un int, extenderlo a long
                emit("movsx rax, eax");
                emit("mov [rbp - " + to_string(var.offset) + "], rax");
            } else {
                emit("mov [rbp - " + to_string(var.offset) + "], eax");
            }
        }
    }
}

void CodeGen::visitBlock(Block* node) {
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

void CodeGen::visitIfStmt(IfStmt* node) {
    string labelElse = newLabel("else_");
    string labelEnd = newLabel("endif_");

    // Evaluar condición
    node->condition->accept(this);
    emit("test rax, rax");

    if (node->elseBranch) {
        emit("jz " + labelElse);
        node->thenBranch->accept(this);
        emit("jmp " + labelEnd);

        emitLabel(labelElse);
        node->elseBranch->accept(this);
        emitLabel(labelEnd);
    } else {
        emit("jz " + labelEnd);
        node->thenBranch->accept(this);
        emitLabel(labelEnd);
    }
}

void CodeGen::visitWhileStmt(WhileStmt* node) {
    string labelStart = newLabel("while_start_");
    string labelEnd = newLabel("while_end_");

    emitLabel(labelStart);

    // Evaluar condición
    node->condition->accept(this);
    emit("test rax, rax");
    emit("jz " + labelEnd);

    // Cuerpo del while
    node->body->accept(this);

    emit("jmp " + labelStart);
    emitLabel(labelEnd);
}

void CodeGen::visitForStmt(ForStmt* node) {
    string labelStart = newLabel("for_start_");
    string labelEnd = newLabel("for_end_");

    // Inicializador
    if (node->initializer) {
        node->initializer->accept(this);
    }

    emitLabel(labelStart);

    // Condición
    if (node->condition) {
        node->condition->accept(this);
        emit("test rax, rax");
        emit("jz " + labelEnd);
    }


    // Cuerpo
    node->body->accept(this);

    // Incremento
    if (node->increment) {
        node->increment->accept(this);
    }

    emit("jmp " + labelStart);
    emitLabel(labelEnd);
}

void CodeGen::visitReturnStmt(ReturnStmt* node) {
    if (node->value) {
        node->value->accept(this);
    }

    emitFunctionEpilog();
}

void CodeGen::visitExprStmt(ExprStmt* node) {
    node->expression->accept(this);
}

void CodeGen::visitFunctionDecl(FunctionDecl* node) {
    currentFunction = node->name;
    localVars.clear();
    stackOffset = 0;

    // Registrar función
    FunctionInfo funcInfo;
    funcInfo.returnType = node->returnType;
    for (auto& param : node->parameters) {
        funcInfo.paramTypes.push_back(param.first);
    }
    functions[node->name] = funcInfo;

    // Emitir label de función
    emitLabel(node->name);

    // Prólogo (lo completaremos después de saber el tamaño del stack)
    emit("push rbp");
    emit("mov rbp, rsp");

    // Guardar parámetros en stack
    vector<string> paramRegs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (size_t i = 0; i < node->parameters.size() && i < 6; i++) {
        auto& param = node->parameters[i];

        int size = 4;
        if (param.first == DataType::LONG) size = 8;

        stackOffset += size;

        VarInfo varInfo;
        varInfo.type = param.first;
        varInfo.offset = stackOffset;
        varInfo.isArray = false;
        localVars[param.second] = varInfo;

        if (param.first == DataType::LONG) {
            emit("mov [rbp - " + to_string(stackOffset) + "], " + paramRegs[i]);
        } else {
            // Para registros de 32 bits: rdi->edi, rsi->esi, etc.
            string reg32;
            if (paramRegs[i] == "rdi") reg32 = "edi";
            else if (paramRegs[i] == "rsi") reg32 = "esi";
            else if (paramRegs[i] == "rdx") reg32 = "edx";
            else if (paramRegs[i] == "rcx") reg32 = "ecx";
            else if (paramRegs[i] == "r8") reg32 = "r8d";
            else if (paramRegs[i] == "r9") reg32 = "r9d";

            emit("mov [rbp - " + to_string(stackOffset) + "], " + reg32);
        }
    }

    // Cuerpo de la función
    node->body->accept(this);

    // Ahora que sabemos el tamaño total del stack, reservar espacio
    // CRÍTICO: Alinear a 16 bytes considerando que push rbp ya desalineó
    int stackSize = stackOffset;

    // Después de push rbp, rsp % 16 = 8
    // Necesitamos que después de sub rsp, stackSize: rsp % 16 = 0
    // Redondear stackSize al siguiente múltiplo de 16 si no es múltiplo
    if (stackSize % 16 != 0) {
        stackSize = ((stackSize / 16) + 1) * 16;
    }

    // Insertar reserva de stack después de mov rbp, rsp de esta función
    string currentOutput = output.str();
    string funcLabel = node->name + ":";
    size_t funcLabelPos = currentOutput.rfind(funcLabel);
    if (funcLabelPos != string::npos) {
        size_t searchStart = funcLabelPos;
        size_t markerPos = currentOutput.find("mov rbp, rsp", searchStart);
        if (markerPos != string::npos) {
            size_t insertPos = currentOutput.find("\n", markerPos) + 1;
            if (stackSize > 0) {
                string reserveCode = "    sub rsp, " + to_string(stackSize) + "\n";
                currentOutput.insert(insertPos, reserveCode);
                output.str("");
                output << currentOutput;
            }
        }
    }

    // Si no hubo return explícito
    if (node->returnType == DataType::VOID) {
        emitFunctionEpilog();
    }

    output << "\n";
    currentFunction = "";
}

