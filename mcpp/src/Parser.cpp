#include "../include/Parser.h"

#include <algorithm>
#include <memory>

namespace {
    bool containsName(const std::vector<std::string>& names, const std::string& value) {
        return std::find(names.begin(), names.end(), value) != names.end();
    }

    std::string prefixNestedParseError(const std::string& nestedError, const std::string& prefixMessage) {
        const std::string parsePrefix = "Parse error at ";
        const std::string separator = ": ";

        if (nestedError.rfind(parsePrefix, 0) != 0) {
            return prefixMessage + " " + nestedError;
        }

        const size_t separatorIndex = nestedError.find(separator, parsePrefix.size());
        if (separatorIndex == std::string::npos) {
            return prefixMessage + " " + nestedError;
        }

        return nestedError.substr(0, separatorIndex + separator.size()) +
            prefixMessage + " " +
            nestedError.substr(separatorIndex + separator.size());
    }
}

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens) {
}

bool Parser::parseProgram(Program& outProgram, std::string& outError) {
    Program parsed;

    while (!isAtEnd()) {
        skipNewlines();

        if (isAtEnd()) {
            break;
        }

        if (check(TokenKind::RightBrace)) {
            outError = makeError(peek().location, "Unmatched closing brace.");
            return false;
        }

        if (isCommandBlockStart()) {
            if (!parseTopLevelCommandBlock(parsed, outError)) {
                return false;
            }
            continue;
        }

        if (check(TokenKind::Slash)) {
            StatementPtr statement;
            if (!parseCommandStatement(statement, false, outError)) {
                return false;
            }

            if (statement != nullptr) {
                parsed.topLevelStatements.push_back(std::move(statement));
            }
            continue;
        }

        skipLine();
    }

    outProgram = std::move(parsed);
    return true;
}

const Token& Parser::peek(size_t offset) const {
    const size_t index = position + offset;
    if (index >= tokens.size()) {
        return tokens.back();
    }
    return tokens[index];
}

bool Parser::isAtEnd() const {
    return check(TokenKind::EndOfFile);
}

bool Parser::check(TokenKind kind, size_t offset) const {
    return peek(offset).kind == kind;
}

bool Parser::checkIdentifier(const std::string& text, size_t offset) const {
    return check(TokenKind::Identifier, offset) && peek(offset).lexeme == text;
}

bool Parser::match(TokenKind kind) {
    if (!check(kind)) {
        return false;
    }

    position++;
    return true;
}

bool Parser::matchIdentifier(const std::string& text) {
    if (!checkIdentifier(text)) {
        return false;
    }

    position++;
    return true;
}

void Parser::skipNewlines() {
    while (match(TokenKind::Newline)) {
    }
}

void Parser::skipLine() {
    while (!check(TokenKind::Newline) && !isAtEnd()) {
        position++;
    }

    match(TokenKind::Newline);
}

std::string Parser::makeError(const SourceLocation& location, const std::string& message) const {
    return "Parse error at " + formatLocation(location) + ": " + message;
}

bool Parser::expect(TokenKind kind, const std::string& message, std::string& outError) {
    if (match(kind)) {
        return true;
    }

    outError = makeError(peek().location, message);
    return false;
}

bool Parser::expectIdentifier(const std::string& text, std::string& outError) {
    if (matchIdentifier(text)) {
        return true;
    }

    outError = makeError(peek().location, "Expected '" + text + "'.");
    return false;
}

bool Parser::expectName(std::string& outName, const std::string& message, std::string& outError) {
    if (!check(TokenKind::Identifier)) {
        outError = makeError(peek().location, message);
        return false;
    }

    outName = peek().lexeme;
    position++;
    return true;
}

bool Parser::consumeLineEnd(std::string& outError) {
    if (match(TokenKind::Newline) || isAtEnd()) {
        return true;
    }

    outError = makeError(peek().location, "Expected end of line.");
    return false;
}

bool Parser::isCommandBlockStart(size_t offset) const {
    return check(TokenKind::Slash, offset) &&
        checkIdentifier("summon", offset + 1) &&
        checkIdentifier("command_block", offset + 2) &&
        check(TokenKind::LeftBrace, offset + 3);
}

