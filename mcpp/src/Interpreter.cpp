#include "../include/Interpreter.h"

#include "../include/Lexer.h"
#include "../include/Parser.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
    using Value = Interpreter::Value;

    const IntegerLiteralExpr* asIntegerLiteral(const Expr& expression) {
        return std::get_if<IntegerLiteralExpr>(&expression.node);
    }

    const StringLiteralExpr* asStringLiteral(const Expr& expression) {
        return std::get_if<StringLiteralExpr>(&expression.node);
    }

    const BooleanLiteralExpr* asBooleanLiteral(const Expr& expression) {
        return std::get_if<BooleanLiteralExpr>(&expression.node);
    }

    const IdentifierExpr* asIdentifier(const Expr& expression) {
        return std::get_if<IdentifierExpr>(&expression.node);
    }

    const UnaryExpr* asUnary(const Expr& expression) {
        return std::get_if<UnaryExpr>(&expression.node);
    }

    const BinaryExpr* asBinary(const Expr& expression) {
        return std::get_if<BinaryExpr>(&expression.node);
    }

    const BlockStatement* asBlock(const Statement& statement) {
        return std::get_if<BlockStatement>(&statement.node);
    }

    const StopStatement* asStop(const Statement& statement) {
        return std::get_if<StopStatement>(&statement.node);
    }

    const SayStatement* asSay(const Statement& statement) {
        return std::get_if<SayStatement>(&statement.node);
    }

    const GiveStatement* asGive(const Statement& statement) {
        return std::get_if<GiveStatement>(&statement.node);
    }

    const StoreResultStatement* asStoreResult(const Statement& statement) {
        return std::get_if<StoreResultStatement>(&statement.node);
    }

    const ReturnStatement* asReturn(const Statement& statement) {
        return std::get_if<ReturnStatement>(&statement.node);
    }

    const FunctionCallStatement* asFunctionCall(const Statement& statement) {
        return std::get_if<FunctionCallStatement>(&statement.node);
    }

    const MsgStatement* asMsg(const Statement& statement) {
        return std::get_if<MsgStatement>(&statement.node);
    }
}

Interpreter::Value Interpreter::Value::makeNone() {
    return Value{};
}

Interpreter::Value Interpreter::Value::makeInt(int v) {
    Value value;
    value.type = Type::Int;
    value.intValue = v;
    return value;
}

Interpreter::Value Interpreter::Value::makeString(const std::string& v) {
    Value value;
    value.type = Type::String;
    value.stringValue = v;
    return value;
}

Interpreter::Value Interpreter::Value::makeBool(bool v) {
    Value value;
    value.type = Type::Bool;
    value.boolValue = v;
    return value;
}

std::string Interpreter::Value::toString() const {
    switch (type) {
    case Type::Int:
        return std::to_string(intValue);
    case Type::String:
        return stringValue;
    case Type::Bool:
        return boolValue ? "true" : "false";
    case Type::None:
    default:
        return "null";
    }
}

bool Interpreter::loadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << path << '\n';
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();

    sourceText = buffer.str();
    program = Program{};
    return true;
}

bool Interpreter::parse() {
    Program parsedProgram;
    if (!parseSource(sourceText, parsedProgram)) {
        return false;
    }

    program = std::move(parsedProgram);
    return true;
}

bool Interpreter::execute() {
    resetState();
    return executeProgram(program);
}

bool Interpreter::run() {
    if (!parse()) {
        return false;
    }

    return execute();
}

bool Interpreter::executeSource(const std::string& source) {
    Program parsedProgram;
    if (!parseSource(source, parsedProgram)) {
        return false;
    }

    return executeProgram(parsedProgram);
}

void Interpreter::resetState() {
    globals.clear();
    functionStorage.clear();
    functionTable.clear();
    stopRequestedFlag = false;
}

bool Interpreter::stopRequested() const {
    return stopRequestedFlag;
}

bool Interpreter::parseSource(const std::string& source, Program& outProgram) const {
    Lexer lexer(source);
    std::vector<Token> tokens;
    std::string error;

    if (!lexer.tokenize(tokens, error)) {
        std::cerr << error << '\n';
        return false;
    }

    Parser parser(tokens);
    if (!parser.parseProgram(outProgram, error)) {
        std::cerr << error << '\n';
        return false;
    }

    return true;
}

bool Interpreter::registerFunctions(Program& parsedProgram) {
    for (FunctionDefinition& function : parsedProgram.functions) {
        if (functionTable.contains(function.name)) {
            std::cerr << makeRuntimeError(
                function.location,
                "Duplicate function definition for '" + function.name + "'."
            ) << '\n';
            return false;
        }

        auto storedFunction = std::make_unique<FunctionDefinition>(std::move(function));
        const std::string functionName = storedFunction->name;
        const FunctionDefinition* functionPtr = storedFunction.get();

        functionStorage.push_back(std::move(storedFunction));
        functionTable[functionName] = functionPtr;
    }

    return true;
}

