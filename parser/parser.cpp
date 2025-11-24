#include "parser.h"
#include <iostream>

Parser::Parser(vector<Token> tokens) : tokens(tokens), current(0) {}

// ========== HELPERS ==========

Token Parser::peek() {
    return tokens[current];
}

Token Parser::previous() {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() {
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(vector<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, string message) {
    if (check(type)) return advance();
    error(message);
    throw runtime_error(message);
}

void Parser::error(string message) {
    Token token = peek();
    cerr << "Parse error at line " << token.line << ": " << message << endl;
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::RETURN:
            case TokenType::INT:
            case TokenType::FLOAT:
            case TokenType::LONG:
            case TokenType::UNSIGNED:
                return;
            default:
                break;
        }
        
        advance();
    }
}

DataType Parser::tokenToDataType(Token token) {
    switch(token.type) {
        case TokenType::INT: return DataType::INT;
        case TokenType::FLOAT: return DataType::FLOAT;
        case TokenType::LONG: return DataType::LONG;
        case TokenType::UNSIGNED:
            // Verifica si es "unsigned int"
            if (check(TokenType::INT)) {
                advance();
                return DataType::UNSIGNED_INT;
            }
            return DataType::UNSIGNED_INT;
        default: return DataType::UNKNOWN;
    }
}

// ========== MAIN PARSE ==========

unique_ptr<Program> Parser::parse() {
    vector<unique_ptr<Stmt>> statements;
    
    while (!isAtEnd()) {
        try {
            statements.push_back(declaration());
        } catch (runtime_error& e) {
            synchronize();
        }
    }
    
    return make_unique<Program>(move(statements));
}

// ========== DECLARATIONS ==========

unique_ptr<Stmt> Parser::declaration() {
    // Verificar si es declaración de tipo
    if (match({TokenType::INT, TokenType::FLOAT, TokenType::LONG, TokenType::UNSIGNED})) {
        Token typeToken = previous();
        DataType type = tokenToDataType(typeToken);
        
        Token name = consume(TokenType::IDENTIFIER, "Expected variable or function name.");
        
        // Es una función?
        if (check(TokenType::LPAREN)) {
            advance(); // consume '('
            
            vector<pair<DataType, string>> parameters;
            
            // Parsear parámetros
            if (!check(TokenType::RPAREN)) {
                do {
                    Token paramTypeToken = advance();
                    DataType paramType = tokenToDataType(paramTypeToken);
                    Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name.");
                    parameters.push_back({paramType, paramName.lexeme});
                } while (match({TokenType::COMMA}));
            }
            
            consume(TokenType::RPAREN, "Expected ')' after parameters.");
            
            // Cuerpo de la función
            consume(TokenType::LBRACE, "Expected '{' before function body.");
            unique_ptr<Block> body = block();
            
            unique_ptr<FunctionDecl> func = make_unique<FunctionDecl>(type, name.lexeme, parameters, move(body));
            func->line = name.line;  // Establecer línea del token
            return func;
        }
        
        // Es una variable o array
        // Es un array?
        if (match({TokenType::LBRACKET})) {
            vector<int> dimensions;
            
            // Parsear dimensiones: [3][4]
            do {
                Token sizeToken = consume(TokenType::INT_LITERAL, "Expected array size.");
                dimensions.push_back(stoi(sizeToken.lexeme));
                consume(TokenType::RBRACKET, "Expected ']'.");
            } while (match({TokenType::LBRACKET}));
            
            unique_ptr<VarDecl> varDecl = make_unique<VarDecl>(type, name.lexeme, dimensions);
            varDecl->line = name.line;  // Establecer línea del token
            
            // Inicializador de array? = {1, 2, 3}
            if (match({TokenType::ASSIGN})) {
                consume(TokenType::LBRACE, "Expected '{' for array initializer.");
                
                // Por ahora simplemente saltamos hasta }
                int braceCount = 1;
                while (braceCount > 0 && !isAtEnd()) {
                    if (check(TokenType::LBRACE)) braceCount++;
                    if (check(TokenType::RBRACE)) braceCount--;
                    advance();
                }
            }
            
            consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
            return varDecl;
        }
        
        // Variable simple con inicializador opcional
        unique_ptr<Expr> initializer = nullptr;
        if (match({TokenType::ASSIGN})) {
            initializer = expression();
        }
        
        consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
        unique_ptr<VarDecl> varDecl = make_unique<VarDecl>(type, name.lexeme, move(initializer));
        varDecl->line = name.line;  // Establecer línea del token
        return varDecl;
    }
    
    // Si no es declaración, es un statement
    return statement();
}

