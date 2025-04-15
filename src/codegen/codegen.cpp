#include "codegen.h"

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<Module> TheModule;
std::unique_ptr<IRBuilder<>> Builder;
std::map<std::string, AllocaInst *> NamedValues;
std::unique_ptr<KaleidoscopeJIT> TheJIT;
std::unique_ptr<FunctionPassManager> TheFPM;
std::unique_ptr<LoopAnalysisManager> TheLAM;
std::unique_ptr<FunctionAnalysisManager> TheFAM;
std::unique_ptr<CGSCCAnalysisManager> TheCGAM;
std::unique_ptr<ModuleAnalysisManager> TheMAM;
std::unique_ptr<PassInstrumentationCallbacks> ThePIC;
std::unique_ptr<StandardInstrumentations> TheSI;
std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
ExitOnError ExitOnErr;
static std::map<std::string, GlobalVariable*> GlobalNamedValues;


Value *LogErrorV(const char *Str)
{

  LogError(Str);
  return nullptr;
}

extern "C" const char *to_string(double val)
{
  static char buffer[64];
  snprintf(buffer, sizeof(buffer), "%f", val);
  return buffer;
}

Function *getFunction(std::string Name)
{
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();

  // If no existing prototype exists, return null.
  return nullptr;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          StringRef VarName)
{
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                   TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getDoubleTy(*TheContext), nullptr, VarName);
}

Value *NumberExprAST::codegen()
{
  // Emit a numeric constant (double) instead of a string representation.
  return ConstantFP::get(*TheContext, APFloat(Val));
}

Value *StringExprAST::codegen()
{
  return Builder->CreateGlobalStringPtr(Val, "str");
}

Value *ReturnExprAST::codegen()
{
  Value *RetValV = RetVal->codegen();
  if (!RetValV)
    return nullptr;

  // Create a return instruction
  return Builder->CreateRet(RetValV);
}
Value *UnaryExprAST::codegen()
{
  Value *OperandV = Operand->codegen();
  if (!OperandV)
    return nullptr;

  Function *F = getFunction(std::string("unary") + Opcode);
  if (!F)
    return LogErrorV("Unknown unary operator");

  return Builder->CreateCall(F, OperandV, "unop");
}

Value *BinaryExprAST::codegen()
{
  if (Op == '=') {
    // Handle assignment
    VariableExprAST *LHSE = static_cast<VariableExprAST*>(LHS.get());
    if (!LHSE)
      return LogErrorV("destination of '=' must be a variable");
      
    Value *Val = RHS->codegen();
    if (!Val)
      return nullptr;

    // Check both local and global variables
    Value *Variable = nullptr;
    
    // 1. Check local variables
    if (NamedValues.count(LHSE->getName())) {
      Variable = NamedValues[LHSE->getName()];
    }
    // 2. Check global variables
    else if (GlobalNamedValues.count(LHSE->getName())) {
      Variable = GlobalNamedValues[LHSE->getName()];
    }
    
    if (!Variable)
      return LogErrorV(("Unknown variable name: " + LHSE->getName()).c_str());

    Builder->CreateStore(Val, Variable);
    return Val;
  }
  
  // Rest of the binary operation handling remains the same...
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op)
  {
  case '+':
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '<':
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  case '>':
    L = Builder->CreateFCmpUGT(L, R, "cmptmp"); // Use unsigned greater than
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  default:
    break;
  }

  // If it wasn't a builtin binary operator, it must be a user defined one. Emit
  // a call to it.
  Function *F = getFunction(std::string("binary") + Op);
  assert(F && "binary operator not found!");

  Value *Ops[] = {L, R};
  return Builder->CreateCall(F, Ops, "binop");
}
Value *WhileExprAST::codegen()
{
  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  BasicBlock *CondBB = BasicBlock::Create(*TheContext, "whilecond", TheFunction);
  BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "whileloop", TheFunction);
  BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "afterwhile", TheFunction);

  Builder->CreateBr(CondBB);
  Builder->SetInsertPoint(CondBB);

  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  CondV = Builder->CreateFCmpONE(CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "whilecond");
  Builder->CreateCondBr(CondV, LoopBB, AfterBB);

  Builder->SetInsertPoint(LoopBB);

  for (auto &Stmt : Body)
  { // ✅ Iterate over multiple expressions
    if (!Stmt->codegen())
      return nullptr;
  }

  Builder->CreateBr(CondBB);

  Builder->SetInsertPoint(AfterBB);
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Function *getPrintfFunction(Module *TheModule, LLVMContext &TheContext)
{
  // Check if printf is already declared
  if (Function *F = TheModule->getFunction("printf"))
    return F;

  // Create printf function type: int printf(char*, ...)
  std::vector<Type *> printfArgs;
  llvm::PointerType::get(llvm::Type::getInt8Ty(TheContext), 0);
  // Corrected usage

  FunctionType *printfType =
      FunctionType::get(Type::getInt32Ty(TheContext), printfArgs, true);

  Function *printfFunc =
      Function::Create(printfType, Function::ExternalLinkage, "printf", TheModule);

  return printfFunc;
}