bool Parser::looksLikeFunctionHeader() const {
    size_t offset = 0;
    while (check(TokenKind::Newline, offset)) {
        offset++;
    }

    if (!check(TokenKind::Slash, offset) ||
        !checkIdentifier("function", offset + 1) ||
        !check(TokenKind::Identifier, offset + 2) ||
        !check(TokenKind::LeftBrace, offset + 3)) {
        return false;
    }

    offset += 4;
    if (check(TokenKind::RightBrace, offset)) {
        offset++;
        return check(TokenKind::Newline, offset) || check(TokenKind::EndOfFile, offset);
    }

    while (true) {
        if (!check(TokenKind::Identifier, offset)) {
            return false;
        }

        offset++;
        if (check(TokenKind::RightBrace, offset)) {
            offset++;
            return check(TokenKind::Newline, offset) || check(TokenKind::EndOfFile, offset);
        }

        if (!check(TokenKind::Comma, offset)) {
            return false;
        }

        offset++;
    }
}

bool Parser::consumeCommandBlockStart(std::string& outError) {
    if (!expect(TokenKind::Slash, "Expected '/' before command block.", outError)) {
        return false;
    }

    if (!expectIdentifier("summon", outError)) {
        return false;
    }

    if (!expectIdentifier("command_block", outError)) {
        return false;
    }

    if (!expect(TokenKind::LeftBrace, "Expected '{' after '/summon command_block'.", outError)) {
        return false;
    }

    if (check(TokenKind::Newline)) {
        match(TokenKind::Newline);
        return true;
    }

    if (check(TokenKind::RightBrace) || isAtEnd()) {
        return true;
    }

    outError = makeError(peek().location, "Expected end of line after '{'.");
    return false;
}

bool Parser::parseBlockContents(
    std::vector<StatementPtr>& outStatements,
    bool isFunctionContext,
    std::string& outError
) {
    while (true) {
        skipNewlines();

        if (isAtEnd()) {
            outError = makeError(peek().location, "Command block was never closed.");
            return false;
        }

        if (match(TokenKind::RightBrace)) {
            if (match(TokenKind::Newline) || isAtEnd()) {
                return true;
            }

            outError = makeError(peek().location, "Expected end of line after '}'.");
            return false;
        }

        StatementPtr statement;
        if (!parseStatement(statement, isFunctionContext, outError)) {
            return false;
        }

        if (statement != nullptr) {
            outStatements.push_back(std::move(statement));
        }
    }
}

bool Parser::parseTopLevelCommandBlock(Program& outProgram, std::string& outError) {
    const SourceLocation blockLocation = peek().location;

    if (!consumeCommandBlockStart(outError)) {
        return false;
    }

    skipNewlines();

    if (looksLikeFunctionHeader()) {
        FunctionDefinition function;
        function.location = blockLocation;

        if (!parseFunctionDefinition(function, outError)) {
            return false;
        }

        outProgram.functions.push_back(std::move(function));
        return true;
    }

    BlockStatement block;
    if (!parseBlockContents(block.statements, false, outError)) {
        return false;
    }

    auto statement = std::make_unique<Statement>();
    statement->location = blockLocation;
    statement->node = std::move(block);
    outProgram.topLevelStatements.push_back(std::move(statement));
    return true;
}

bool Parser::parseFunctionDefinition(FunctionDefinition& outFunction, std::string& outError) {
    skipNewlines();
    outFunction.location = peek().location;

    if (!parseFunctionHeaderLine(outFunction.name, outFunction.parameters, outError)) {
        return false;
    }

    return parseBlockContents(outFunction.body, true, outError);
}

