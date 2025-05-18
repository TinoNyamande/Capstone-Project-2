#ifndef LEXER_H
#define LEXER_H
#include "../../include/KaleidoscopeJIT.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <fstream> 
#include <filesystem>

using namespace llvm;
using namespace llvm::orc;

extern std::ifstream InputFile;
extern std::string FileBuffer;
extern const char *FileBufferPtr;
extern int CurrentLine;


enum Token
{
  tok_eof = -1,
  tok_def = -2,
  tok_extern = -3,
  tok_identifier = -4,
  tok_number = -5,
  tok_if = -6,
  tok_then = -7,
  tok_else = -8,
  tok_for = -9,
  tok_in = -10,
  tok_binary = -11,
  tok_unary = -12,
  tok_var = -13,
  tok_return = -14,
  tok_string = -15,
  tok_while = -16,
  tok_do = -17,
  tok_class = -18,
  tok_new = -19,
  tok_this = -20,
  tok_extends = -21,
  tok_public = -21,
  tok_private = -22,
  tok_dot = -23,  
  tok_arrow = -24, 
  tok_semicolon = -25,
  tok_globalvar = -26,
};

extern std::ifstream InputFile;
extern std::string IdentifierStr; 
extern double NumVal;   

int gettok();

#endif