Value *CallExprAST::codegen()
{

  // Look up the function in the module.
  Function *CalleeF = getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("'basa' iri harina kuwanikwa");

  if (Callee == "nyora")
  {

    // Ensure `nyora` gets exactly one argument.
    if (Args.size() != 1)
      return LogErrorV("nyora expects exactly one argument");

    // Generate code for the argument.
    Value *Arg = Args[0]->codegen();
    if (!Arg)
      return nullptr;

    // Retrieve printf function
    Function *printfFunc = getPrintfFunction(TheModule.get(), *TheContext);
    if (!printfFunc)
      return LogErrorV("Failed to declare printf function");

    // Check if the argument is a double or integer
    if (Arg->getType()->isDoubleTy())
    {

      Value *formatStr = Builder->CreateGlobalStringPtr("%.5f\n", "fmt"); // 5 decimal places for floats
      return Builder->CreateCall(printfFunc, {formatStr, Arg}, "printfcall");
    }
    else if (Arg->getType()->isIntegerTy())
    {

      Value *formatStr = Builder->CreateGlobalStringPtr("%d\n", "fmt"); // Format for integers
      return Builder->CreateCall(printfFunc, {formatStr, Arg}, "printfcall");
    }
    else if (Arg->getType()->isPointerTy())
    {

      Value *formatStr = Builder->CreateGlobalStringPtr("%s\n", "fmt"); // Use %s for strings
      return Builder->CreateCall(printfFunc, {formatStr, Arg}, "printfcall");
    }
    else
    {
      return LogErrorV("Unsupported type for nyora");
    }
  }

  // Handle other function calls.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i)
  {
    auto ArgV = Args[i]->codegen();
    if (!ArgV)
      return nullptr;
    ArgsV.push_back(ArgV);
  }

  Value *Call = Builder->CreateCall(CalleeF, ArgsV, "calltmp");

  // If the function returns void, return nullptr.
  if (CalleeF->getReturnType()->isVoidTy())
    return nullptr;

  return Call;
}

Value *IfExprAST::codegen()
{
  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  CondV = Builder->CreateFCmpONE(CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");
  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
  BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else");
  BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont");

  Builder->CreateCondBr(CondV, ThenBB, ElseBB);

  // Emit 'then' block
  Builder->SetInsertPoint(ThenBB);
  for (auto &Stmt : ThenBody)
  {
    if (!Stmt->codegen())
      return nullptr;
  }
  Builder->CreateBr(MergeBB);

  TheFunction->insert(TheFunction->end(), ElseBB);
  Builder->SetInsertPoint(ElseBB);

  for (auto &Stmt : ElseBody)
  {
    if (!Stmt->codegen())
      return nullptr;
  }
  Builder->CreateBr(MergeBB);

  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

// Output for-loop as:
//   var = alloca double
//   ...
//   start = startexpr
//   store start -> var
//   goto loop
// loop:
//   ...
//   bodyexpr
//   ...
// loopend:
//   step = stepexpr
//   endcond = endexpr
//
//   curvar = load var
//   nextvar = curvar + step
//   store nextvar -> var
//   br endcond, loop, endloop
// outloop:

Value *BlockExprAST::codegen()
{
  Value *LastValue = nullptr;

  for (auto &Stmt : Body)
  {
    LastValue = Stmt->codegen();
    if (!LastValue)
      return nullptr; // Stop on failure.
  }

  return LastValue; // Return the last evaluated expression.
}

Value *ForExprAST::codegen()
{
  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  // Create allocas for the loop variable
  AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);

  // Emit the start code first
  Value *StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;

  // Store the value into the alloca
  Builder->CreateStore(StartVal, Alloca);

  // Create the loop header block
  BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "loop", TheFunction);
  Builder->CreateBr(LoopBB);
  Builder->SetInsertPoint(LoopBB);

  // Save the old variable (if it exists)
  AllocaInst *OldVal = NamedValues[VarName];
  NamedValues[VarName] = Alloca;

  // Generate loop body (handling multiple statements)
  if (Body)
  {
    for (auto &Stmt : Body->getBody())
    { // Fixed to use getBody()
      if (!Stmt->codegen())
        return nullptr;
    }
  }

  // Compute the step value
  Value *StepVal = Step ? Step->codegen() : ConstantFP::get(*TheContext, APFloat(1.0));
  if (!StepVal)
    return nullptr;

  // Load current loop variable
  Value *CurVar = Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, VarName.c_str());

  // Compute NextVar = i + StepVal
  Value *NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar");
  Builder->CreateStore(NextVar, Alloca);

  // Compute the end condition
  Value *EndCond = End->codegen();
  if (!EndCond)
    return nullptr;

  // Compare NextVar to EndCond: NextVar < EndCond (strict comparison)
  EndCond = Builder->CreateFCmpULT(NextVar, EndCond, "loopcond");

  // Create the "after loop" block
  BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "afterloop", TheFunction);
  Builder->CreateCondBr(EndCond, LoopBB, AfterBB);
  Builder->SetInsertPoint(AfterBB);

  // Restore the old variable (if it existed)
  if (OldVal)
    NamedValues[VarName] = OldVal;
  else
    NamedValues.erase(VarName);

  // Return 0.0 (per Kaleidoscope convention)
  return ConstantFP::get(*TheContext, APFloat(0.0));
}