bool Parser::parseFunctionHeaderLine(
    std::string& outName,
    std::vector<std::string>& outParameters,
    std::string& outError
) {
    if (!expect(TokenKind::Slash, "Expected function header.", outError)) {
        return false;
    }

    if (!expectIdentifier("function", outError)) {
        return false;
    }

    if (!expectName(outName, "Expected function name.", outError)) {
        return false;
    }

    if (!expect(TokenKind::LeftBrace, "Expected '{' after function name.", outError)) {
        return false;
    }

    outParameters.clear();

    if (!match(TokenKind::RightBrace)) {
        while (true) {
            const SourceLocation parameterLocation = peek().location;

            std::string parameterName;
            if (!expectName(parameterName, "Expected parameter name.", outError)) {
                return false;
            }

            if (containsName(outParameters, parameterName)) {
                outError = makeError(parameterLocation, "Duplicate parameter '" + parameterName + "'.");
                return false;
            }

            outParameters.push_back(parameterName);

            if (match(TokenKind::RightBrace)) {
                break;
            }

            if (!expect(TokenKind::Comma, "Expected ',' or '}' in parameter list.", outError)) {
                return false;
            }
        }
    }

    return consumeLineEnd(outError);
}

bool Parser::parseStatement(StatementPtr& outStatement, bool isFunctionContext, std::string& outError) {
    outStatement.reset();

    if (check(TokenKind::RightBrace)) {
        outError = makeError(peek().location, "Unmatched closing brace.");
        return false;
    }

    if (isCommandBlockStart()) {
        return parseGenericCommandBlock(outStatement, outError);
    }

    if (check(TokenKind::Slash)) {
        return parseCommandStatement(outStatement, isFunctionContext, outError);
    }

    skipLine();
    return true;
}

bool Parser::parseGenericCommandBlock(StatementPtr& outStatement, std::string& outError) {
    const SourceLocation location = peek().location;

    if (!consumeCommandBlockStart(outError)) {
        return false;
    }

    BlockStatement block;
    if (!parseBlockContents(block.statements, false, outError)) {
        return false;
    }

    auto statement = std::make_unique<Statement>();
    statement->location = location;
    statement->node = std::move(block);
    outStatement = std::move(statement);
    return true;
}

bool Parser::parseCommandStatement(StatementPtr& outStatement, bool isFunctionContext, std::string& outError) {
    const SourceLocation slashLocation = peek().location;

    if (!expect(TokenKind::Slash, "Expected command.", outError)) {
        return false;
    }

    return parseCommandAfterSlash(slashLocation, outStatement, isFunctionContext, outError);
}

