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
    if (IdentifierStr == "basa")
      return tok_def;
    if (IdentifierStr == "extern")
      return tok_extern;
    if (IdentifierStr == "kana")
      return tok_if;
    if (IdentifierStr == "then")
      return tok_then;
    if (IdentifierStr == "kana_kuti")
      return tok_else;
    if (IdentifierStr == "pakati")
      return tok_for;
    if (IdentifierStr == "mu")
      return tok_in;
    if (IdentifierStr == "binary")
      return tok_binary;
    if (IdentifierStr == "unary")
      return tok_unary;
    if (IdentifierStr == "zita")
      return tok_var;
    if (IdentifierStr == "dzosa")
      return tok_return;
    if (IdentifierStr == "vhura")
      return tok_open;
    // if (IdentifierStr == "nyora")
    //   return tok_write;
    if (IdentifierStr == "verenga")
      return tok_read;
    if (IdentifierStr == "bvisa")
      return tok_delete;
    if (IdentifierStr == "kusvika")
    return tok_while;
if (IdentifierStr == "ita")
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