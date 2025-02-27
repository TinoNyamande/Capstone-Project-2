// Codegen.h
#ifndef CODEGEN_H
#define CODEGEN_H

#include "Codegen.h"
#include "../lexer/lexer.h"
#include "../ast/ast.h"
#include "../parser/parser.h"

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::map<std::string, AllocaInst *> NamedValues;
extern std::unique_ptr<KaleidoscopeJIT> TheJIT;
extern std::unique_ptr<FunctionPassManager> TheFPM;
extern std::unique_ptr<LoopAnalysisManager> TheLAM;
extern std::unique_ptr<FunctionAnalysisManager> TheFAM;
extern std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
extern std::unique_ptr<ModuleAnalysisManager> TheMAM;
extern std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
extern std::unique_ptr<StandardInstrumentations> TheSI;
extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
extern ExitOnError ExitOnErr;

// Initializes the LLVM module and global states.
void MainLoop();
void HandleTopLevelExpression();
void HandleExtern();
void HandleDefinition();
void InitializeModuleAndManagers();

#endif // CODEGEN_H