bool Interpreter::executeProgram(Program& parsedProgram) {
    if (!registerFunctions(parsedProgram)) {
        return false;
    }

    stopRequestedFlag = false;

    bool shouldStop = false;
    Value lastResult = Value::makeInt(0);
    if (!executeStatements(parsedProgram.topLevelStatements, globals, shouldStop, lastResult, false)) {
        return false;
    }

    stopRequestedFlag = shouldStop;
    return true;
}

bool Interpreter::tryGetInt(const Value& value, int& out) {
    if (value.type == Value::Type::Int) {
        out = value.intValue;
        return true;
    }

    if (value.type == Value::Type::Bool) {
        out = value.boolValue ? 1 : 0;
        return true;
    }

    if (value.type != Value::Type::String || value.stringValue.empty()) {
        return false;
    }

    size_t index = 0;
    if (value.stringValue[0] == '-') {
        if (value.stringValue.size() == 1) {
            return false;
        }

        index = 1;
    }

    for (; index < value.stringValue.size(); index++) {
        if (!std::isdigit(static_cast<unsigned char>(value.stringValue[index]))) {
            return false;
        }
    }

    out = std::stoi(value.stringValue);
    return true;
}

std::string Interpreter::makeRuntimeError(const SourceLocation& location, const std::string& message) {
    return "Runtime error at " + formatLocation(location) + ": " + message;
}

bool Interpreter::evaluateExpression(
    const Expr& expression,
    const std::unordered_map<std::string, Value>& scope,
    Value& outValue,
    std::string& outError
) const {
    if (const IntegerLiteralExpr* literal = asIntegerLiteral(expression)) {
        outValue = Value::makeInt(literal->value);
        return true;
    }

    if (const StringLiteralExpr* literal = asStringLiteral(expression)) {
        outValue = Value::makeString(literal->value);
        return true;
    }

    if (const BooleanLiteralExpr* literal = asBooleanLiteral(expression)) {
        outValue = Value::makeBool(literal->value);
        return true;
    }

    if (const IdentifierExpr* identifier = asIdentifier(expression)) {
        const auto it = scope.find(identifier->name);
        if (it == scope.end()) {
            outError = makeRuntimeError(expression.location, "Unknown identifier '" + identifier->name + "'.");
            return false;
        }

        outValue = it->second;
        return true;
    }

    if (const UnaryExpr* unary = asUnary(expression)) {
        Value operandValue;
        if (!evaluateExpression(*unary->operand, scope, operandValue, outError)) {
            return false;
        }

        int number = 0;
        if (!tryGetInt(operandValue, number)) {
            outError = makeRuntimeError(expression.location, "Unary '-' requires a numeric value.");
            return false;
        }

        outValue = Value::makeInt(-number);
        return true;
    }

    const BinaryExpr* binary = asBinary(expression);
    if (binary == nullptr) {
        outError = makeRuntimeError(expression.location, "Unsupported expression node.");
        return false;
    }

    Value leftValue;
    if (!evaluateExpression(*binary->left, scope, leftValue, outError)) {
        return false;
    }

    Value rightValue;
    if (!evaluateExpression(*binary->right, scope, rightValue, outError)) {
        return false;
    }

    switch (binary->op) {
    case BinaryOp::Add:
        if (leftValue.type == Value::Type::Int && rightValue.type == Value::Type::Int) {
            outValue = Value::makeInt(leftValue.intValue + rightValue.intValue);
        }
        else {
            outValue = Value::makeString(leftValue.toString() + rightValue.toString());
        }
        return true;

    case BinaryOp::Subtract:
    case BinaryOp::Multiply:
    case BinaryOp::Divide: {
        int leftNumber = 0;
        int rightNumber = 0;
        if (!tryGetInt(leftValue, leftNumber) || !tryGetInt(rightValue, rightNumber)) {
            const std::string opName =
                binary->op == BinaryOp::Subtract ? "Subtraction" :
                binary->op == BinaryOp::Multiply ? "Multiplication" :
                "Division";

            outError = makeRuntimeError(expression.location, opName + " requires numeric values.");
            return false;
        }

        if (binary->op == BinaryOp::Subtract) {
            outValue = Value::makeInt(leftNumber - rightNumber);
            return true;
        }

        if (binary->op == BinaryOp::Multiply) {
            outValue = Value::makeInt(leftNumber * rightNumber);
            return true;
        }

        if (rightNumber == 0) {
            outError = makeRuntimeError(expression.location, "Division by zero.");
            return false;
        }

        outValue = Value::makeInt(leftNumber / rightNumber);
        return true;
    }
    default:
        break;
    }

    outError = makeRuntimeError(expression.location, "Unsupported binary operation.");
    return false;
}

