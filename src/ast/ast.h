#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include "../lexer/lexer.h"


extern std::unique_ptr<IRBuilder<>> Builder;
  
  class ExprAST
  {
  public:
    virtual ~ExprAST() = default;

    virtual Value *codegen() = 0;
  };

  /// NumberExprAST - Expression class for numeric literals like "1.0".
  class NumberExprAST : public ExprAST
  {
    double Val;

  public:
    NumberExprAST(double Val) : Val(Val) {}

    Value *codegen() override;
  };

  class StringExprAST : public ExprAST
  {
    std::string Val;

  public:
    StringExprAST(const std::string &Val) : Val(Val) {}
    Value *codegen() override;
  };

  /// VariableExprAST - Expression class for referencing a variable, like "a".
  class VariableExprAST : public ExprAST
  {
    std::string Name;

  public:
    VariableExprAST(const std::string &Name) : Name(Name) {}

    Value *codegen() override;
    const std::string &getName() const { return Name; }
  };

  /// UnaryExprAST - Expression class for a unary operator.
  class UnaryExprAST : public ExprAST
  {
    char Opcode;
    std::unique_ptr<ExprAST> Operand;

  public:
    UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
        : Opcode(Opcode), Operand(std::move(Operand)) {}

    Value *codegen() override;
  };

  class WhileExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Cond;
    std::vector<std::unique_ptr<ExprAST>> Body;
public:
    WhileExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Body)
        : Cond(std::move(Cond)), Body(std::move(Body)) {}

    Value *codegen() override;
};



  /// BinaryExprAST - Expression class for a binary operator.
  class BinaryExprAST : public ExprAST
  {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

  public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    Value *codegen() override;
  };

  /// CallExprAST - Expression class for function calls.
  class CallExprAST : public ExprAST
  {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

  public:
    CallExprAST(const std::string &Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}

    Value *codegen() override;
  };

  /// IfExprAST - Expression class for if/then/else.
  class IfExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Cond;
    std::vector<std::unique_ptr<ExprAST>> ThenBody;
    std::vector<std::unique_ptr<ExprAST>> ElseBody;

