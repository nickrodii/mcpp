#pragma once

#include "Ast.h"

#include <string>
#include <vector>

enum class TokenKind {
    EndOfFile,
    Newline,
    Slash,
    LeftBrace,
    RightBrace,
    LeftParen,
    RightParen,
    Colon,
    Comma,
    Plus,
    Minus,
    Star,
    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Identifier,
    Integer,
    String
};

struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    std::string lexeme;
    SourceLocation location;
};

class Lexer {
public:
    explicit Lexer(const std::string& sourceText);

    bool tokenize(std::vector<Token>& outTokens, std::string& outError) const;

private:
    const std::string& sourceText;

    bool lexString(
        size_t& index,
        size_t& line,
        size_t& column,
        std::vector<Token>& outTokens,
        std::string& outError
    ) const;
};
