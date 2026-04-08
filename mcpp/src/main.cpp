#include "../include/Interpreter.h"

#include <cctype>
#include <iostream>
#include <string>

namespace {
    constexpr const char* kVersion = "0.1.0";

    void printHelp() {
        std::cout
            << "mcpp " << kVersion << '\n'
            << "Built by Nick Rodi\n\n"
            << "Usage:\n"
            << "  mcpp\n"
            << "  mcpp <file.mcpp>\n"
            << "  mcpp --help\n"
            << "  mcpp --version\n"
            << '\n'
            << "Interactive REPL can be started by calling mcpp with no arguments.\n";
    }

    std::string trim(const std::string& text) {
        size_t start = 0;
        while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
            start++;
        }

        size_t end = text.size();
        while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
            end--;
        }

        return text.substr(start, end - start);
    }

    int computeBraceBalance(const std::string& text) {
        int balance = 0;
        bool inString = false;

        for (size_t index = 0; index < text.size(); index++) {
            const char ch = text[index];

            if (ch == '"' && (index == 0 || text[index - 1] != '\\')) {
                inString = !inString;
                continue;
            }

            if (inString) {
                continue;
            }

            if (ch == '{') {
                balance++;
            }
            else if (ch == '}') {
                balance--;
            }
        }

        return balance;
    }

    void printReplHelp() {
        std::cout
            << "REPL commands:\n"
            << "  :help  show this message\n"
            << "  :quit  exit the REPL\n"
            << "  :exit  exit the REPL\n";
    }

    int runRepl() {
        Interpreter interpreter;
        std::string pendingInput;

        std::cout
            << "mcpp " << kVersion << " REPL\n"
            << "Type :help for REPL commands. Type :quit to exit.\n";

        while (true) {
            const bool needsMoreInput = computeBraceBalance(pendingInput) > 0;

            std::cout << (needsMoreInput ? "... " : ">>> ");
            std::cout.flush();

            std::string line;
            if (!std::getline(std::cin, line)) {
                if (!pendingInput.empty()) {
                    std::cerr << "\nDiscarding incomplete input.\n";
                }

                std::cout << '\n';
                return 0;
            }

            const std::string trimmedLine = trim(line);
            if (pendingInput.empty()) {
                if (trimmedLine.empty()) {
                    continue;
                }

                if (trimmedLine == ":help") {
                    printReplHelp();
                    continue;
                }

                if (trimmedLine == ":quit" || trimmedLine == ":exit") {
                    return 0;
                }
            }

            pendingInput += line;
            pendingInput += '\n';

            if (computeBraceBalance(pendingInput) > 0) {
                continue;
            }

            if (!interpreter.executeSource(pendingInput)) {
                pendingInput.clear();
                continue;
            }

            pendingInput.clear();

            if (interpreter.stopRequested()) {
                return 0;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        return runRepl();
    }

    const std::string firstArg = argv[1];

    if (firstArg == "--help" || firstArg == "-h") {
        printHelp();
        return 0;
    }

    if (firstArg == "--version" || firstArg == "-v") {
        std::cout << "mcpp " << kVersion << '\n';
        return 0;
    }

    if (argc > 2) {
        std::cerr << "Error: expected exactly one file path.\n\n";
        printHelp();
        return 1;
    }

    Interpreter interpreter;
    const std::string path = firstArg;

    if (!interpreter.loadFile(path)) {
        return 1;
    }

    if (!interpreter.run()) {
        return 1;
    }

    return 0;
}