Value* VariableExprAST::codegen() {
  // Debug output - remove after testing
  //llvm::errs() << "Looking for variable: " << Name << "\n";
  
  // 1. Check local variables
  if (NamedValues.count(Name)) {
      AllocaInst* V = NamedValues[Name];
      return Builder->CreateLoad(V->getAllocatedType(), V, Name.c_str());
  }
  
  // 2. Check global variables
  if (GlobalNamedValues.count(Name)) {
      GlobalVariable* GV = GlobalNamedValues[Name];
      return Builder->CreateLoad(GV->getValueType(), GV, Name.c_str());
  }
  
  // 3. Check function arguments (if applicable)
  
  return LogErrorV(("Unknown variable name: " + Name).c_str());
}
Value *VarExprAST::codegen()
{
  std::vector<AllocaInst *> OldBindings;

  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  // Register all variables and emit their initializer.
  for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
  {
    const std::string &VarName = VarNames[i].first;
    ExprAST *Init = VarNames[i].second.get();

    // Emit the initializer before adding the variable to scope, this prevents
    // the initializer from referencing the variable itself, and permits stuff
    // like this:
    //  var a = 1 in
    //    var a = a in ...   # refers to outer 'a'.
    Value *InitVal;
    if (Init)
    {
      InitVal = Init->codegen();
      if (!InitVal)
        return nullptr;
    }
    else
    { // If not specified, use 0.0.
      InitVal = ConstantFP::get(*TheContext, APFloat(0.0));
    }

    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
    Builder->CreateStore(InitVal, Alloca);

    // Remember the old variable binding so that we can restore the binding when
    // we unrecurse.
    OldBindings.push_back(NamedValues[VarName]);

    // Remember this binding.
    NamedValues[VarName] = Alloca;
  }

  // Codegen the body, now that all vars are in scope.
  Value *BodyVal = Body->codegen();
  if (!BodyVal)
    return nullptr;

  // Pop all our variables from scope.
  for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
    NamedValues[VarNames[i].first] = OldBindings[i];

  // Return the body computation.
  return BodyVal;
}
Value* GlobalVarExprAST::codegen() {
  for (auto& [Name, Init] : VarNames) {
      // Check for existing global
      if (TheModule->getNamedGlobal(Name)) {
          return LogErrorV(("Redefinition of global variable " + Name).c_str());
      }

      // Get initializer value (default to 0.0 if none)
      Value* InitVal = Init ? Init->codegen() : 
          ConstantFP::get(*TheContext, APFloat(0.0));
      if (!InitVal) return nullptr;

      // Create the global variable
      auto* GV = new GlobalVariable(
          *TheModule,
          InitVal->getType(),
          false,  // isConstant
          GlobalValue::ExternalLinkage,
          dyn_cast<Constant>(InitVal),
          Name);
      
      // Add to global symbol table
      GlobalNamedValues[Name] = GV;
  }
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}


Function *FunctionAST::codegen() {
  std::string FuncName = FullName.empty() ? Proto->getName() : FullName;
  
  // Create function prototype in the Module's symbol table
  FunctionProtos[FuncName] = std::make_unique<PrototypeAST>(
      FuncName,
      Proto->getArgs(),
      Proto->isOperator(),
      Proto->getBinaryPrecedence()
  );
  
  
  
  Function *TheFunction = getFunction(FuncName);
  if (!TheFunction) {
      return nullptr;
  }
  
  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);
  
  // Record function arguments
  NamedValues.clear();
  for (auto &Arg : TheFunction->args()) {
      AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());
      Builder->CreateStore(&Arg, Alloca);
      NamedValues[std::string(Arg.getName())] = Alloca;
  }
  
  // Generate code for each expression
  Value *RetVal = nullptr;
  for (size_t i = 0; i < Body.size(); ++i) {
      RetVal = Body[i]->codegen();
      if (!RetVal) {
          return nullptr;
      }
      
      if (i == Body.size() - 1) {
          Builder->CreateRet(RetVal);
      }
  }
  
  verifyFunction(*TheFunction);
  
  TheFPM->run(*TheFunction, *TheFAM);
  
  return TheFunction;
}


