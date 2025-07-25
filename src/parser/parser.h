// Parser.h
#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "../ast/ast.h"
#include <memory>
#include <map>

// Parsing state.
extern int CurTok;

int getNextToken();


// Operator precedence map.
extern std::map<char, int> BinopPrecedence;
int GetTokPrecedence();
class ExprAST;

// Error handling helper functions.
std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);
std::unique_ptr<FunctionAST> LogErrorF(const char *Str);
std::unique_ptr<ClassAST> LogErrorC(const char *Str);

// Parser functions.
std::unique_ptr<ExprAST> ParseExpression();
std::unique_ptr<ExprAST> ParseNumberExpr();
std::unique_ptr<ExprAST> ParseWhileExpr();
std::unique_ptr<ExprAST> ParseParenExpr();
std::unique_ptr<ExprAST> ParseIdentifierExpr();
std::unique_ptr<ExprAST> ParseReturnExpr();
std::unique_ptr<ExprAST>  ParseVarExpr();
std::unique_ptr<ExprAST>  ParseGlobalVarExpr();
std::unique_ptr<ExprAST> ParseUnary();
std::unique_ptr<ExprAST> ParsePrimary();
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);
std::unique_ptr<ExprAST> ParseIfExpr();
std::unique_ptr<ExprAST> ParseForExpr();
std::unique_ptr<ClassAST> ParseClass();
std::unique_ptr<PrototypeAST> ParsePrototype();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<PrototypeAST> ParseExtern();
std::unique_ptr<FunctionAST> ParseTopLevelExpr();
std::unique_ptr<ExprAST> ParseMemberAccess(std::unique_ptr<ExprAST> Object);
std::unique_ptr<ExprAST> ParseNewExpr() ;


#endif // PARSER_H