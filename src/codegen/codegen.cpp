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
static bool ModuleInitialized = false;
static std::mutex global_var_mutex;

#define DEBUG_LOG(msg) std::cerr << "[DEBUG] " << msg << "\n"

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
  
  // First check global variables
  {
    std::lock_guard<std::mutex> lock(global_var_mutex);
    if (GlobalNamedValues.count(Name)) {
      GlobalVariable* GV = GlobalNamedValues[Name];
      if (!GV) {
        return LogErrorV("Null global variable");
      }
      return Builder->CreateLoad(GV->getValueType(), GV, Name.c_str());
    }
  }
  
  // Then check local variables
  AllocaInst* A = NamedValues[Name];
  if (!A)
    return LogErrorV("Unknown variable name");
  
  return Builder->CreateLoad(A->getAllocatedType(), A, Name.c_str());
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


std::string LLVMTypeToString(llvm::Type* type) {
  std::string str;
  llvm::raw_string_ostream rso(str);
  type->print(rso);
  return rso.str();
}

std::string LLVMValueToString(llvm::Value* val) {
  std::string str;
  llvm::raw_string_ostream rso(str);
  val->print(rso);
  return rso.str();
}
 
void DumpGlobalVariables() {
  DEBUG_LOG("=== GLOBAL VARIABLE DUMP (" << GlobalNamedValues.size() << ") ===");
  for (const auto& [name, gv] : GlobalNamedValues) {
    DEBUG_LOG("'" << name << "'");
    DEBUG_LOG("  Address: " << (void*)gv);
    if (!gv) {
      DEBUG_LOG("  WARNING: NULL GLOBAL VARIABLE!");
      continue;
    }
    DEBUG_LOG("  Type: " + LLVMTypeToString(gv->getValueType()));
    DEBUG_LOG("  Initializer: " << (void*)gv->getInitializer());
    if (gv->hasInitializer()) {
      DEBUG_LOG("  Init Value: " + LLVMValueToString(gv->getInitializer()));
    }
    DEBUG_LOG("  IsConstant: " << gv->isConstant());
    DEBUG_LOG("  Linkage: " << gv->getLinkage());
  }
  DEBUG_LOG("=== END DUMP ===");
}
Value* GlobalVarExprAST::codegen() {
  
  for (auto& [Name, Init] : VarNames) {
    
    if (TheModule->getNamedGlobal(Name)) {

      return LogErrorV(("Redefinition of global variable " + Name).c_str());
    }
    if (GlobalNamedValues.count(Name)) {
        
      return LogErrorV(("Internal error with global variable " + Name).c_str());
    }

    Value* InitVal = nullptr;
    if (Init) {
      InitVal = Init->codegen();
      if (!InitVal) {
        return nullptr;
      }
    } else {
      InitVal = ConstantFP::get(*TheContext, APFloat(0.0));
    }

    auto* ConstInit = dyn_cast<Constant>(InitVal);
    if (!ConstInit) {

      return LogErrorV(("Initializer for " + Name + " must be constant").c_str());
    }

    auto* GV = new GlobalVariable(
      *TheModule,
      InitVal->getType(),
      false,
      GlobalValue::ExternalLinkage,
      ConstInit,
      Name
    );

    GlobalNamedValues[Name] = GV;
  }

 
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Function* FunctionAST::codegen(const std::string& FuncNameOverride) {
  std::string FuncName = FuncNameOverride.empty() ? getName() : FuncNameOverride;

  // Get the function type from the prototype (or build it)
  FunctionType *FT = FunctionType::get(Type::getDoubleTy(*TheContext), false);

  // Create the function with the resolved name
  Function *TheFunction = Function::Create(
      FT,
      Function::ExternalLinkage,
      FuncName,
      TheModule.get()
  );

  // Create a new basic block to start insertion into
  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);

  // Generate the body
  for (auto &Expr : Body) {
    if (!Expr->codegen()) {
      TheFunction->eraseFromParent(); // cleanup
      return nullptr;
    }
  }

  // Return 0.0 by default if the body has no return (optional)
  Builder->CreateRet(ConstantFP::get(*TheContext, APFloat(0.0)));

  // Verify the function
  verifyFunction(*TheFunction);

  return TheFunction;
}


