#pragma once

#include "Ast.h"
#include "Lexer.h"

#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    bool parseProgram(Program& outProgram, std::string& outError);

private:
    const std::vector<Token>& tokens;
    size_t position = 0;

    const Token& peek(size_t offset = 0) const;
    bool isAtEnd() const;
    bool check(TokenKind kind, size_t offset = 0) const;
    bool checkIdentifier(const std::string& text, size_t offset = 0) const;
    bool match(TokenKind kind);
    bool matchIdentifier(const std::string& text);
    void skipNewlines();
    void skipLine();

    std::string makeError(const SourceLocation& location, const std::string& message) const;
    bool expect(TokenKind kind, const std::string& message, std::string& outError);
    bool expectIdentifier(const std::string& text, std::string& outError);
    bool expectName(std::string& outName, const std::string& message, std::string& outError);
    bool consumeLineEnd(std::string& outError);

    bool isCommandBlockStart(size_t offset = 0) const;
    bool looksLikeFunctionHeader() const;
    bool consumeCommandBlockStart(std::string& outError);
    bool parseBlockContents(
        std::vector<StatementPtr>& outStatements,
        bool isFunctionContext,
        std::string& outError
    );

    bool parseTopLevelCommandBlock(Program& outProgram, std::string& outError);
    bool parseFunctionDefinition(FunctionDefinition& outFunction, std::string& outError);
    bool parseFunctionHeaderLine(
        std::string& outName,
        std::vector<std::string>& outParameters,
        std::string& outError
    );

    bool parseStatement(StatementPtr& outStatement, bool isFunctionContext, std::string& outError);
    bool parseGenericCommandBlock(StatementPtr& outStatement, std::string& outError);
    bool parseCommandStatement(StatementPtr& outStatement, bool isFunctionContext, std::string& outError);
    bool parseCommandAfterSlash(
        const SourceLocation& slashLocation,
        StatementPtr& outStatement,
        bool isFunctionContext,
        std::string& outError
    );
    bool parseExecuteStoreResult(
        const SourceLocation& slashLocation,
        StatementPtr& outStatement,
        bool isFunctionContext,
        std::string& outError
    );
    bool parseFunctionCallStatement(
        const SourceLocation& slashLocation,
        StatementPtr& outStatement,
        std::string& outError
    );

    bool parseExpression(ExprPtr& outExpression, std::string& outError);
    bool parseAddSub(ExprPtr& outExpression, std::string& outError);
    bool parseMulDiv(ExprPtr& outExpression, std::string& outError);
    bool parseUnary(ExprPtr& outExpression, std::string& outError);
    bool parsePrimary(ExprPtr& outExpression, std::string& outError);
};
