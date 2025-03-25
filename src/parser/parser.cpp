#include "parser.h"



int CurTok;
int getNextToken() { return CurTok = gettok(); }

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
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
/// Log an error message for class parsing and return nullptr.
std::unique_ptr<ClassAST> LogErrorClass(const char *Str) {
  fprintf(stderr, "Class Parsing Error: %s\n", Str);
  return nullptr;
}


std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
std::unique_ptr<ExprAST> ParseNumberExpr()
{
  auto Result = std::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}

std::unique_ptr<ExprAST> ParseFileOperation()
{
  std::string Operation = IdentifierStr; // Save the operation name.
  getNextToken();                        // Consume the operation (e.g., "open").

  if (CurTok != '(')
  {
    return LogError("Panotarisirwa '(' wabva kushandisa izwi rekuvhura faera");
  }
  getNextToken(); // Consume '('

  if (Operation == "vhura")
  {
    auto FilePath = ParseExpression();
    if (!FilePath)
      return nullptr;

    if (CurTok != ',')
    {
      return LogError("Panotarisirwa ',' pamberi pe nzira ye faera ");
    }
    getNextToken(); // Consume ','

    auto Mode = ParseExpression();
    if (!Mode)
      return nullptr;

    if (CurTok != ')')
    {
      return LogError("Panotarisirwa ')' kuvhara basa  'vhura'");
    }
    getNextToken(); // Consume ')'

    return std::make_unique<FileOpenAST>(std::move(FilePath), std::move(Mode));
  }
  else if (Operation == "verenga" || Operation == "bvisa")
  {
    auto FilePath = ParseExpression();
    if (!FilePath)
      return nullptr;

    if (CurTok != ')')
    {
      return LogError(("Panotarisirwa ')' pamberi penzira ye faera  '" + Operation + "'").c_str());
    }
    getNextToken(); // Consume ')'

    if (Operation == "verenga")
    {
      return std::make_unique<FileReadAST>(std::move(FilePath));
    }
    else
    {
      return std::make_unique<FileDeleteAST>(std::move(FilePath));
    }
  }
  else if (Operation == "write")
  {
    auto FilePath = ParseExpression();
    if (!FilePath)
      return nullptr;

    if (CurTok != ',')
    {
      return LogError("Expected ',' after file path in 'write'");
    }
    getNextToken(); // Consume ','

    auto Content = ParseExpression();
    if (!Content)
      return nullptr;

    if (CurTok != ')')
    {
      return LogError("Expected ')' after content in 'write'");
    }
    getNextToken(); // Consume ')'

    return std::make_unique<FileWriteAST>(std::move(FilePath), std::move(Content));
  }

  return LogError(("Unknown file operation: " + Operation).c_str());
}
std::unique_ptr<ExprAST> ParseWhileExpr() {
    getNextToken(); // Eat 'while'

    if (CurTok != '(')
        return LogError("Panotarisirwa '(' pamberi pa 'ita'");
    getNextToken(); // Eat '('

    auto Cond = ParseExpression();
    if (!Cond)
        return nullptr;

    if (CurTok != ')')
        return LogError("Panotarisirwa ')' ");
    getNextToken(); // Eat ')'

    if (CurTok != '{')
        return LogError("Panotarisirwa '{'");
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
        return LogError("Panotarisirwa '}' kuvhara zvanyorwa pamusoro");
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
    return LogError("Panotarisirwa ')'");
  getNextToken(); // eat ).
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
  std::string IdName = IdentifierStr;
  // if (IdName == "vhura" || IdName == "verenga" || IdName == "nyora" || IdName == "bvisa")
  // {
  //   return ParseFileOperation();
  // }

  getNextToken(); // eat identifier.

  if (CurTok != '(') // Simple variable ref.
    return std::make_unique<VariableExprAST>(IdName);

  // Call.
  getNextToken(); // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')')
  {
    while (true)
    {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return LogError("Panotarisirwa ')' or ','");
      getNextToken();
    }
  }

  // Eat the ')'.
  getNextToken();

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
/// ifexpr ::= 'if' '(' expression ')' '{' expression '}' ('else' '{' expression '}')?
/// ifexpr ::= 'if' '(' expression ')' '{' expression '}' ('else' '{' expression '}')?
std::unique_ptr<ExprAST> ParseIfExpr() {
    getNextToken(); // Eat 'if'

    if (CurTok != '(')
        return LogError("Panotarisirwa '(' pamberi pa 'kana'");
    getNextToken(); // Eat '('

    auto Cond = ParseExpression();
    if (!Cond)
        return nullptr;

    if (CurTok != ')')
        return LogError("Panotarisirwa ')' pamberi ");
    getNextToken(); // Eat ')'

    if (CurTok != '{')
        return LogError("Panotarisirwa '{' panotangira  muviri wa'kana'");
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
        return LogError("Panotarisirwa '}' panoperera muviri wa 'kana'");
    getNextToken(); // Eat '}'

    std::vector<std::unique_ptr<ExprAST>> ElseStatements;
    if (CurTok == tok_else) {
        getNextToken(); // Eat 'else'

        if (CurTok != '{')
            return LogError("Panotarisirwa '{' panotangira muviri 'kana kuti'");
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
            return LogError("Panotarisirwa '}' panoperera muviri wa 'kana_kuti'");
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
std::unique_ptr<ExprAST> ParseForExpr()
{
  getNextToken(); // eat the for.

  if (CurTok != tok_identifier)
    return LogError("Panotarisirwa 'zita' mukati ma ");

  std::string IdName = IdentifierStr;
  getNextToken(); // eat identifier.

  if (CurTok != '=')
    return LogError("Panotarisirwa '=' pamberi pa 'pakati'");
  getNextToken(); // eat '='.

  auto Start = ParseExpression();
  if (!Start)
    return nullptr;
  if (CurTok != ',')
    return LogError("Panotarisirwa ',' ");
  getNextToken();

  auto End = ParseExpression();
  if (!End)
    return nullptr;

  // The step value is optional.
  std::unique_ptr<ExprAST> Step;
  if (CurTok == ',')
  {
    getNextToken();
    Step = ParseExpression();
    if (!Step)
      return nullptr;
  }

  if (CurTok != tok_in)
    return LogError("Panotarisirwa 'mu' pamberi pa 'pakati'");
  getNextToken(); // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

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
    return LogError("Panotarisirwa izwi pamberi pa 'zita'");

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
      return LogError("Panotarisirwa izwi pamberi pa 'zita'");
  }

  // At this point, we have to have 'in'.
  if (CurTok != tok_in)
    return LogError("Panotarisirwa 'mu' pamberi pa 'zita'");
  getNextToken(); // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<VarExprAST>(std::move(VarNames), std::move(Body));
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
  default:
    return LogError("unknown token when expecting an expression");

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

  // // Handle file operation tokens.
  // case tok_open:
  // case tok_write:
  // case tok_read:
  // case tok_delete:
  //   return ParseFileOperation(); // Delegate to ParseFileOperation.
  // 
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

  switch (CurTok)
  {
  default:
    return LogErrorP("Panotarisirwa zita re 'basa'");
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
    return LogErrorP("Panotarisirwa '('");

  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(IdentifierStr);
  if (CurTok != ')')
    return LogErrorP("Panotarisirwa ')' ");

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
        } else if (CurTok != '}') {
            return LogErrorF("Panotarisirwa'}' pakupera kwemuviri we 'basa'");
        }
    }

    if (CurTok != '}')
        return LogErrorF("Panotarisirwa'}' ");
    getNextToken(); // Eat '}'

    return std::make_unique<FunctionAST>(std::move(Proto), std::move(BodyExpressions));
}


/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
  if (auto E = ParseExpression())
  {
    // Make an anonymous proto.
    auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                std::vector<std::string>());

    // Create a vector and add the expression to it
    std::vector<std::unique_ptr<ExprAST>> Body;
    Body.push_back(std::move(E)); // Move the unique_ptr into the vector

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

std::unique_ptr<ClassAST> ParseClass() {
  getNextToken(); // Consume 'class'

  // Expect class name
  if (CurTok != tok_identifier)
      return LogErrorClass("Expected class name after 'class'");
  std::string ClassName = IdentifierStr;
  getNextToken(); // Consume class name

  // Check for inheritance
  std::string ParentClass = "";
  if (CurTok == tok_extends) {
      getNextToken(); // Consume 'extends'
      if (CurTok != tok_identifier)
          return LogErrorClass("Expected parent class name after 'extends'");
      ParentClass = IdentifierStr;
      getNextToken(); // Consume parent class name
  }

  // Expect opening brace '{'
  if (CurTok != '{')
      return LogErrorClass("Expected '{' after class declaration");
  getNextToken(); // Consume '{'

  std::vector<std::string> Fields;
  std::vector<std::unique_ptr<PrototypeAST>> Methods;

  // Parse class body
  while (CurTok != '}') {
      if (CurTok == tok_public || CurTok == tok_private) {
          getNextToken(); // Consume 'public' or 'private'
      }

      if (CurTok == tok_identifier) {
          // Field declaration (e.g., `age;`)
          Fields.push_back(IdentifierStr);
          getNextToken();
          if (CurTok != ';')
              return LogErrorClass("Expected ';' after field declaration");
          getNextToken(); // Consume ';'
      } else if (CurTok == tok_def) {
          // Method declaration
          auto Method = ParsePrototype();
          if (!Method) return nullptr;
          Methods.push_back(std::move(Method));
      } else {
          return LogErrorClass("Unexpected token in class body");
      }
  }

  getNextToken(); // Consume '}'
  ClassTable[ClassName] = ClassAST(ClassName, ParentClass, Fields, std::move(Methods));
  return std::make_unique<ClassAST>(ClassName, ParentClass, Fields, std::move(Methods));
}

std::unique_ptr<ExprAST> ParseNewExpr() {
  getNextToken(); // Consume 'new'

  if (CurTok != tok_identifier)
      return LogError("Expected class name after 'new'");

  std::string ClassName = IdentifierStr;
  getNextToken(); // Consume class name

  if (CurTok != '(')
      return LogError("Expected '(' after class name");
  getNextToken(); // Consume '('

  if (CurTok != ')')
      return LogError("Expected ')' after '(' in object instantiation");
  getNextToken(); // Consume ')'

  return std::make_unique<NewExprAST>(ClassName);
}

static std::unique_ptr<ExprAST> ParseMemberAccess(std::unique_ptr<ExprAST> Object) {
  getNextToken(); // Consume '.'

  if (CurTok != tok_identifier)
      return LogError("Expected member name after '.'");

  std::string MemberName = IdentifierStr;
  getNextToken(); // Consume member name

  // Retrieve the class name from the object (Assuming object is an identifier)
  std::string ClassName = "Unknown";  // Default if we can't resolve
  if (auto *Var = dynamic_cast<VariableExprAST *>(Object.get())) {
      ClassName = Var->getName();  // Assuming variables store class instances
  }

  return std::make_unique<MemberExprAST>(std::move(Object), MemberName, ClassName);
}