Value *ClassAST::codegen() {
  
  for (auto &Method : Methods) {
      PrototypeAST* OriginalProto = Method->getProto();
      std::string MethodName = OriginalProto->getName();
      std::string FullName = Name + "." + MethodName;
      
      
      // Create and register the prototype first
      auto NewProto = std::make_unique<PrototypeAST>(
          FullName,
          OriginalProto->getArgs(),
          OriginalProto->isOperator(),
          OriginalProto->getBinaryPrecedence()
      );
      
      // Register the prototype before generating the function
      FunctionProtos[FullName] = std::move(NewProto);
      
      // Generate the function body
      std::vector<std::unique_ptr<ExprAST>> Body;
      for (auto &Expr : Method->getBody()) {
          Body.push_back(std::move(Expr));
      }
      
      auto NewFunction = std::make_unique<FunctionAST>(
          FunctionProtos[FullName]->clone(), // Use a clone of the registered prototype
          std::move(Body),
          FullName
      );
      
      if (!NewFunction->codegen()) {
          return LogErrorV(("Failed to generate method " + FullName).c_str());
      }
  }
  
  return Constant::getNullValue(Type::getInt32Ty(*TheContext));
}

Function *PrototypeAST::codegen()
{
  // Make the function type:  double(double,double) etc.
  std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
  FunctionType *FT =
      FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);

  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}





//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

void InitializeModuleAndManagers()
{
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("KaleidoscopeJIT", *TheContext);
  TheModule->setDataLayout(TheJIT->getDataLayout());

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Create new pass and analysis managers.
  TheFPM = std::make_unique<FunctionPassManager>();
  TheLAM = std::make_unique<LoopAnalysisManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheCGAM = std::make_unique<CGSCCAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext,
                                                     /*DebugLogging*/ true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());

  // Add transform passes.
  // Promote allocas to registers.
  TheFPM->addPass(PromotePass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->addPass(InstCombinePass());
  // Reassociate expressions.
  TheFPM->addPass(ReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->addPass(GVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->addPass(SimplifyCFGPass());

  // Register analysis passes used in these transform passes.
  PassBuilder PB;
  PB.registerModuleAnalyses(*TheMAM);
  PB.registerFunctionAnalyses(*TheFAM);
  PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
}

void HandleDefinition()
{
  if (auto FnAST = ParseDefinition())
  {
    if (auto *FnIR = FnAST->codegen())
    {

      ExitOnErr(TheJIT->addModule(
          ThreadSafeModule(std::move(TheModule), std::move(TheContext))));
      InitializeModuleAndManagers();
    }
  }
  else
  {
    // Skip token for error recovery.
    getNextToken();
  }
}

void HandleExtern()
{
  if (auto ProtoAST = ParseExtern())
  {
    if (auto *FnIR = ProtoAST->codegen())
    {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  }
  else
  {
    // Skip token for error recovery.
    getNextToken();
  }
}

void HandleTopLevelExpression()
{
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr())
  {
    if (FnAST->codegen())
    {
      // Create a ResourceTracker to track JIT'd memory allocated to our
      // anonymous expression -- that way we can free it after executing.
      auto RT = TheJIT->getMainJITDylib().createResourceTracker();

      auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
      ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
      InitializeModuleAndManagers();

      // Search the JIT for the __anon_expr symbol.
      auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));

      // Get the symbol's address and cast it to the right type (takes no
      // arguments, returns a double) so we can call it as a native function.
      double (*FP)() = ExprSymbol.getAddress().toPtr<double (*)()>();
      FP();
      // fprintf(stderr, "Evaluated to %f\n", FP());

      // Delete the anonymous expression module from the JIT.
      ExitOnErr(RT->remove());
    }
  }
  else
  {
    // Skip token for error recovery.
    getNextToken();
  }
}
void MainLoop() {
  while (true) {
      switch (CurTok) {
      case tok_eof:
          return;
      case ';':
          getNextToken();
          break;
      case tok_globalvar: {
          auto Global = ParseGlobalVarExpr();
          if (Global) {
              if (!Global->codegen()) {
                  LogError("Failed to codegen global variable");
              }
          }
          break;
      }
      case tok_def:
          HandleDefinition();
          break;
      case tok_class: 
          getNextToken(); // eat 'class'
          if (auto Class = ParseClass()) {
              if (!Class->codegen()) {
              }
          } else {
              getNextToken();
          }
          break;
      case tok_extern:
          HandleExtern();
          break;
      default:
          HandleTopLevelExpression();
          break;
      }
  }
}