bool Parser::parseCommandAfterSlash(
    const SourceLocation& slashLocation,
    StatementPtr& outStatement,
    bool isFunctionContext,
    std::string& outError
) {
    if (matchIdentifier("stop")) {
        if (!consumeLineEnd(outError)) {
            return false;
        }

        auto statement = std::make_unique<Statement>();
        statement->location = slashLocation;
        statement->node = StopStatement{};
        outStatement = std::move(statement);
        return true;
    }

    if (matchIdentifier("say")) {
        ExprPtr expression;
        if (!parseExpression(expression, outError)) {
            return false;
        }

        if (!consumeLineEnd(outError)) {
            return false;
        }

        auto statement = std::make_unique<Statement>();
        statement->location = slashLocation;
        statement->node = SayStatement{ std::move(expression) };
        outStatement = std::move(statement);
        return true;
    }

    if (matchIdentifier("give")) {
        std::string variableName;
        if (!expectName(variableName, "Expected variable name after '/give'.", outError)) {
            return false;
        }

        ExprPtr expression;
        if (!parseExpression(expression, outError)) {
            return false;
        }

        if (!consumeLineEnd(outError)) {
            return false;
        }

        auto statement = std::make_unique<Statement>();
        statement->location = slashLocation;
        statement->node = GiveStatement{ variableName, std::move(expression) };
        outStatement = std::move(statement);
        return true;
    }

    if (matchIdentifier("execute")) {
        if (matchIdentifier("store")) {
            if (!expectIdentifier("result", outError)) {
                return false;
            }

            return parseExecuteStoreResult(slashLocation, outStatement, isFunctionContext, outError);
        }

        if (matchIdentifier("if")) {
            return parseExecuteConditionRun(
                slashLocation,
                outStatement,
                ExecuteConditionMode::If,
                isFunctionContext,
                outError
            );
        }

        if (matchIdentifier("unless")) {
            return parseExecuteConditionRun(
                slashLocation,
                outStatement,
                ExecuteConditionMode::Unless,
                isFunctionContext,
                outError
            );
        }

        if (matchIdentifier("until")) {
            return parseExecuteConditionRun(
                slashLocation,
                outStatement,
                ExecuteConditionMode::Until,
                isFunctionContext,
                outError
            );
        }

        outError = makeError(
            peek().location,
            "Expected 'store', 'if', 'unless', or 'until' after '/execute'."
        );
        return false;
    }

    if (matchIdentifier("function")) {
        return parseFunctionCallStatement(slashLocation, outStatement, outError);
    }

    if (matchIdentifier("msg")) {
        std::string variableName;
        if (!expectName(variableName, "Expected variable name after '/msg'.", outError)) {
            return false;
        }

        if (!consumeLineEnd(outError)) {
            return false;
        }

        auto statement = std::make_unique<Statement>();
        statement->location = slashLocation;
        statement->node = MsgStatement{ variableName };
        outStatement = std::move(statement);
        return true;
    }

    if (matchIdentifier("gamerule")) {
        const SourceLocation ruleLocation = peek().location;

        std::string ruleName;
        if (!expectName(ruleName, "Expected gamerule name after '/gamerule'.", outError)) {
            return false;
        }

        if (ruleName != "keep_inventory") {
            outError = makeError(ruleLocation, "Unknown gamerule '" + ruleName + "'.");
            return false;
        }

        bool ruleValue = false;
        if (matchIdentifier("true")) {
            ruleValue = true;
        }
        else if (matchIdentifier("false")) {
            ruleValue = false;
        }
        else {
            outError = makeError(peek().location, "Expected 'true' or 'false' after gamerule name.");
            return false;
        }

        if (!consumeLineEnd(outError)) {
            return false;
        }

        auto statement = std::make_unique<Statement>();
        statement->location = slashLocation;
        statement->node = GameruleStatement{ ruleName, ruleValue };
        outStatement = std::move(statement);
        return true;
    }

    outError = makeError(peek().location, "Unknown command.");
    return false;
}

bool Parser::parseExecuteStoreResult(
    const SourceLocation& slashLocation,
    StatementPtr& outStatement,
    bool isFunctionContext,
    std::string& outError
) {
    if (check(TokenKind::Identifier) && checkIdentifier("run", 1)) {
        std::string variableName;
        if (!expectName(variableName, "Expected target variable name.", outError)) {
            return false;
        }

        if (!expectIdentifier("run", outError)) {
            return false;
        }

        StatementPtr nestedCommand;
        if (!parseRunCommand(nestedCommand, isFunctionContext, outError)) {
            return false;
        }

        auto statement = std::make_unique<Statement>();
        statement->location = slashLocation;
        statement->node = StoreResultStatement{ variableName, std::move(nestedCommand) };
        outStatement = std::move(statement);
        return true;
    }

    ExprPtr expression;
    if (!parseExpression(expression, outError)) {
        return false;
    }

    if (!consumeLineEnd(outError)) {
        return false;
    }

    auto statement = std::make_unique<Statement>();
    statement->location = slashLocation;
    statement->node = ReturnStatement{ std::move(expression) };
    outStatement = std::move(statement);
    return true;
}

bool Parser::parseExecuteConditionRun(
    const SourceLocation& slashLocation,
    StatementPtr& outStatement,
    ExecuteConditionMode mode,
    bool isFunctionContext,
    std::string& outError
) {
    ExprPtr condition;
    if (!parseExpression(condition, outError)) {
        return false;
    }

    if (!expectIdentifier("run", outError)) {
        return false;
    }

    StatementPtr nestedCommand;
    if (!parseRunCommand(nestedCommand, isFunctionContext, outError)) {
        return false;
    }

    auto statement = std::make_unique<Statement>();
    statement->location = slashLocation;
    statement->node = ExecuteConditionStatement{ mode, std::move(condition), std::move(nestedCommand) };
    outStatement = std::move(statement);
    return true;
}