// ========== STATEMENTS ==========

unique_ptr<Stmt> Parser::statement() {
    if (match({TokenType::IF})) return ifStatement();
    if (match({TokenType::WHILE})) return whileStatement();
    if (match({TokenType::FOR})) return forStatement();
    if (match({TokenType::RETURN})) return returnStatement();
    if (match({TokenType::LBRACE})) return block();
    
    return exprStatement();
}

unique_ptr<Stmt> Parser::exprStatement() {
    // Manejo especial para asignaciones
    if (check(TokenType::IDENTIFIER)) {
        Token name = peek();
        int savedPos = current;
        advance();
        
        // Asignación a variable: x = expr;
        if (match({TokenType::ASSIGN, TokenType::PLUSEQ, TokenType::MINUSEQ})) {
            Token op = previous();
            unique_ptr<Expr> value = expression();
            
            // Manejar += y -=
            if (op.type == TokenType::PLUSEQ) {
                value = make_unique<BinaryOp>(
                    make_unique<Variable>(name.lexeme),
                    Token(TokenType::PLUS, "+", op.line, op.column),
                    move(value)
                );
            } else if (op.type == TokenType::MINUSEQ) {
                value = make_unique<BinaryOp>(
                    make_unique<Variable>(name.lexeme),
                    Token(TokenType::MINUS, "-", op.line, op.column),
                    move(value)
                );
            }
            
            consume(TokenType::SEMICOLON, "Expected ';' after assignment.");
            unique_ptr<AssignStmt> assign = make_unique<AssignStmt>(name.lexeme, move(value));
            assign->line = name.line;  // Establecer línea del token
            return assign;
        }
        
        // Asignación a array: arr[i] = expr;
        if (check(TokenType::LBRACKET)) {
            vector<unique_ptr<Expr>> indices;
            
            while (match({TokenType::LBRACKET})) {
                indices.push_back(expression());
                consume(TokenType::RBRACKET, "Expected ']'.");
            }
            
            if (match({TokenType::ASSIGN})) {
                unique_ptr<Expr> value = expression();
                consume(TokenType::SEMICOLON, "Expected ';' after assignment.");
                unique_ptr<AssignStmt> assign = make_unique<AssignStmt>(name.lexeme, move(indices), move(value));
                assign->line = name.line;  // Establecer línea del token
                return assign;
            }
        }
        
        // No es asignación, retroceder
        current = savedPos;
    }
    
    Token startToken = peek();  // Token al inicio de la expresión
    unique_ptr<Expr> expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    unique_ptr<ExprStmt> exprStmt = make_unique<ExprStmt>(move(expr));
    exprStmt->line = startToken.line;  // Línea del inicio de la expresión
    return exprStmt;
}

unique_ptr<Stmt> Parser::ifStatement() {
    Token ifToken = previous();  // El token 'if' que ya se consumió en statement()
    consume(TokenType::LPAREN, "Expected '(' after 'if'.");
    unique_ptr<Expr> condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after if condition.");
    
    unique_ptr<Stmt> thenBranch = statement();
    unique_ptr<Stmt> elseBranch = nullptr;
    
    if (match({TokenType::ELSE})) {
        elseBranch = statement();
    }
    
    unique_ptr<IfStmt> ifStmt = make_unique<IfStmt>(move(condition), move(thenBranch), move(elseBranch));
    ifStmt->line = ifToken.line;
    return ifStmt;
}