bool Interpreter::executeStatements(
    const std::vector<StatementPtr>& statements,
    std::unordered_map<std::string, Value>& scope,
    bool& shouldStop,
    Value& lastResult,
    bool isFunctionContext
) const {
    for (const StatementPtr& statement : statements) {
        if (statement == nullptr) {
            continue;
        }

        if (!executeStatement(*statement, scope, shouldStop, lastResult, isFunctionContext)) {
            return false;
        }

        if (shouldStop) {
            return true;
        }
    }

    return true;
}

bool Interpreter::executeStatement(
    const Statement& statement,
    std::unordered_map<std::string, Value>& scope,
    bool& shouldStop,
    Value& lastResult,
    bool isFunctionContext
) const {
    if (asBlock(statement) != nullptr) {
        return true;
    }

    if (asStop(statement) != nullptr) {
        shouldStop = true;
        lastResult = Value::makeInt(0);
        return true;
    }

    if (const SayStatement* say = asSay(statement)) {
        Value value;
        std::string error;
        if (!evaluateExpression(*say->expression, scope, value, error)) {
            std::cerr << error << '\n';
            return false;
        }

        std::cout << value.toString() << '\n';
        lastResult = value;
        return true;
    }

    if (const GiveStatement* give = asGive(statement)) {
        Value value;
        std::string error;
        if (!evaluateExpression(*give->expression, scope, value, error)) {
            std::cerr << error << '\n';
            return false;
        }

        scope[give->variableName] = value;
        lastResult = value;
        return true;
    }

    if (const StoreResultStatement* store = asStoreResult(statement)) {
        Value nestedResult = Value::makeInt(0);
        if (!executeStatement(*store->command, scope, shouldStop, nestedResult, isFunctionContext)) {
            return false;
        }

        scope[store->variableName] = nestedResult;
        lastResult = nestedResult;
        return true;
    }

    if (const ReturnStatement* returnStatement = asReturn(statement)) {
        if (!isFunctionContext) {
            std::cerr << makeRuntimeError(
                statement.location,
                "/execute store result <expr> is only valid inside a function."
            ) << '\n';
            return false;
        }

        Value value;
        std::string error;
        if (!evaluateExpression(*returnStatement->expression, scope, value, error)) {
            std::cerr << error << '\n';
            return false;
        }

        lastResult = value;
        return true;
    }

    if (const FunctionCallStatement* call = asFunctionCall(statement)) {
        std::unordered_map<std::string, Value> namedArgs;

        for (const NamedArgument& argument : call->arguments) {
            Value value;
            std::string error;
            if (!evaluateExpression(*argument.expression, scope, value, error)) {
                std::cerr << error << '\n';
                return false;
            }

            namedArgs[argument.name] = value;
        }

        Value result = Value::makeInt(0);
        if (!callFunction(statement.location, call->name, namedArgs, result, shouldStop)) {
            return false;
        }

        lastResult = result;
        return true;
    }

    if (const MsgStatement* msg = asMsg(statement)) {
        std::string input;
        std::getline(std::cin, input);
        scope[msg->variableName] = Value::makeString(input);
        lastResult = scope[msg->variableName];
        return true;
    }

    std::cerr << makeRuntimeError(statement.location, "Unsupported statement node.") << '\n';
    return false;
}

bool Interpreter::callFunction(
    const SourceLocation& callLocation,
    const std::string& name,
    const std::unordered_map<std::string, Value>& namedArgs,
    Value& outResult,
    bool& shouldStop
) const {
    const auto it = functionTable.find(name);
    if (it == functionTable.end()) {
        std::cerr << makeRuntimeError(callLocation, "Function '" + name + "' was not found.") << '\n';
        return false;
    }

    const FunctionDefinition& function = *it->second;

    for (const auto& pair : namedArgs) {
        bool found = false;
        for (const std::string& parameter : function.parameters) {
            if (parameter == pair.first) {
                found = true;
                break;
            }
        }

        if (!found) {
            std::cerr << makeRuntimeError(
                callLocation,
                "Function '" + name + "' does not have a parameter named '" + pair.first + "'."
            ) << '\n';
            return false;
        }
    }

    std::unordered_map<std::string, Value> localScope;
    for (const std::string& parameter : function.parameters) {
        const auto argIt = namedArgs.find(parameter);
        if (argIt != namedArgs.end()) {
            localScope[parameter] = argIt->second;
        }
        else {
            localScope[parameter] = Value::makeNone();
        }
    }

    outResult = Value::makeInt(0);
    return executeStatements(function.body, localScope, shouldStop, outResult, true);
}
