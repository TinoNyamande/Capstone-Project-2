#include "parser.h"

int CurTok;
int getNextToken() { return CurTok = gettok(); }

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
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

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str)
{
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str)
{
  LogError(Str);
  return nullptr;
}
std::unique_ptr<FunctionAST> LogErrorF(const char *Str)
{
  LogError(Str);
  return nullptr;
}

 std::unique_ptr<ExprAST> ParseExpression();


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
/// numberexpr ::= number
std::unique_ptr<ExprAST> ParseNumberExpr()
{
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}
 std::unique_ptr<PrototypeAST> ParsePrototype()
{
  std::string FnName;

  unsigned Kind = 0; // 0 = identifier, 1 = unary, 2 = binary.
  unsigned BinaryPrecedence = 30;

  switch (CurTok)
  {
  default:
    return LogErrorP("Expected function name in prototype");
  case tok_identifier:
    FnName = IdentifierStr;
    Kind = 0;
    getNextToken();
    break;
  case tok_unary:
    getNextToken();
    if (!isascii(CurTok))
      return LogErrorP("Expected unary operator");
    FnName = "unary";
    FnName += (char)CurTok;
    Kind = 1;
    getNextToken();
    break;
  case tok_binary:
    getNextToken();
    if (!isascii(CurTok))
      return LogErrorP("Expected binary operator");
    FnName = "binary";
    FnName += (char)CurTok;
    Kind = 2;
    getNextToken();

    // Read the precedence if present.
    if (CurTok == tok_number)
    {
      if (NumVal < 1 || NumVal > 100)
        return LogErrorP("Invalid precedence: must be 1..100");
      BinaryPrecedence = (unsigned)NumVal;
      getNextToken();
    }
    break;
  }

  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(IdentifierStr);
  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  // success.
  getNextToken(); // eat ')'.

  // Verify right number of names for operator.
  if (Kind && ArgNames.size() != Kind)
    return LogErrorP("Invalid number of operands for operator");

  return std::make_unique<PrototypeAST>(FnName, ArgNames, Kind != 0,
                                        BinaryPrecedence);
}


std::unique_ptr<ExprAST> ParseWhileExpr() {
  getNextToken(); // Eat 'while'

  if (CurTok != '(')
      return LogError("yaitirwa '(' mushure me 'while'");
  getNextToken(); // Eat '('

  auto Cond = ParseExpression();
  if (!Cond)
      return nullptr;

  if (CurTok != ')')
      return LogError("yaitirwa ')' mushure me mamiriro");
  getNextToken(); // Eat ')'

  if (CurTok != '{')
      return LogError("yaitirwa '{' kutanga muviri we while");
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
      return LogError("yaitirwa '}' mushure me muviri we while");
  getNextToken(); // Eat '}'

  return std::make_unique<WhileExprAST>(std::move(Cond), std::move(BodyExpressions));
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
      return nullptr;

  if (CurTok != ')')
      return LogError("yaitirwa ')'");
  getNextToken(); // eat ).
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;
  // if (IdName == "open" || IdName == "read" || IdName == "write" || IdName == "delete") {
  //     return ParseFileOperation();
  // }

  getNextToken(); // eat identifier.

  if (CurTok != '(') // Simple variable ref.
      return std::make_unique<VariableExprAST>(IdName);

  // Call.
  getNextToken(); // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
      while (true) {
          if (auto Arg = ParseExpression())
              Args.push_back(std::move(Arg));
          else
              return nullptr;

          if (CurTok == ')')
              break;

          if (CurTok != ',')
              return LogError("Yaitirwa ')' kana ',' mune rondedzero ye argument");
          getNextToken();
      }
  }

  // Eat the ')'.
  getNextToken();

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken(); // Eat 'if'

  if (CurTok != '(')
      return LogError("yaitirwa '(' mushure me 'if'");
  getNextToken(); // Eat '('

  auto Cond = ParseExpression();
  if (!Cond)
      return nullptr;

  if (CurTok != ')')
      return LogError("yaitirwa ')' mushure me mamiriro");
  getNextToken(); // Eat ')'

  if (CurTok != '{')
      return LogError("yaitirwa '{' mushure me mamiriro e 'if'");
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
      return LogError("yaitirwa '}' mushure me chikamu che 'if'");
  getNextToken(); // Eat '}'

  std::vector<std::unique_ptr<ExprAST>> ElseStatements;
  if (CurTok == tok_else) {
      getNextToken(); // Eat 'else'

      if (CurTok != '{')
          return LogError("yaitirwa '{' mushure me 'else'");
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
          return LogError("yaitirwa '}' mushure me chikamu che 'else'");
      getNextToken(); // Eat '}'
  }

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(ThenStatements), std::move(ElseStatements));
}

std::unique_ptr<ExprAST> ParseReturnExpr() {
  getNextToken(); // eat the return keyword

  auto RetVal = ParseExpression();
  if (!RetVal)
      return nullptr;

  return std::make_unique<ReturnExprAST>(std::move(RetVal));
}