unique_ptr<Stmt> Parser::whileStatement() {
    Token whileToken = previous();  // El token 'while' que ya se consumió
    consume(TokenType::LPAREN, "Expected '(' after 'while'.");
    unique_ptr<Expr> condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after while condition.");
    
    unique_ptr<Stmt> body = statement();
    
    unique_ptr<WhileStmt> whileStmt = make_unique<WhileStmt>(move(condition), move(body));
    whileStmt->line = whileToken.line;
    return whileStmt;
}
unique_ptr<Stmt> Parser::forStatement() {
    Token forToken = previous();  // El token 'for' que ya se consumió
    consume(TokenType::LPAREN, "Expected '(' after 'for'.");

    // Initializer: int i = 0 o i = 0
    unique_ptr<Stmt> initializer = nullptr;
    if (match({TokenType::INT, TokenType::FLOAT, TokenType::LONG, TokenType::UNSIGNED})) {
        Token typeToken = previous();
        DataType type = tokenToDataType(typeToken);
        Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");

        unique_ptr<Expr> init = nullptr;
        if (match({TokenType::ASSIGN})) {
            init = expression();
        }
        consume(TokenType::SEMICOLON, "Expected ';' after for initializer.");
        initializer = make_unique<VarDecl>(type, name.lexeme, move(init));
        initializer->line = name.line;  // Establecer línea
    } else if (!check(TokenType::SEMICOLON)) {
        initializer = exprStatement();
    } else {
        advance(); // Consume ';'
    }

    // Condition: i < 10
    unique_ptr<Expr> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = expression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after for condition.");

    // Increment: i++ (expresión normal)
    unique_ptr<Expr> increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = expression();
    }
    consume(TokenType::RPAREN, "Expected ')' after for clauses.");

    unique_ptr<Stmt> body = statement();

    unique_ptr<ForStmt> forStmt = make_unique<ForStmt>(move(initializer), move(condition), move(increment), move(body));
    forStmt->line = forToken.line;
    return forStmt;
}

unique_ptr<Stmt> Parser::returnStatement() {
    Token returnToken = previous();  // El token 'return' que ya se consumió
    unique_ptr<Expr> value = nullptr;
    
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return value.");
    unique_ptr<ReturnStmt> retStmt = make_unique<ReturnStmt>(move(value));
    retStmt->line = returnToken.line;
    return retStmt;
}

unique_ptr<Block> Parser::block() {
    vector<unique_ptr<Stmt>> statements;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }
    
    consume(TokenType::RBRACE, "Expected '}' after block.");
    return make_unique<Block>(move(statements));
}

// ========== EXPRESSIONS (Precedencia descendente) ==========

unique_ptr<Expr> Parser::expression() {
    return assignment();
}

unique_ptr<Expr> Parser::assignment() {
    // Primero intentar parsear una expresión de menor precedencia
    unique_ptr<Expr> expr = ternary();
    
    // Si encontramos un ASSIGN, entonces es una asignación
    // En C, las asignaciones son expresiones que retornan el valor asignado
    if (match({TokenType::ASSIGN, TokenType::PLUSEQ, TokenType::MINUSEQ})) {
        Token op = previous();
        unique_ptr<Expr> value = assignment(); // Asociatividad a la derecha
        
        // Verificar que el lado izquierdo es una variable
        Variable* var = dynamic_cast<Variable*>(expr.get());
        if (!var) {
            error("Left side of assignment must be a variable.");
            throw runtime_error("Left side of assignment must be a variable.");
        }
        
        // Manejar += y -=
        if (op.type == TokenType::PLUSEQ) {
            value = make_unique<BinaryOp>(
                make_unique<Variable>(var->name),
                Token(TokenType::PLUS, "+", op.line, op.column),
                move(value)
            );
        } else if (op.type == TokenType::MINUSEQ) {
            value = make_unique<BinaryOp>(
                make_unique<Variable>(var->name),
                Token(TokenType::MINUS, "-", op.line, op.column),
                move(value)
            );
        }
        
        // Crear una expresión de asignación que retorna el valor asignado
        unique_ptr<AssignExpr> assignExpr = make_unique<AssignExpr>(var->name, move(value));
        assignExpr->line = op.line;  // Línea del operador de asignación
        return assignExpr;
    }
    
    // Verificar si es asignación a array: arr[i] = expr
    if (ArrayAccess* arrAccess = dynamic_cast<ArrayAccess*>(expr.get())) {
        if (match({TokenType::ASSIGN})) {
            Token assignToken = previous();
            unique_ptr<Expr> value = assignment();
            unique_ptr<AssignExpr> assignExpr = make_unique<AssignExpr>(arrAccess->arrayName, move(arrAccess->indices), move(value));
            assignExpr->line = assignToken.line;
            return assignExpr;
        }
    }
    
    return expr;
}