public:
    IfExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> ThenBody, std::vector<std::unique_ptr<ExprAST>> ElseBody)
        : Cond(std::move(Cond)), ThenBody(std::move(ThenBody)), ElseBody(std::move(ElseBody)) {}

    Value *codegen() override;
};

  /// ForExprAST - Expression class for for/in.
  class ForExprAST : public ExprAST
  {
    std::string VarName;
    std::unique_ptr<ExprAST> Start, End, Step, Body;

  public:
    ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
               std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
               std::unique_ptr<ExprAST> Body)
        : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
          Step(std::move(Step)), Body(std::move(Body)) {}

    Value *codegen() override;
  };

  /// VarExprAST - Expression class for var/in
  class VarExprAST : public ExprAST
  {
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
    std::unique_ptr<ExprAST> Body;

  public:
    VarExprAST(
        std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
        std::unique_ptr<ExprAST> Body)
        : VarNames(std::move(VarNames)), Body(std::move(Body)) {}

    Value *codegen() override;
  };

  class ReturnExprAST : public ExprAST
  {
    std::unique_ptr<ExprAST> RetVal;

  public:
    ReturnExprAST(std::unique_ptr<ExprAST> RetVal)
        : RetVal(std::move(RetVal)) {}

    Value *codegen() override;
  };

  /// FileOpenAST - Represents opening a file.
  class FileOpenAST : public ExprAST
  {
    std::unique_ptr<ExprAST> FilePath;
    std::unique_ptr<ExprAST> Mode;

  public:
    FileOpenAST(std::unique_ptr<ExprAST> FilePath, std::unique_ptr<ExprAST> Mode)
        : FilePath(std::move(FilePath)), Mode(std::move(Mode)) {}

    Value *codegen() override;
  };

  /// FileReadAST - Represents reading from a file.
  class FileReadAST : public ExprAST
  {
    std::unique_ptr<ExprAST> FilePath;

  public:
    FileReadAST(std::unique_ptr<ExprAST> FilePath)
        : FilePath(std::move(FilePath)) {}

    Value *codegen() override;
  };

  /// FileWriteAST - Represents writing to a file.
  class FileWriteAST : public ExprAST
  {
    std::unique_ptr<ExprAST> FileHandle;
    std::unique_ptr<ExprAST> Content;

  public:
    FileWriteAST(std::unique_ptr<ExprAST> FileHandle, std::unique_ptr<ExprAST> Content)
        : FileHandle(std::move(FileHandle)), Content(std::move(Content)) {}

    Value *codegen() override;
  };

  /// FileAppendAST - Represents appending to a file.
  class FileAppendAST : public ExprAST
  {
    std::unique_ptr<ExprAST> FileHandle;
    std::unique_ptr<ExprAST> Content;

  public:
    FileAppendAST(std::unique_ptr<ExprAST> FileHandle, std::unique_ptr<ExprAST> Content)
        : FileHandle(std::move(FileHandle)), Content(std::move(Content)) {}

    Value *codegen() override;
  };

  /// FileCreateAST - Represents creating a new file.
  class FileCreateAST : public ExprAST
  {
    std::unique_ptr<ExprAST> FilePath;

  public:
    FileCreateAST(std::unique_ptr<ExprAST> FilePath)
        : FilePath(std::move(FilePath)) {}

    Value *codegen() override;
  };

  /// FileDeleteAST - Represents deleting a file.
  class FileDeleteAST : public ExprAST
  {
    std::unique_ptr<ExprAST> FilePath;

  public:
    FileDeleteAST(std::unique_ptr<ExprAST> FilePath)
        : FilePath(std::move(FilePath)) {}

    Value *codegen() override;
  };

  /// PrototypeAST - This class represents the "prototype" for a function,
  /// which captures its name, and its argument names (thus implicitly the number
  /// of arguments the function takes), as well as if it is an operator.
  class PrototypeAST
  {
    std::string Name;
    std::vector<std::string> Args;
    bool IsOperator;
    unsigned Precedence; // Precedence if a binary op.

  public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args,
                 bool IsOperator = false, unsigned Prec = 0)
        : Name(Name), Args(std::move(Args)), IsOperator(IsOperator),
          Precedence(Prec) {}

    Function *codegen();
    const std::string &getName() const { return Name; }

    bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
    bool isBinaryOp() const { return IsOperator && Args.size() == 2; }

    char getOperatorName() const
    {
      assert(isUnaryOp() || isBinaryOp());
      return Name[Name.size() - 1];
    }

    unsigned getBinaryPrecedence() const { return Precedence; }
  };

  /// FunctionAST - This class represents a function definition itself.
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::vector<std::unique_ptr<ExprAST>> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::vector<std::unique_ptr<ExprAST>> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}

    Function *codegen();  // ✅ Declare codegen() method
};
class ClassAST {
  std::string Name;
  std::string Parent;
  std::vector<std::unique_ptr<PrototypeAST>> Methods;
  std::vector<std::string> Fields;

public:
  ClassAST(std::string name, std::string parent, std::vector<std::string> fields,
           std::vector<std::unique_ptr<PrototypeAST>> methods)
      : Name(std::move(name)), Parent(std::move(parent)), Fields(std::move(fields)), Methods(std::move(methods)) {}

  Value *codegen();

  // Getter function for Fields
  const std::vector<std::string> &getFields() const { return Fields; }
};

/// AST for object instantiation (e.g., `new Dog()`)
class NewExprAST : public ExprAST {
  std::string ClassName;

public:
  NewExprAST(const std::string &className) : ClassName(className) {}
  Value *codegen() override;
};

/// AST for member access (e.g., `dog.bark()`)
class MemberExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Object;
  std::string Member;
  std::string ClassName;  // Store the class name

public:
  MemberExprAST(std::unique_ptr<ExprAST> object, std::string member, std::string className)
      : Object(std::move(object)), Member(std::move(member)), ClassName(std::move(className)) {}

  Value *codegen() override;
};
std::map<std::string, ClassAST> ClassTable;



#endif 