#include "lexer/lexer.h"
#include "../ast/ast.h"
#include "../parser/parser.h"
#include "../codegen/codegen.h"

#include <iostream>
#include <string>
#include <fstream>

#define SHONALANG_VERSION "1.0.0"

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// putchard - putchar that takes a double and returns 0.
extern "C" DLLEXPORT double putchard(double X)
{
  fputc((char)X, stderr);
  return 0;
}

extern "C" DLLEXPORT void openFile(const char *filePath, const char *mode)
{
  std::ofstream file(filePath, std::ios::out | std::ios::app);
  if (!file.is_open())
  {
    fprintf(stderr, "Error opening file\n");
  }
}

extern "C" DLLEXPORT const char *readFile(const char *filePath) {
    static std::string content;
    content.clear();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        fprintf(stderr, "Error: Could not open file: %s\n", filePath);
        content = "";
        return content.c_str();
    }

    content.assign((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
    file.close();

    return content.c_str();
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--version") {
            std::cout << "ShonaLang Version: " << SHONALANG_VERSION << std::endl;
            return 0;
        }
    }

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    std::ifstream InputFile(argv[1]);
    if (!InputFile.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    BinopPrecedence['<'] = 10;
    BinopPrecedence['>'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;

    auto AddBuiltinFunctions = []()
    {
        FunctionProtos["putchard"] = std::make_unique<PrototypeAST>("putchard", std::vector<std::string>{"x"});
        FunctionProtos["nyora"] = std::make_unique<PrototypeAST>("nyora", std::vector<std::string>{"x"});
    };

    AddBuiltinFunctions();

    getNextToken();
    TheJIT = ExitOnErr(KaleidoscopeJIT::Create());
    InitializeModuleAndManagers();
    MainLoop();

    InputFile.close();
    return 0;
}