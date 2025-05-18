#include "parser.h"

int CurTok;
int getNextToken() { return CurTok = gettok(); }


std::map<char, int> BinopPrecedence;

 int GetTokPrecedence()
{
  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Kukanganisa pa line %d: %s\n", CurrentLine, Str);
  return nullptr;
}

std::unique_ptr<ClassAST> LogErrorC(const char *Str) {
  LogError(Str);
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  return nullptr;
}

std::unique_ptr<FunctionAST> LogErrorF(const char *Str) {
  LogError(Str);
  return nullptr;
}


 std::unique_ptr<ExprAST> ParseExpression();


// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr()
{
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}
 
  std::unique_ptr<ExprAST> ParseWhileExpr() {
     getNextToken(); // Eat 'while'
 
     if (CurTok != '(')
     return LogError("Inotarisirwa '(' mushure me 'kusvika'");
   getNextToken(); // Eat '('
   
   auto Cond = ParseExpression();
   if (!Cond)
     return nullptr;
   
   if (CurTok != ')')
     return LogError("Inotarisirwa ')' mushure me condition");
   getNextToken(); // Eat ')'
   
   if (CurTok != '{')
     return LogError("Inotarisirwa '{' kutanga muviri we 'kusvika'");
   getNextToken(); // Eat '{'
 
 
     std::vector<std::unique_ptr<ExprAST>> BodyExpressions;
     
     // Parse multiple statements until '}' is found
     while (CurTok != '}' && CurTok != tok_eof) {
         if (auto E = ParseExpression()) {
             BodyExpressions.push_back(std::move(E));
         } else {
             return nullptr; // Stop parsing on error.
         }
 
         // Consume a semicolon between statements (optional)
         if (CurTok == ';') {
             getNextToken();
         }
     }
 
     if (CurTok != '}')
        return LogError("Panotarisirwa '}' mushure me muviri we 'kusvika'");
     getNextToken(); // Eat '}'
 
     return std::make_unique<WhileExprAST>(std::move(Cond), std::move(BodyExpressions));
 }
 
 
 /// parenexpr ::= '(' expression ')'
  std::unique_ptr<ExprAST> ParseParenExpr()
 {
   getNextToken(); // eat (.
   auto V = ParseExpression();
   if (!V)
     return nullptr;
 
   if (CurTok != ')')
      return LogError("panotarisirwa ')'");
   getNextToken(); // eat ).
   return V;
 }
 

 std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;
  getNextToken(); // eat identifier

  // Regular variable
  if (CurTok != '(' && CurTok != '.') {
    return std::make_unique<VariableExprAST>(IdName);
  }

  // Handle Class access
  if (CurTok == '.') {
    getNextToken(); // eat '.'

  if (CurTok != tok_identifier)
    return LogError("Panotarisirwa zita mushure me '.'");


  std::string MemberName = IdentifierStr;
  getNextToken();

    // Variable access: Class.VarName
  if (CurTok != '(') {
      std::string FullVar = IdName + "." + MemberName;
      return std::make_unique<VariableExprAST>(FullVar);
    }

    // Method access: Class.Method()
    getNextToken(); // eat '('
    std::vector<std::unique_ptr<ExprAST>> Args;

    if (CurTok != ')') {
      while (true) {
        if (auto Arg = ParseExpression())
          Args.push_back(std::move(Arg));
        else
          return nullptr;

        if (CurTok == ')') break;
        if (CurTok != ',')
          return LogError("Panotarisirwa ',' or ')' mu list rema argument");

        getNextToken();
      }
    }

    getNextToken(); // eat ')'
    std::string FullMethod = IdName + "." + MemberName;
    return std::make_unique<CallExprAST>(FullMethod, std::move(Args));
  }

  // Simple function call
  getNextToken(); // eat '('
  std::vector<std::unique_ptr<ExprAST>> Args;

  if (CurTok != ')') {
    while (true) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')') break;
      if (CurTok != ',')
        return LogError("Panotarisirwa )");

      getNextToken();
    }
  }

  getNextToken(); // eat ')'
  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}


 
 /// ifexpr ::= 'if' expression 'then' expression 'else' expression
 /// ifexpr ::= 'if' '(' expression ')' '{' expression '}' ('else' '{' expression '}')?
 /// ifexpr ::= 'if' '(' expression ')' '{' expression '}' ('else' '{' expression '}')?
  std::unique_ptr<ExprAST> ParseIfExpr() {
     getNextToken(); // Eat 'if'
 
     if (CurTok != '(')
        return LogError("Panotarisirwa'(' mushure me 'kana'");
     getNextToken(); // Eat '('
 
     auto Cond = ParseExpression();
     if (!Cond)
         return nullptr;
 
     if (CurTok != ')')
        return LogError("Panotarisirwa ')' mushure ma kana");
     getNextToken(); // Eat ')'
 
     if (CurTok != '{')
         return LogError("Panotarisirwa '{' ");
     getNextToken(); // Eat '{'
 
     std::vector<std::unique_ptr<ExprAST>> ThenStatements;
     while (CurTok != '}' && CurTok != tok_eof) {
         if (auto E = ParseExpression()) {
             ThenStatements.push_back(std::move(E));
         } else {
             return nullptr;
         }
 
         if (CurTok == ';') {
             getNextToken(); // Eat semicolon
         }
     }
 
     if (CurTok != '}')
         return LogError("Panotarisirwa '}' mushure me chikamu che 'kana'");
     getNextToken(); // Eat '}'
 
     std::vector<std::unique_ptr<ExprAST>> ElseStatements;
     if (CurTok == tok_else) {
         getNextToken(); // Eat 'else'
 
         if (CurTok != '{')
             return LogError("Panotarisirwa '{' mushure me 'kanakuti'");
         getNextToken(); // Eat '{'
 
         while (CurTok != '}' && CurTok != tok_eof) {
             if (auto E = ParseExpression()) {
                 ElseStatements.push_back(std::move(E));
             } else {
                 return nullptr;
             }
 
             if (CurTok == ';') {
                 getNextToken();
             }
         }
 
         if (CurTok != '}')
             return LogError("Panotarisirwa '}' mukupera kwe code ya 'kanakuti'");
         getNextToken(); // Eat '}'
     }
 
     return std::make_unique<IfExprAST>(std::move(Cond), std::move(ThenStatements), std::move(ElseStatements));
 }
 
  std::unique_ptr<ExprAST> ParseReturnExpr()
 {
   getNextToken(); // eat the return keyword
 
   auto RetVal = ParseExpression();
   if (!RetVal)
     return nullptr;
 
   return std::make_unique<ReturnExprAST>(std::move(RetVal));
 }
 /// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
 std::unique_ptr<ExprAST> ParseForExpr() {
  getNextToken(); // eat 'for'

  if (CurTok != '(')
      return LogError("Panotarisirwa '(' mushure me 'pakati'");
  getNextToken(); // eat '('.

  // Expect variable name inside parentheses
  if (CurTok != tok_identifier)
      return LogError("Panotarisirwa 'zita'  mukati me 'pakati ()'");
  
  std::string IdName = IdentifierStr;
  getNextToken(); // eat identifier.

  if (CurTok != '=')
      return LogError("Panotarisirwa '=' mushure me 'zita' mu 'pakati ()'");
  getNextToken(); // eat '='.

  auto Start = ParseExpression();
  if (!Start)
      return nullptr;

  if (CurTok != ',')
      return LogError("Panotarisirwa ',' mushure me kukosha kwekutanga mu 'pakati ()'");
  getNextToken(); // eat ','.

  auto End = ParseExpression();
  if (!End)
      return nullptr;

  // The step value is optional.
  std::unique_ptr<ExprAST> Step;
  if (CurTok == ',') {
      getNextToken(); // eat ','.
      Step = ParseExpression();
      if (!Step)
          return nullptr;
  }

  if (CurTok != ')')
      return LogError("Panotarisirwa ')'");
  getNextToken(); // eat ')'.

  // Expect opening curly brace for the loop body.
  if (CurTok != '{')
      return LogError("Panotarisirwa '{' mushure me 'pakati ()'");
  getNextToken(); // eat '{'.

  // Parse multiple expressions inside the loop body.
  std::vector<std::unique_ptr<ExprAST>> BodyStmts;
  while (CurTok != '}' && CurTok != tok_eof) {
      auto Expr = ParseExpression();
      if (!Expr)
          return nullptr;
      BodyStmts.push_back(std::move(Expr));
  }

  if (CurTok != '}')
      return LogError("Panotarisirwa '}' mushure me loop body");
  getNextToken(); // eat '}'.

  // Create a BlockExprAST to store multiple statements.
  auto Body = std::make_unique<BlockExprAST>(std::move(BodyStmts));

  return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End),
                                      std::move(Step), std::move(Body));
}

 
 /// varexpr ::= 'var' identifier ('=' expression)?
 //                    (',' identifier ('=' expression)?)* 'in' expression
 std::unique_ptr<ExprAST> ParseVarExpr()
 {
   getNextToken(); // eat the var.
 
   std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
 
   // At least one variable name is required.
   if (CurTok != tok_identifier)
     return LogError("Panotarisirwa zita remusiyano mushure me 'zita'");
 
   while (true)
   {
     std::string Name = IdentifierStr;
     getNextToken(); // eat identifier.
 
     // Read the optional initializer.
     std::unique_ptr<ExprAST> Init = nullptr;
     if (CurTok == '=')
     {
       getNextToken(); // eat the '='.
 
       Init = ParseExpression();
       if (!Init)
         return nullptr;
     }
 
     VarNames.push_back(std::make_pair(Name, std::move(Init)));
 
     // End of var list, exit loop.
     if (CurTok != ',')
       break;
     getNextToken(); // eat the ','.
 
     if (CurTok != tok_identifier)
       return LogError("Panotarisirwa rondedzero yezita remusiyano mushure me 'zita'");
   }
 
   // At this point, we have to have 'in'.
   if (CurTok != tok_in)
     return LogError("Panotarisirwa 'in' mushure me 'zita'");
   getNextToken(); // eat 'in'.
 
   auto Body = ParseExpression();
   if (!Body)
     return nullptr;
 
   return std::make_unique<VarExprAST>(std::move(VarNames), std::move(Body));
 }
 std::unique_ptr<ExprAST> ParseGlobalVarExpr() {
  getNextToken(); // eat globalvar
  
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> Vars;
  
  if (CurTok != tok_identifier)
      return LogError("expected variable name after 'zita'");

  while (true) {
      std::string Name = IdentifierStr;
      getNextToken(); // eat identifier

      std::unique_ptr<ExprAST> Init = nullptr;
      if (CurTok == '=') {
          getNextToken(); // eat '='
          Init = ParseExpression();
          if (!Init) return nullptr;
      }

      Vars.emplace_back(Name, std::move(Init));

      if (CurTok != ',') break;
      getNextToken(); // eat ','
      
      if (CurTok != tok_identifier)
          return LogError("Panotarisirwa zita pamberi pe comma");
  }
  
  return std::make_unique<GlobalVarExprAST>(std::move(Vars));
}
 /// primary
 ///   ::= identifierexpr
 ///   ::= numberexpr
 ///   ::= parenexpr
 ///   ::= ifexpr
 ///   ::= forexpr
 ///   ::= varexpr
  std::unique_ptr<ExprAST> ParsePrimary()
 {
   switch (CurTok)
   {
   default:{
     return LogError("Paita izwi risiri kuzivikanwa");
   }
   case tok_identifier:
     return ParseIdentifierExpr();
 
   case tok_number:
     return ParseNumberExpr();
 
   case '(':
     return ParseParenExpr();
 
   case tok_if:
     return ParseIfExpr();
 
   case tok_for:
     return ParseForExpr();
 
   case tok_var:
     return ParseVarExpr();

 
   case tok_return:
     return ParseReturnExpr();
   case tok_while:
     return ParseWhileExpr();
 
   case tok_string:
   {
     auto Result = std::make_unique<StringExprAST>(IdentifierStr);
     getNextToken(); // Consume the string token
     return Result;
   }
 

   }
 }
 
 /// unary
 ///   ::= primary
 ///   ::= '!' unary
  std::unique_ptr<ExprAST> ParseUnary()
 {
   // If the current token is not an operator, it must be a primary expr.
   if (!isascii(CurTok) || CurTok == '(' || CurTok == ',')
     return ParsePrimary();
 
   // If this is a unary operator, read it.
   int Opc = CurTok;
   getNextToken();
   if (auto Operand = ParseUnary())
     return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
   return nullptr;
 }
 
 /// binoprhs
 ///   ::= ('+' unary)*
  std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                               std::unique_ptr<ExprAST> LHS)
 {
   // If this is a binop, find its precedence.
   while (true)
   {
     int TokPrec = GetTokPrecedence();
 
     // If this is a binop that binds at least as tightly as the current binop,
     // consume it, otherwise we are done.
     if (TokPrec < ExprPrec)
       return LHS;
 
     // Okay, we know this is a binop.
     int BinOp = CurTok;
     getNextToken(); // eat binop
 
     // Parse the unary expression after the binary operator.
     auto RHS = ParseUnary();
     if (!RHS)
       return nullptr;
 
     // If BinOp binds less tightly with RHS than the operator after RHS, let
     // the pending operator take RHS as its LHS.
     int NextPrec = GetTokPrecedence();
     if (TokPrec < NextPrec)
     {
       RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
       if (!RHS)
         return nullptr;
     }
 
     // Merge LHS/RHS.
     LHS =
         std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
   }
 }
 std::unique_ptr<ClassAST> ParseClass() {
  
  if (CurTok != tok_identifier) {
      return LogErrorC("Panotarisirwa zita rekirasi pamberi pa 'kirasi'");
  }
  
  std::string ClassName = IdentifierStr;
  getNextToken(); // eat class name

  if (CurTok != '{') {
      return LogErrorC("Panotarisirwa '{' pamberi pezita rekirasi");
  }
  getNextToken(); // eat '{'

  std::vector<std::unique_ptr<FunctionAST>> Methods;
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> Members;


  while (CurTok != '}' && CurTok != tok_eof) {
      if (CurTok == tok_def) {
          if (auto Fn = ParseDefinition()) {
              Methods.push_back(std::move(Fn));
          } else {
              return nullptr;
          }
      }
      else if (CurTok == tok_globalvar) {
        // Member variable definition
        getNextToken(); // eat 'var'
        
        if (CurTok != tok_identifier)
            return LogErrorC("Panotarisirwa izwi amberi pa 'zita'");
        
        std::string VarName = IdentifierStr;
        getNextToken();
        
        std::unique_ptr<ExprAST> Init = nullptr;
        if (CurTok == '=') {
            getNextToken();
            Init = ParseExpression();
        }
        
        Members.emplace_back(VarName, std::move(Init));
    }

       else {
          return LogErrorC("Panotwarisirwa basa");
      }
  }

  if (CurTok != '}') {
      return LogErrorC("Panptarisirwa'}' pakupera kwe kirasi");
  }
  getNextToken(); // eat '}'

  return std::make_unique<ClassAST>(ClassName, std::move(Methods),std::move(Members));
}

 
 /// expression
 ///   ::= unary binoprhs
 ///
  std::unique_ptr<ExprAST> ParseExpression()
 {
   // Parse the left-hand side of the expression.
   auto LHS = ParseUnary();
   if (!LHS)
     return nullptr;
 
   // Parse the rest of the expression (binary operations).
   auto Result = ParseBinOpRHS(0, std::move(LHS));
 
   // Ignore any trailing semicolons.
   while (CurTok == ';')
     getNextToken(); // Consume the semicolon.
 
   return Result;
 }
 
 /// prototype
 ///   ::= id '(' id* ')'
 ///   ::= binary LETTER number? (id, id)
 ///   ::= unary LETTER (id)
  std::unique_ptr<PrototypeAST> ParsePrototype()
 {
   std::string FnName;
 
   unsigned Kind = 0; // 0 = identifier, 1 = unary, 2 = binary.
   unsigned BinaryPrecedence = 30;
   if (CurTok != tok_identifier) {
    return LogErrorP("Panotarisirwa zita re 'basa'");
   }
   FnName = IdentifierStr;
   Kind = 0;
   getNextToken();
 
 
  //}
 
   if (CurTok != '(')
     return LogErrorP("Panotarisirwa '('");
 
   std::vector<std::string> ArgNames;
   while (getNextToken() == tok_identifier)
     ArgNames.push_back(IdentifierStr);
   if (CurTok != ')')
      return LogErrorP("Panotarisirwa ')'");
 
   // success.
   getNextToken(); // eat ')'.
 
   // Verify right number of names for operator.
   if (Kind && ArgNames.size() != Kind)
     return LogErrorP("Invalid number of operands for operator");
 
   return std::make_unique<PrototypeAST>(FnName, ArgNames, Kind != 0,
                                         BinaryPrecedence);
 }
 
 /// definition ::= 'def' prototype expression
  std::unique_ptr<FunctionAST> ParseDefinition() {
     getNextToken(); // Eat 'def'
     if (CurTok != tok_identifier) {
      return LogErrorF("Panotarisirwa zita re basa mushure me 'basa'");
    }
     std::string FName = IdentifierStr;
     auto Proto = ParsePrototype();
     if (!Proto)
         return nullptr;
 
     if (CurTok != '{')
         return LogErrorF("Panotarisirwa '{' kutanga muviri we 'basa'");
     getNextToken(); // Eat '{'
 
     std::vector<std::unique_ptr<ExprAST>> BodyExpressions;
 
     while (CurTok != '}' && CurTok != tok_eof) {
         if (auto E = ParseExpression()) {
             BodyExpressions.push_back(std::move(E));
         } else {
             return nullptr;
         }
 
         if (CurTok == ';') {
             getNextToken();
         }
        //   else if (CurTok != '}') {
        //      return LogErrorF("Expected '}' to close function body");
        //  }
     }
 
     if (CurTok != '}')
         return LogErrorF("Panotarisirwa '}' kupedza muviri we 'basa'");
     getNextToken(); // Eat '}'
 
     return std::make_unique<FunctionAST>(std::move(Proto), std::move(BodyExpressions),FName);
 }
 
 
 /// toplevelexpr ::= expression
 std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  // First check for global variables
  if (CurTok == tok_globalvar) {
      // Parse the global but don't create a function for it
      auto Global = ParseGlobalVarExpr();
      if (!Global) return nullptr;
      
      // Immediately generate code for the global variable
      Global->codegen();
      
      // Return an empty function to maintain the expected return type
      auto Proto = std::make_unique<PrototypeAST>("__global_decl", 
                                                std::vector<std::string>());
      return std::make_unique<FunctionAST>(std::move(Proto), 
                                         std::vector<std::unique_ptr<ExprAST>>());
  }

  // Original functionality for all other cases
  if (auto E = ParseExpression()) {
      // Make an anonymous proto
      auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                std::vector<std::string>());
      
      // Create a vector and add the expression to it
      std::vector<std::unique_ptr<ExprAST>> Body;
      Body.push_back(std::move(E));
      
      return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
  }
  return nullptr;
}
 
 /// external ::= 'extern' prototype
  std::unique_ptr<PrototypeAST> ParseExtern()
 {
   getNextToken(); // eat extern.
   return ParsePrototype();
 }