unique_ptr<Expr> Parser::ternary() {
    unique_ptr<Expr> expr = logicalOr();
    
    // Operador ternario: condition ? exprTrue : exprFalse
    // Por ahora lo omitimos hasta tener el token '?'
    // TODO: Implementar cuando agreguemos QUESTION y COLON tokens
    
    return expr;
}

unique_ptr<Expr> Parser::logicalOr() {
    unique_ptr<Expr> expr = logicalAnd();
    
    while (match({TokenType::OR})) {
        Token op = previous();
        unique_ptr<Expr> right = logicalAnd();
        expr = make_unique<BinaryOp>(move(expr), op, move(right));
    }
    
    return expr;
}

unique_ptr<Expr> Parser::logicalAnd() {
    unique_ptr<Expr> expr = equality();
    
    while (match({TokenType::AND})) {
        Token op = previous();
        unique_ptr<Expr> right = equality();
        expr = make_unique<BinaryOp>(move(expr), op, move(right));
    }
    
    return expr;
}

unique_ptr<Expr> Parser::equality() {
    unique_ptr<Expr> expr = comparison();
    
    while (match({TokenType::EQ, TokenType::NE})) {
        Token op = previous();
        unique_ptr<Expr> right = comparison();
        expr = make_unique<BinaryOp>(move(expr), op, move(right));
    }
    
    return expr;
}

unique_ptr<Expr> Parser::comparison() {
    unique_ptr<Expr> expr = term();
    
    while (match({TokenType::LT, TokenType::GT, TokenType::LE, TokenType::GE})) {
        Token op = previous();
        unique_ptr<Expr> right = term();
        expr = make_unique<BinaryOp>(move(expr), op, move(right));
    }
    
    return expr;
}

unique_ptr<Expr> Parser::term() {
    unique_ptr<Expr> expr = factor();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token op = previous();
        unique_ptr<Expr> right = factor();
        expr = make_unique<BinaryOp>(move(expr), op, move(right));
    }
    
    return expr;
}

unique_ptr<Expr> Parser::factor() {
    unique_ptr<Expr> expr = unary();
    
    while (match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO})) {
        Token op = previous();
        unique_ptr<Expr> right = unary();
        expr = make_unique<BinaryOp>(move(expr), op, move(right));
    }
    
    return expr;
}

unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::MINUS, TokenType::NOT})) {
        Token op = previous();
        unique_ptr<Expr> right = unary();
        return make_unique<UnaryOp>(op, move(right));
    }
    
    return cast();
}

unique_ptr<Expr> Parser::cast() {
    // Casting: (float)x, (int)y
    if (check(TokenType::LPAREN)) {
        int savedPos = current;
        advance(); // consume '('
        
        // Verificar si es un tipo
        if (match({TokenType::INT, TokenType::FLOAT, TokenType::LONG, TokenType::UNSIGNED})) {
            Token typeToken = previous();
            DataType targetType = tokenToDataType(typeToken);
            
            if (match({TokenType::RPAREN})) {
                unique_ptr<Expr> expr = cast();
                unique_ptr<CastExpr> castExpr = make_unique<CastExpr>(targetType, move(expr));
                castExpr->line = typeToken.line;  // Línea del token de tipo
                return castExpr;
            }
        }
        
        // No es un cast, retroceder
        current = savedPos;
    }
    
    return postfix();
}