bool Parser::parseRunCommand(
    StatementPtr& outStatement,
    bool isFunctionContext,
    std::string& outError
) {
    const SourceLocation nestedSlashLocation = peek().location;
    if (!expect(TokenKind::Slash, "Expected command after 'run'.", outError)) {
        return false;
    }

    if (checkIdentifier("summon") && checkIdentifier("command_block", 1)) {
        outError = makeError(peek().location, "Command blocks cannot be used after 'run'.");
        return false;
    }

    StatementPtr nestedCommand;
    if (!parseCommandAfterSlash(nestedSlashLocation, nestedCommand, isFunctionContext, outError)) {
        outError = makeRunCommandError(outError);
        return false;
    }

    outStatement = std::move(nestedCommand);
    return true;
}

std::string Parser::makeRunCommandError(const std::string& nestedError) const {
    return prefixNestedParseError(nestedError, "Invalid command after 'run'.");
}

bool Parser::parseFunctionCallStatement(
    const SourceLocation& slashLocation,
    StatementPtr& outStatement,
    std::string& outError
) {
    std::string functionName;
    if (!expectName(functionName, "Expected function name after '/function'.", outError)) {
        return false;
    }

    if (!expect(TokenKind::LeftBrace, "Expected '{' after function name.", outError)) {
        return false;
    }

    std::vector<NamedArgument> arguments;

    if (!match(TokenKind::RightBrace)) {
        while (true) {
            const SourceLocation argumentLocation = peek().location;

            std::string argumentName;
            if (!expectName(argumentName, "Expected named argument.", outError)) {
                return false;
            }

            bool duplicate = false;
            for (const NamedArgument& existing : arguments) {
                if (existing.name == argumentName) {
                    duplicate = true;
                    break;
                }
            }

            if (duplicate) {
                outError = makeError(argumentLocation, "Duplicate argument '" + argumentName + "'.");
                return false;
            }

            if (!expect(TokenKind::Colon, "Expected ':' after argument name.", outError)) {
                return false;
            }

            ExprPtr expression;
            if (!parseExpression(expression, outError)) {
                return false;
            }

            arguments.push_back(NamedArgument{ argumentName, std::move(expression) });

            if (match(TokenKind::RightBrace)) {
                break;
            }

            if (!expect(TokenKind::Comma, "Expected ',' or '}' in argument list.", outError)) {
                return false;
            }
        }
    }

    if (!consumeLineEnd(outError)) {
        return false;
    }

    auto statement = std::make_unique<Statement>();
    statement->location = slashLocation;
    statement->node = FunctionCallStatement{ functionName, std::move(arguments) };
    outStatement = std::move(statement);
    return true;
}

bool Parser::parseExpression(ExprPtr& outExpression, std::string& outError) {
    return parseEquality(outExpression, outError);
}

bool Parser::parseEquality(ExprPtr& outExpression, std::string& outError) {
    if (!parseComparison(outExpression, outError)) {
        return false;
    }

    while (true) {
        BinaryOp op;
        if (match(TokenKind::EqualEqual)) {
            op = BinaryOp::Equal;
        }
        else if (match(TokenKind::BangEqual)) {
            op = BinaryOp::NotEqual;
        }
        else {
            break;
        }

        ExprPtr right;
        if (!parseComparison(right, outError)) {
            return false;
        }

        auto expression = std::make_unique<Expr>();
        expression->location = outExpression->location;
        expression->node = BinaryExpr{ op, std::move(outExpression), std::move(right) };
        outExpression = std::move(expression);
    }

    return true;
}

