#include "../include/Lexer.h"

#include <cctype>

namespace {
    std::string makeLexerError(const SourceLocation& location, const std::string& message) {
        return "Lex error at " + formatLocation(location) + ": " + message;
    }
}

Lexer::Lexer(const std::string& sourceText)
    : sourceText(sourceText) {
}

bool Lexer::tokenize(std::vector<Token>& outTokens, std::string& outError) const {
    outTokens.clear();

    size_t index = 0;
    size_t line = 1;
    size_t column = 1;

    while (index < sourceText.size()) {
        const char ch = sourceText[index];

        if (ch == ' ' || ch == '\t') {
            index++;
            column++;
            continue;
        }

        if (ch == '\r') {
            index++;
            continue;
        }

        if (ch == '\n') {
            outTokens.push_back(Token{ TokenKind::Newline, "\n", SourceLocation{ line, column } });
            index++;
            line++;
            column = 1;
            continue;
        }

        if (ch == '"') {
            if (!lexString(index, line, column, outTokens, outError)) {
                return false;
            }
            continue;
        }

        const SourceLocation location{ line, column };

        switch (ch) {
        case '/':
            outTokens.push_back(Token{ TokenKind::Slash, "/", location });
            index++;
            column++;
            continue;
        case '{':
            outTokens.push_back(Token{ TokenKind::LeftBrace, "{", location });
            index++;
            column++;
            continue;
        case '}':
            outTokens.push_back(Token{ TokenKind::RightBrace, "}", location });
            index++;
            column++;
            continue;
        case '(':
            outTokens.push_back(Token{ TokenKind::LeftParen, "(", location });
            index++;
            column++;
            continue;
        case ')':
            outTokens.push_back(Token{ TokenKind::RightParen, ")", location });
            index++;
            column++;
            continue;
        case ':':
            outTokens.push_back(Token{ TokenKind::Colon, ":", location });
            index++;
            column++;
            continue;
        case ',':
            outTokens.push_back(Token{ TokenKind::Comma, ",", location });
            index++;
            column++;
            continue;
        case '+':
            outTokens.push_back(Token{ TokenKind::Plus, "+", location });
            index++;
            column++;
            continue;
        case '-':
            outTokens.push_back(Token{ TokenKind::Minus, "-", location });
            index++;
            column++;
            continue;
        case '*':
            outTokens.push_back(Token{ TokenKind::Star, "*", location });
            index++;
            column++;
            continue;
        default:
            break;
        }

        if (std::isdigit(static_cast<unsigned char>(ch))) {
            std::string lexeme;
            while (index < sourceText.size() &&
                std::isdigit(static_cast<unsigned char>(sourceText[index]))) {
                lexeme += sourceText[index];
                index++;
                column++;
            }

            outTokens.push_back(Token{ TokenKind::Integer, lexeme, location });
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            std::string lexeme;
            while (index < sourceText.size()) {
                const char current = sourceText[index];
                if (std::isalnum(static_cast<unsigned char>(current)) || current == '_') {
                    lexeme += current;
                    index++;
                    column++;
                }
                else {
                    break;
                }
            }

            outTokens.push_back(Token{ TokenKind::Identifier, lexeme, location });
            continue;
        }

        outError = makeLexerError(location, std::string("Unexpected character '") + ch + "'.");
        return false;
    }

    outTokens.push_back(Token{ TokenKind::EndOfFile, "", SourceLocation{ line, column } });
    return true;
}

bool Lexer::lexString(
    size_t& index,
    size_t& line,
    size_t& column,
    std::vector<Token>& outTokens,
    std::string& outError
) const {
    const SourceLocation location{ line, column };

    index++;
    column++;

    std::string value;

    while (index < sourceText.size()) {
        const char ch = sourceText[index];

        if (ch == '\n' || ch == '\r') {
            outError = makeLexerError(location, "Unterminated string literal.");
            return false;
        }

        if (ch == '\\') {
            if (index + 1 >= sourceText.size()) {
                outError = makeLexerError(location, "Unterminated escape sequence in string literal.");
                return false;
            }

            const char escaped = sourceText[index + 1];
            switch (escaped) {
            case 'n':
                value += '\n';
                break;
            case 'r':
                value += '\r';
                break;
            case 't':
                value += '\t';
                break;
            case '"':
                value += '"';
                break;
            case '\\':
                value += '\\';
                break;
            default:
                value += escaped;
                break;
            }

            index += 2;
            column += 2;
            continue;
        }

        if (ch == '"') {
            index++;
            column++;
            outTokens.push_back(Token{ TokenKind::String, value, location });
            return true;
        }

        value += ch;
        index++;
        column++;
    }

    outError = makeLexerError(location, "Unterminated string literal.");
    return false;
}