Value *ClassAST::codegen() {

  // Handle Methods
  for (auto &Method : Methods) {
    PrototypeAST* OriginalProto = Method->getProto();
    std::string MethodName = OriginalProto->getName();
    std::string FullName = Name + "." + MethodName;


    auto NewProto = std::make_unique<PrototypeAST>(
        FullName,
        OriginalProto->getArgs(),
        OriginalProto->isOperator(),
        OriginalProto->getBinaryPrecedence()
    );

    FunctionProtos[FullName] = std::move(NewProto);

    // Move method body
    std::vector<std::unique_ptr<ExprAST>> Body;
    for (auto &Expr : Method->getBody()) {
      Body.push_back(std::move(Expr));
    }

    auto NewFunction = std::make_unique<FunctionAST>(
        FunctionProtos[FullName]->clone(),
        std::move(Body),
        FullName
    );

    if (!NewFunction->codegen()) {
      return LogErrorV(("Failed to generate method " + FullName).c_str());
    }
  }

  // Handle Members
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> QualifiedMembers;

  for (auto &Member : Members) {
    std::string FullVarName = Name + "." + Member.first;
    QualifiedMembers.emplace_back(FullVarName, std::move(Member.second));
  }

  auto GlobalVars = std::make_unique<GlobalVarExprAST>(std::move(QualifiedMembers));

  if (!GlobalVars->codegen()) {
    return LogErrorV(("Failed to generate class member variables for " + Name).c_str());
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

void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
      if (auto *FnIR = FnAST->codegen()) {
          // Don't add to JIT yet — handled in MainLoop
      }
  } else {
      getNextToken();
  }
}


std::unique_ptr<Module> CloneModule(Module& M, LLVMContext& Context) {
  auto NewModule = std::make_unique<Module>("jit_module", Context);
  NewModule->setDataLayout(M.getDataLayout());
  
  // Value mapping for cloning
  ValueToValueMapTy VMap;
  
  // Clone all global variables
  for (auto& Global : M.globals()) {
    auto* NewGlobal = new GlobalVariable(
        *NewModule,
        Global.getValueType(),
        Global.isConstant(),
        Global.getLinkage(),
        nullptr, // Initializer handled below
        Global.getName()
    );
    NewGlobal->copyAttributesFrom(&Global);
    VMap[&Global] = NewGlobal;
  }
  
  // Clone initializers
  for (auto& Global : M.globals()) {
    if (Global.hasInitializer()) {
      auto* NewGlobal = cast<GlobalVariable>(VMap[&Global]);
      NewGlobal->setInitializer(MapValue(Global.getInitializer(), VMap));
    }
  }
  
// Clone function declarations
for (Function &F : M.functions()) {
  Function *NewF = Function::Create(
      cast<FunctionType>(F.getFunctionType()),
      F.getLinkage(),
      F.getName(),
      NewModule.get()
  );
  NewF->copyAttributesFrom(&F);
  VMap[&F] = NewF;
}

// Clone function bodies
for (Function &F : M.functions()) {
  if (!F.isDeclaration()) {
    Function *NewF = cast<Function>(VMap[&F]);
    SmallVector<ReturnInst*, 8> Returns;
    CloneFunctionInto(NewF, &F, VMap, CloneFunctionChangeType::DifferentModule, Returns);
  }
}

  
  return NewModule;
}
void HandleTopLevelExpression() {
  static int AnonCount = 0;
  std::string FuncName = "__anon_expr" + std::to_string(AnonCount++);

  if (auto FnAST = ParseTopLevelExpr()) {
    // Pass the function name override
    if (FnAST->codegen(FuncName)) {

      // Create resource tracker
      auto RT = TheJIT->getMainJITDylib().createResourceTracker();

      // Use a temporary context for cloning
      auto TmpContext = std::make_unique<LLVMContext>();
      auto ClonedModule = CloneModule(*TheModule, *TmpContext);

      // Create the ThreadSafeModule
      orc::ThreadSafeModule TSM(std::move(ClonedModule), std::move(TmpContext));

      // Add to JIT
      ExitOnErr(TheJIT->addModule(std::move(TSM), RT));

      // Lookup and execute the unique function
      auto ExprSymbol = ExitOnErr(TheJIT->lookup(FuncName));
      auto FP = ExprSymbol.getAddress().toPtr<double (*)()>();
      FP();

      // Clean up
      ExitOnErr(RT->remove());
    }
  } else {
    getNextToken(); // Skip on error
  }
}


void MainLoop() {
  bool needsModuleAdd = false;

  while (true) {
      switch (CurTok) {
          case tok_eof: {
              if (needsModuleAdd) {

                  ExitOnErr(TheJIT->addModule(
                      ThreadSafeModule(std::move(TheModule), std::move(TheContext))
                  ));
                  InitializeModuleAndManagers(); // Prep for possible future input
                  needsModuleAdd = false;
              }
              return;
          }

          case tok_globalvar: {
            auto Global = ParseGlobalVarExpr();
            if (Global && Global->codegen()) {
                // Keep the module with globals alive
                needsModuleAdd = true;
            } else {
                LogError("Failed to codegen global variable");
            }
            break;
        }
        

          case ';':
              getNextToken(); // Skip empty statement
              break;

          case tok_def:
              HandleDefinition();
              needsModuleAdd = true;
              break;

          case tok_class: {
              getNextToken(); // eat 'class'
              if (auto Class = ParseClass()) {
                  if (!Class->codegen()) {
                  } else {
                      needsModuleAdd = true;
                  }
              } else {
                  getNextToken(); // Skip token to avoid infinite loop
              }
              break;
          }

          default:
              // ✅ Don't reinitialize the module!
              HandleTopLevelExpression();
              break;
      }
  }
}