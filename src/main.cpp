#include "lexer/lexer.h"
#include "../ast/ast.h"
#include "../parser/parser.h"
#include "../codegen/codegen.h"


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
  std::ofstream file(filePath, std::ios::out | std::ios::app); // Example for append mode
  if (!file.is_open())
  {
    fprintf(stderr, "Error opening file\n");
  }
}
extern "C" DLLEXPORT void testfile(const char *filePath, const char *mode)
{
  std::ofstream file(filePath, std::ios::out | std::ios::app); // Example for append mode
  if (!file.is_open())
  {
    fprintf(stderr, "Error opening file\n");
  }
}
extern "C" DLLEXPORT const char *readFile(const char *filePath) {
    static std::string content;  // Static to persist after function returns
    content.clear();

    fprintf(stderr, "Attempting to read file: %s\n", filePath);

    std::ifstream file(filePath);
    if (!file.is_open()) {
        fprintf(stderr, "Error: Could not open file: %s\n", filePath);
        content = "";  // Set to an empty string if the file cannot be opened
        return content.c_str();
    }

    fprintf(stderr, "File opened successfully: %s\n", filePath);

    // Read the file content
    content.assign((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
    file.close();

    fprintf(stderr, "File read successfully. Content:\n%s\n", content.c_str());

    // Automatically print content to the console
    fprintf(stdout, content.c_str());

    return content.c_str();
}


extern "C" DLLEXPORT void writeFile(const char *filePath, const char *content)
{
  std::ofstream file(filePath, std::ios::trunc);
  if (!file.is_open())
  {
    fprintf(stderr, "Error writing to file\n");
  }
  else
  {
    file << content;
  }
}

extern "C" DLLEXPORT void deleteFile(const char *filePath)
{
  if (!std::filesystem::remove(filePath))
  {
    fprintf(stderr, "Error deleting file\n");
  }
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" DLLEXPORT void nyora(const char *value)
{
  if (value)
  {
    fprintf(stderr, "%s\n", value);
  }
  else
  {
    fprintf(stderr, "Error: null value passed to nyora\n");
  }
}
extern "C" DLLEXPORT void test(const char *value)
{
  if (value)
  {
    fprintf(stderr, "%s\n", value);
  }
  else
  {
    fprintf(stderr, "Error: null value passed to nyora\n");
  }
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
    return 1;
  }

  // Open the input file using the global InputFile
  InputFile.open(argv[1]);
  if (!InputFile.is_open())
  {
    std::cerr << "Error: Could not open file " << argv[1] << std::endl;
    return 1;
  }

  // Initialize LLVM components
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // Install standard binary operators
  BinopPrecedence['<'] = 10;
  BinopPrecedence['>'] = 10; // Set precedence for '>'
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest precedence

  // Add built-in functions to the global prototype map
  auto AddBuiltinFunctions = []()
  {
    FunctionProtos["putchard"] = std::make_unique<PrototypeAST>("putchard", std::vector<std::string>{"x"});
    FunctionProtos["nyora"] = std::make_unique<PrototypeAST>("nyora", std::vector<std::string>{"x"});
    FunctionProtos["test"] = std::make_unique<PrototypeAST>("test", std::vector<std::string>{"x"});
    FunctionProtos["testfile"] = std::make_unique<PrototypeAST>("testfile", std::vector<std::string>{"filePath", "mode"});
    FunctionProtos["openFile"] = std::make_unique<PrototypeAST>("openFile", std::vector<std::string>{"filePath", "mode"});
    FunctionProtos["readFile"] = std::make_unique<PrototypeAST>("readFile", std::vector<std::string>{"filePath"});
    FunctionProtos["writeFile"] = std::make_unique<PrototypeAST>("writeFile", std::vector<std::string>{"fileHandle", "content"});
    FunctionProtos["deleteFile"] = std::make_unique<PrototypeAST>("deleteFile", std::vector<std::string>{"filePath"});
  };

  AddBuiltinFunctions();

  getNextToken();
  TheJIT = ExitOnErr(KaleidoscopeJIT::Create());

  // Initialize the module and managers
  InitializeModuleAndManagers();

  // Run the main interpreter loop
  MainLoop();

  // Close the input file
  InputFile.close();

  return 0;
}