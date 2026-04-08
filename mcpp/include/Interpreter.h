#pragma once

#include "Ast.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Interpreter {
public:
    struct Value {
        enum class Type {
            None,
            Int,
            String,
            Bool
        };

        Type type = Type::None;
        int intValue = 0;
        std::string stringValue;
        bool boolValue = false;

        static Value makeNone();
        static Value makeInt(int v);
        static Value makeString(const std::string& v);
        static Value makeBool(bool v);

        std::string toString() const;
    };

    bool loadFile(const std::string& path);
    bool parse();
    bool execute();
    bool run();
    bool executeSource(const std::string& source);
    void resetState();
    bool stopRequested() const;

private:
    std::string sourceText;
    Program program;
    std::unordered_map<std::string, Value> globals;
    std::vector<std::unique_ptr<FunctionDefinition>> functionStorage;
    std::unordered_map<std::string, const FunctionDefinition*> functionTable;
    bool stopRequestedFlag = false;

    static bool tryGetInt(const Value& value, int& out);
    static std::string makeRuntimeError(const SourceLocation& location, const std::string& message);

    bool parseSource(const std::string& source, Program& outProgram) const;
    bool registerFunctions(Program& parsedProgram);
    bool executeProgram(Program& parsedProgram);

    bool evaluateExpression(
        const Expr& expression,
        const std::unordered_map<std::string, Value>& scope,
        Value& outValue,
        std::string& outError
    ) const;

    bool executeStatements(
        const std::vector<StatementPtr>& statements,
        std::unordered_map<std::string, Value>& scope,
        bool& shouldStop,
        Value& lastResult,
        bool isFunctionContext
    ) const;

    bool executeStatement(
        const Statement& statement,
        std::unordered_map<std::string, Value>& scope,
        bool& shouldStop,
        Value& lastResult,
        bool isFunctionContext
    ) const;

    bool callFunction(
        const SourceLocation& callLocation,
        const std::string& name,
        const std::unordered_map<std::string, Value>& namedArgs,
        Value& outResult,
        bool& shouldStop
    ) const;
};
