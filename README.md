# MC++
MC++ is an esoteric, command-oriented programming language built using C++ designed to mirror Minecraft server commands as closely as possible.

It supports script execution, an interactive REPL, scoped command blocks, named-argument functions, dynamic / loose typing, and command-based control flow.

## Running MC++

- Download the latest build from https://github.com/nickrodii/mcpp/releases
- To start the REPL, simply run the program or call it with `mcpp`.
- To run a mcpp file, use `mcpp <file.mcpp>`
- To show help, run `mcpp --help` or `mcpp -h`
- To show version, run `mcpp --version` or `mcpp -v`

## Building MC++ from source code

- Building from source code requires Windows, Visual Studio: Desktop Development with C++, and MSVC / MSBuild
- *To build in Visual Studio*: Open `mcpp.slnx`, then Build Solution
- *To build from terminal*: Open Developer Command Prompt for Visual Studio and run: `MSBuild.exe mcpp.vcxproj /p:Configuration=Release /p:Platform=x64`
## REPL

The REPL is an interactive session for performing live commands and seeing output in realtime. The REPL supports multi-line input and persistent session state.

- `>>>` means fresh input
- `...` means current block is still open
- `/stop` exits REPL session
- `:help` shows REPL help
- `:quit` exits the REPL
- `:exit` exits the REPL

## Commands

- `/say <expr>` evaluates an expression and prints the result
- `/give <name> <expr>` assigns a value to a variable
- `/msg <name>` reads one line of input and stores it as string `<name>`
- `/stop` halts program
- `/function <name> {param1, param2, ...}` starts a function, uses `/execute store result` to return within function scope, and must be within a command block to be multiple lines long. Command block `}` ends the function definition.
- `/function <name> {a: expr, b: expr, ...}` calls a function with named arguments
- `/execute store result <var> run <command>` runs a command and stores that command's result in `<var>`
- `/execute store result <expr>` returns a value from inside a function
- `/execute if <condition> run <command>` runs the command only if the condition is true
- `/execute unless <condition> run <command>` runs the command only if the condition is false
- `/execute until <condition> run <command>` keeps running the command until the condition becomes true
- `/summon command_block { ... }` creates a block, usually used with function definitions
- `/gamerule keep_inventory true` makes block or function variable changes persist outside that scope
- `/gamerule keep_inventory false` keeps block or function variable changes local to that scope

## Expressions

MC++ supports the following:
- Integers: `67`
- Strings: `"example"`
- Booleans: `true` or `false`
- Variable references: `x`
- Parantheses: `(x + 2) * 3`
- Unary minus: `-x`
- Arithmetic: `+`, `-`, `*`, `/`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`

## Runtime Behavior

- `+` performs integer addition when both operands are integers, and otherwise performs string concatenation
- `-`, `*`, `/`, `<`, `<=`, `>`, `>=` all require numeric values
- Numeric strings can be coerced into numbers
- Division is integer division (no decimal)
- Conditions can use booleans, integers, or strings. Empty string is false, non-empty string is true, but `"false"` and numeric `0` are treated as false.

## Scope Rules
- Top level: variables persist for current script or REPL session
- Command blocks: creates child scope
- Functions: creates child scope
- With `keep_inventory` active, block/function writes merge back into the parent scope, otherwise local within block/function.
