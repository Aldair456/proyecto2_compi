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
            
            return make_unique<FunctionDecl>(type, name.lexeme, parameters, move(body));
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
        return make_unique<VarDecl>(type, name.lexeme, move(initializer));
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
            return make_unique<AssignStmt>(name.lexeme, move(value));
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
                return make_unique<AssignStmt>(name.lexeme, move(indices), move(value));
            }
        }
        
        // No es asignación, retroceder
        current = savedPos;
    }
    
    unique_ptr<Expr> expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return make_unique<ExprStmt>(move(expr));
}

unique_ptr<Stmt> Parser::ifStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'if'.");
    unique_ptr<Expr> condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after if condition.");
    
    unique_ptr<Stmt> thenBranch = statement();
    unique_ptr<Stmt> elseBranch = nullptr;
    
    if (match({TokenType::ELSE})) {
        elseBranch = statement();
    }
    
    return make_unique<IfStmt>(move(condition), move(thenBranch), move(elseBranch));
}

unique_ptr<Stmt> Parser::whileStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'while'.");
    unique_ptr<Expr> condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after while condition.");
    
    unique_ptr<Stmt> body = statement();
    
    return make_unique<WhileStmt>(move(condition), move(body));
}
unique_ptr<Stmt> Parser::forStatement() {
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

    return make_unique<ForStmt>(move(initializer), move(condition), move(increment), move(body));
}

unique_ptr<Stmt> Parser::returnStatement() {
    unique_ptr<Expr> value = nullptr;
    
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return value.");
    return make_unique<ReturnStmt>(move(value));
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
        return make_unique<AssignExpr>(var->name, move(value));
    }
    
    // Verificar si es asignación a array: arr[i] = expr
    if (ArrayAccess* arrAccess = dynamic_cast<ArrayAccess*>(expr.get())) {
        if (match({TokenType::ASSIGN})) {
            unique_ptr<Expr> value = assignment();
            return make_unique<AssignExpr>(arrAccess->arrayName, move(arrAccess->indices), move(value));
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
                return make_unique<CastExpr>(targetType, move(expr));
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
            
            while (match({TokenType::LBRACKET})) {
                indices.push_back(expression());
                consume(TokenType::RBRACKET, "Expected ']'.");
            }
            
            return make_unique<ArrayAccess>(arrayName, move(indices));
        }
    }
    
    return expr;
}

unique_ptr<Expr> Parser::primary() {
    // Literales numéricos
    if (match({TokenType::INT_LITERAL})) {
        return make_unique<IntLiteral>(stoi(previous().lexeme));
    }
    
    if (match({TokenType::FLOAT_LITERAL})) {
        return make_unique<FloatLiteral>(stof(previous().lexeme));
    }
    
    if (match({TokenType::LONG_LITERAL})) {
        string lexeme = previous().lexeme;
        // Remover el sufijo 'L'
        if (lexeme.back() == 'L' || lexeme.back() == 'l') {
            lexeme.pop_back();
        }
        return make_unique<LongLiteral>(stol(lexeme));
    }
    
    // String literal
    if (match({TokenType::STRING_LITERAL})) {
        return make_unique<StringLiteral>(previous().lexeme);
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
            return make_unique<CallExpr>(name.lexeme, move(arguments));
        }
        
        // Variable simple
        return make_unique<Variable>(name.lexeme);
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

