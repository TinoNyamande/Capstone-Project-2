#include "lexer.h"
#include <string>
#include <fstream>
#include <cctype>

std::string IdentifierStr; // Filled in if tok_identifier
double NumVal;  
std::ifstream InputFile;           // Filled in if tok_number

int gettok()
{
  static int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = InputFile.get();

  // Handle string literals.
  if (LastChar == '"')
  {
    std::string Str;
    while ((LastChar = InputFile.get()) != '"' && LastChar != EOF)
      Str += LastChar;

    if (LastChar == EOF)
    {
      return tok_eof;
    }

    LastChar = InputFile.get(); // Eat closing quote.
    IdentifierStr = Str;
    return tok_string;
  }

  // Handle identifiers and keywords.
  if (isalpha(LastChar))
  {
    IdentifierStr = LastChar;
    while (isalnum((LastChar = InputFile.get())))
      IdentifierStr += LastChar;

    // Check for keywords.
    if (IdentifierStr == "def")
      return tok_def;
    if (IdentifierStr == "extern")
      return tok_extern;
    if (IdentifierStr == "if")
      return tok_if;
    if (IdentifierStr == "then")
      return tok_then;
    if (IdentifierStr == "else")
      return tok_else;
    if (IdentifierStr == "for")
      return tok_for;
    if (IdentifierStr == "in")
      return tok_in;
    if (IdentifierStr == "binary")
      return tok_binary;
    if (IdentifierStr == "unary")
      return tok_unary;
    if (IdentifierStr == "var")
      return tok_var;
    if (IdentifierStr == "return")
      return tok_return;
    if (IdentifierStr == "open")
      return tok_open;
    if (IdentifierStr == "write")
      return tok_write;
    if (IdentifierStr == "read")
      return tok_read;
    if (IdentifierStr == "delete")
      return tok_delete;
    if (IdentifierStr == "while")
    return tok_while;
if (IdentifierStr == "do")
    return tok_do;


    return tok_identifier;
  }

  // Handle numbers.
  if (isdigit(LastChar) || LastChar == '.')
  {
    std::string NumStr;
    do
    {
      NumStr += LastChar;
      LastChar = InputFile.get();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), nullptr);
    return tok_number;
  }

  // Handle comments.
  if (LastChar == '#')
  {
    do
      LastChar = InputFile.get();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  // Handle end of file.
  if (LastChar == EOF)
  {
    return tok_eof;
  }

  // Handle unknown characters.
  int ThisChar = LastChar;
  LastChar = InputFile.get();
  return ThisChar;
}