unique_ptr<Expr> Parser::postfix() {
    unique_ptr<Expr> expr = primary();
    
    // Array access: arr[i][j]
    if (Variable* var = dynamic_cast<Variable*>(expr.get())) {
        if (check(TokenType::LBRACKET)) {
            string arrayName = var->name;
            vector<unique_ptr<Expr>> indices;
            
            Token bracketToken = peek();  // Token del primer '['
            while (match({TokenType::LBRACKET})) {
                indices.push_back(expression());
                consume(TokenType::RBRACKET, "Expected ']'.");
            }
            
            unique_ptr<ArrayAccess> arrAccess = make_unique<ArrayAccess>(arrayName, move(indices));
            arrAccess->line = bracketToken.line;  // Línea del primer '['
            expr = move(arrAccess);
        }
    }
    
    // Postfix increment/decrement: i++, i--
    while (match({TokenType::INCREMENT, TokenType::DECREMENT})) {
        Token op = previous();
        // i++ se convierte en: i = i + 1
        // i-- se convierte en: i = i - 1
        if (Variable* var = dynamic_cast<Variable*>(expr.get())) {
            TokenType binOp = (op.type == TokenType::INCREMENT) ? TokenType::PLUS : TokenType::MINUS;
            unique_ptr<Expr> one = make_unique<IntLiteral>(1);
            one->line = op.line;
            unique_ptr<BinaryOp> addExpr = make_unique<BinaryOp>(
                make_unique<Variable>(var->name),
                Token(binOp, op.type == TokenType::INCREMENT ? "+" : "-", op.line, op.column),
                move(one)
            );
            addExpr->line = op.line;
            
            // Crear asignación: i = i + 1
            unique_ptr<AssignExpr> assignExpr = make_unique<AssignExpr>(var->name, move(addExpr));
            assignExpr->line = op.line;
            expr = move(assignExpr);
        } else {
            // Si no es una variable, crear UnaryOp (aunque esto no es común en C)
            expr = make_unique<UnaryOp>(op, move(expr));
        }
    }
    
    return expr;
}

unique_ptr<Expr> Parser::primary() {
    // Literales numéricos
    if (match({TokenType::INT_LITERAL})) {
        Token token = previous();
        unique_ptr<IntLiteral> lit = make_unique<IntLiteral>(stoi(token.lexeme));
        lit->line = token.line;
        return lit;
    }
    
    if (match({TokenType::FLOAT_LITERAL})) {
        Token token = previous();
        unique_ptr<FloatLiteral> lit = make_unique<FloatLiteral>(stof(token.lexeme));
        lit->line = token.line;
        return lit;
    }
    
    if (match({TokenType::LONG_LITERAL})) {
        Token token = previous();
        string lexeme = token.lexeme;
        // Remover el sufijo 'L'
        if (lexeme.back() == 'L' || lexeme.back() == 'l') {
            lexeme.pop_back();
        }
        unique_ptr<LongLiteral> lit = make_unique<LongLiteral>(stol(lexeme));
        lit->line = token.line;
        return lit;
    }
    
    // String literal
    if (match({TokenType::STRING_LITERAL})) {
        Token token = previous();
        unique_ptr<StringLiteral> lit = make_unique<StringLiteral>(token.lexeme);
        lit->line = token.line;
        return lit;
    }
    
    // Identificadores (variables o llamadas a función)
    // También reconocer PRINTF como identificador para llamadas a función
    if (match({TokenType::IDENTIFIER, TokenType::PRINTF})) {
        Token name = previous();
        
        // Si es PRINTF, cambiar el lexeme a "printf"
        if (name.type == TokenType::PRINTF) {
            name.lexeme = "printf";
        }
        
        // Llamada a función
        if (match({TokenType::LPAREN})) {
            vector<unique_ptr<Expr>> arguments;
            
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(expression());
                } while (match({TokenType::COMMA}));
            }
            
            consume(TokenType::RPAREN, "Expected ')' after arguments.");
            unique_ptr<CallExpr> call = make_unique<CallExpr>(name.lexeme, move(arguments));
            call->line = name.line;
            return call;
        }
        
        // Variable simple
        unique_ptr<Variable> var = make_unique<Variable>(name.lexeme);
        var->line = name.line;
        return var;
    }
    
    // Expresiones entre paréntesis
    if (match({TokenType::LPAREN})) {
        unique_ptr<Expr> expr = expression();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return expr;
    }
    
    error("Expected expression.");
    throw runtime_error("Expected expression.");
}

