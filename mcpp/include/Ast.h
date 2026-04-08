#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct SourceLocation {
    size_t line = 1;
    size_t column = 1;
};

inline std::string formatLocation(const SourceLocation& location) {
    return "line " + std::to_string(location.line) + ", column " + std::to_string(location.column);
}

struct Expr;
struct Statement;

using ExprPtr = std::unique_ptr<Expr>;
using StatementPtr = std::unique_ptr<Statement>;

struct IntegerLiteralExpr {
    int value = 0;
};

struct StringLiteralExpr {
    std::string value;
};

struct BooleanLiteralExpr {
    bool value = false;
};

struct IdentifierExpr {
    std::string name;
};

enum class UnaryOp {
    Negate
};

struct UnaryExpr {
    UnaryOp op = UnaryOp::Negate;
    ExprPtr operand;
};

enum class BinaryOp {
    Add,
    Subtract,
    Multiply,
    Divide,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual
};

struct BinaryExpr {
    BinaryOp op = BinaryOp::Add;
    ExprPtr left;
    ExprPtr right;
};

using ExprNode = std::variant<
    IntegerLiteralExpr,
    StringLiteralExpr,
    BooleanLiteralExpr,
    IdentifierExpr,
    UnaryExpr,
    BinaryExpr
>;

struct Expr {
    SourceLocation location;
    ExprNode node;
};

struct BlockStatement {
    std::vector<StatementPtr> statements;
};

struct StopStatement {
};

struct SayStatement {
    ExprPtr expression;
};

struct GiveStatement {
    std::string variableName;
    ExprPtr expression;
};

struct StoreResultStatement {
    std::string variableName;
    StatementPtr command;
};

enum class ExecuteConditionMode {
    If,
    Unless,
    Until
};

struct ExecuteConditionStatement {
    ExecuteConditionMode mode = ExecuteConditionMode::If;
    ExprPtr condition;
    StatementPtr command;
};

struct ReturnStatement {
    ExprPtr expression;
};

struct NamedArgument {
    std::string name;
    ExprPtr expression;
};

struct FunctionCallStatement {
    std::string name;
    std::vector<NamedArgument> arguments;
};

struct MsgStatement {
    std::string variableName;
};

struct GameruleStatement {
    std::string name;
    bool value = false;
};

using StatementNode = std::variant<
    BlockStatement,
    StopStatement,
    SayStatement,
    GiveStatement,
    StoreResultStatement,
    ExecuteConditionStatement,
    ReturnStatement,
    FunctionCallStatement,
    MsgStatement,
    GameruleStatement
>;

struct Statement {
    SourceLocation location;
    StatementNode node;
};

struct FunctionDefinition {
    SourceLocation location;
    std::string name;
    std::vector<std::string> parameters;
    std::vector<StatementPtr> body;
};

struct Program {
    std::vector<FunctionDefinition> functions;
    std::vector<StatementPtr> topLevelStatements;
};