bool Parser::parseComparison(ExprPtr& outExpression, std::string& outError) {
    if (!parseAddSub(outExpression, outError)) {
        return false;
    }

    while (true) {
        BinaryOp op;
        if (match(TokenKind::Less)) {
            op = BinaryOp::Less;
        }
        else if (match(TokenKind::LessEqual)) {
            op = BinaryOp::LessEqual;
        }
        else if (match(TokenKind::Greater)) {
            op = BinaryOp::Greater;
        }
        else if (match(TokenKind::GreaterEqual)) {
            op = BinaryOp::GreaterEqual;
        }
        else {
            break;
        }

        ExprPtr right;
        if (!parseAddSub(right, outError)) {
            return false;
        }

        auto expression = std::make_unique<Expr>();
        expression->location = outExpression->location;
        expression->node = BinaryExpr{ op, std::move(outExpression), std::move(right) };
        outExpression = std::move(expression);
    }

    return true;
}

bool Parser::parseAddSub(ExprPtr& outExpression, std::string& outError) {
    if (!parseMulDiv(outExpression, outError)) {
        return false;
    }

    while (true) {
        BinaryOp op;
        if (match(TokenKind::Plus)) {
            op = BinaryOp::Add;
        }
        else if (match(TokenKind::Minus)) {
            op = BinaryOp::Subtract;
        }
        else {
            break;
        }

        ExprPtr right;
        if (!parseMulDiv(right, outError)) {
            return false;
        }

        auto expression = std::make_unique<Expr>();
        expression->location = outExpression->location;
        expression->node = BinaryExpr{ op, std::move(outExpression), std::move(right) };
        outExpression = std::move(expression);
    }

    return true;
}

bool Parser::parseMulDiv(ExprPtr& outExpression, std::string& outError) {
    if (!parseUnary(outExpression, outError)) {
        return false;
    }

    while (true) {
        BinaryOp op;
        if (match(TokenKind::Star)) {
            op = BinaryOp::Multiply;
        }
        else if (match(TokenKind::Slash)) {
            op = BinaryOp::Divide;
        }
        else {
            break;
        }

        ExprPtr right;
        if (!parseUnary(right, outError)) {
            return false;
        }

        auto expression = std::make_unique<Expr>();
        expression->location = outExpression->location;
        expression->node = BinaryExpr{ op, std::move(outExpression), std::move(right) };
        outExpression = std::move(expression);
    }

    return true;
}

bool Parser::parseUnary(ExprPtr& outExpression, std::string& outError) {
    if (check(TokenKind::Minus)) {
        const SourceLocation location = peek().location;
        match(TokenKind::Minus);

        ExprPtr operand;
        if (!parseUnary(operand, outError)) {
            return false;
        }

        auto expression = std::make_unique<Expr>();
        expression->location = location;
        expression->node = UnaryExpr{ UnaryOp::Negate, std::move(operand) };
        outExpression = std::move(expression);
        return true;
    }

    return parsePrimary(outExpression, outError);
}

bool Parser::parsePrimary(ExprPtr& outExpression, std::string& outError) {
    const Token token = peek();

    if (match(TokenKind::Integer)) {
        auto expression = std::make_unique<Expr>();
        expression->location = token.location;
        expression->node = IntegerLiteralExpr{ std::stoi(token.lexeme) };
        outExpression = std::move(expression);
        return true;
    }

    if (match(TokenKind::String)) {
        auto expression = std::make_unique<Expr>();
        expression->location = token.location;
        expression->node = StringLiteralExpr{ token.lexeme };
        outExpression = std::move(expression);
        return true;
    }

    if (match(TokenKind::Identifier)) {
        auto expression = std::make_unique<Expr>();
        expression->location = token.location;

        if (token.lexeme == "true") {
            expression->node = BooleanLiteralExpr{ true };
        }
        else if (token.lexeme == "false") {
            expression->node = BooleanLiteralExpr{ false };
        }
        else {
            expression->node = IdentifierExpr{ token.lexeme };
        }

        outExpression = std::move(expression);
        return true;
    }

    if (match(TokenKind::LeftParen)) {
        ExprPtr expression;
        if (!parseExpression(expression, outError)) {
            return false;
        }

        if (!expect(TokenKind::RightParen, "Expected ')' after expression.", outError)) {
            return false;
        }

        outExpression = std::move(expression);
        return true;
    }

    outError = makeError(token.location, "Expected expression.");
    return false;
}