std::unique_ptr<ExprAST> ParseForExpr() {
  getNextToken(); // eat the for.

  if (CurTok != tok_identifier)
      return LogError("yaitirwa zita remusiyano mushure me 'for'");

  std::string IdName = IdentifierStr;
  getNextToken(); // eat identifier.

  if (CurTok != '=')
      return LogError("yaitirwa '=' mushure me 'for'");
  getNextToken(); // eat '='.

  auto Start = ParseExpression();
  if (!Start)
      return nullptr;

  if (CurTok != ',')
      return LogError("yaitirwa ',' mushure me kukosha kwekutanga mu 'for'");
  getNextToken();

  auto End = ParseExpression();
  if (!End)
      return nullptr;

  // The step value is optional.
  std::unique_ptr<ExprAST> Step;
  if (CurTok == ',') {
      getNextToken();
      Step = ParseExpression();
      if (!Step)
          return nullptr;
  }

  if (CurTok != tok_in)
      return LogError("yaitirwa 'in' mushure me 'for'");
  getNextToken(); // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
      return nullptr;

  return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End),
                                      std::move(Step), std::move(Body));
}

std::unique_ptr<ExprAST> ParseVarExpr() {
  getNextToken(); // eat the var.

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

  // At least one variable name is required.
  if (CurTok != tok_identifier)
      return LogError("yaitirwa zita remusiyano mushure me 'var'");

  while (true) {
      std::string Name = IdentifierStr;
      getNextToken(); // eat identifier.

      // Read the optional initializer.
      std::unique_ptr<ExprAST> Init = nullptr;
      if (CurTok == '=') {
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
          return LogError("yaitirwa rondedzero yezita remusiyano mushure me 'var'");
  }

  // At this point, we have to have 'in'.
  if (CurTok != tok_in)
      return LogError("yaitirwa 'in' mushure me 'var'");
  getNextToken(); // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
      return nullptr;

  return std::make_unique<VarExprAST>(std::move(VarNames), std::move(Body));
}

// std::unique_ptr<ExprAST> ParseIfExpr() {
//   getNextToken(); // Dya 'if'

//   if (CurTok != '(')
//       return LogError("yakatarisirwa '(' mushure me 'if'");
//   getNextToken(); // Dya '('

//   auto Cond = ParseExpression();
//   if (!Cond)
//       return nullptr;

//   if (CurTok != ')')
//       return LogError("yakatarisirwa ')' mushure memamiriro");
//   getNextToken(); // Dya ')'

//   if (CurTok != '{')
//       return LogError("yakatarisirwa '{' mushure memamiriro e 'if'");
//   getNextToken(); // Dya '{'

//   std::vector<std::unique_ptr<ExprAST>> ThenStatements;
//   while (CurTok != '}' && CurTok != tok_eof) {
//       if (auto E = ParseExpression()) {
//           ThenStatements.push_back(std::move(E));
//       } else {
//           return nullptr;
//       }

//       if (CurTok == ';') {
//           getNextToken(); // Dya semicolon
//       }
//   }

//   if (CurTok != '}')
//       return LogError("yakatarisirwa '}' mushure me 'if' block");
//   getNextToken(); // Dya '}'

//   std::vector<std::unique_ptr<ExprAST>> ElseStatements;
//   if (CurTok == tok_else) {
//       getNextToken(); // Dya 'else'

//       if (CurTok != '{')
//           return LogError("yakatarisirwa '{' mushure me 'else'");
//       getNextToken(); // Dya '{'

//       while (CurTok != '}' && CurTok != tok_eof) {
//           if (auto E = ParseExpression()) {
//               ElseStatements.push_back(std::move(E));
//           } else {
//               return nullptr;
//           }

//           if (CurTok == ';') {
//               getNextToken();
//           }
//       }

//       if (CurTok != '}')
//           return LogError("yakatarisirwa '}' mushure me 'else' block");
//       getNextToken(); // Dya '}'
//   }

//   return std::make_unique<IfExprAST>(std::move(Cond), std::move(ThenStatements), std::move(ElseStatements));
// }
/// tsanangudzo ::= 'def' prototype expression
/// tsanangudzo ::= 'def' prototype expression
std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken(); // Dya 'def'
  auto Proto = ParsePrototype();
  if (!Proto)
      return nullptr;

  if (CurTok != '{')
      return LogErrorF("Yakatarisirwa '{' kutanga muviri webasa");
  getNextToken(); // Dya '{'

  std::vector<std::unique_ptr<ExprAST>> BodyExpressions;

  while (CurTok != '}' && CurTok != tok_eof) {
      if (auto E = ParseExpression()) {
          BodyExpressions.push_back(std::move(E));
      } else {
          return nullptr;
      }

      if (CurTok == ';') {
          getNextToken();
      } else if (CurTok != '}') {
          return LogErrorF("Yakatarisirwa '}' kuvhara muviri webasa");
      }
  }

  if (CurTok != '}')
      return LogErrorF("Yakatarisirwa '}' kuvhara muviri webasa");
  getNextToken(); // Dya '}'

  return std::make_unique<FunctionAST>(std::move(Proto), std::move(BodyExpressions));
}

/// chirevo chepamusoro-soro ::= chirevo
std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
if (auto E = ParseExpression())
{
  // Gadzira prototype isina zita.
  auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                              std::vector<std::string>());

  // Gadzira vector uye wedzera chirevo mariri
  std::vector<std::unique_ptr<ExprAST>> Body;
  Body.push_back(std::move(E)); // Fambisa unique_ptr muvector

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