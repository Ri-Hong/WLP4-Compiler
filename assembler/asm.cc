#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream> // For std::ostringstream

#include "scanner.h"

std::string kindToString(Token::Kind kind)
{
  switch (kind)
  {
  case Token::ID:
    return "ID";
  case Token::LABEL:
    return "LABEL";
  case Token::WORD:
    return "WORD";
  case Token::COMMA:
    return "COMMA";
  case Token::LPAREN:
    return "LPAREN";
  case Token::RPAREN:
    return "RPAREN";
  case Token::INT:
    return "INT";
  case Token::HEXINT:
    return "HEXINT";
  case Token::REG:
    return "REG";
  case Token::WHITESPACE:
    return "WHITESPACE";
  case Token::COMMENT:
    return "COMMENT";
  // Add other cases as necessary
  default:
    return "UNKNOWN";
  }
}

void output_uint_to_bin(int64_t a)
{
  char c;
  c = (a >> 24) & 0xff;
  std::cout << c;
  c = (a >> 16) & 0xff;
  std::cout << c;
  c = (a >> 8) & 0xff;
  std::cout << c;
  c = (a >> 0) & 0xff; // or c = a & 0xff
  std::cout << c;
}

// Throws if out of range
void check_reg_range(int reg)
{
  if (reg < 0 || reg > 31)
  {
    throw std::out_of_range("ERROR: Register out of range! $" + std::to_string(reg));
  }
}

// Throws if out of range
void check_itmd_range_int(int itmd)
{
  if (itmd < -32768 || itmd > 32767)
  {
    throw std::out_of_range("ERROR: Intermediate out of range!" + std::to_string(itmd));
  }
}

// Throws if out of range
void check_itmd_range_hex(int itmd)
{
  if (itmd < 0 || itmd > 65535)
  {
    throw std::out_of_range("ERROR: Intermediate out of range!" + std::to_string(itmd));
  }
}

// Throws if Token is not of right kind
void check_token_ok(Token::Kind expected, Token actual)
{
  if (actual.getKind() != expected)
  {
    std::ostringstream message;
    message << "ERROR: Expected ";

    message << kindToString(expected) << ". Got " << kindToString(actual.getKind()) << " instead.";

    throw std::runtime_error(message.str());
  }
}

// Returns true (1) if ok
// Returns false if not ok
void check_token_ok(const std::vector<Token::Kind> possibleExpected, Token actual)
{
  for (Token::Kind expected : possibleExpected)
  {
    if (actual.getKind() == expected)
    {
      return;
    }
  }
  // If we reach here, the actual token's kind was not in possibleExpected
  std::ostringstream msg;
  msg << "ERROR: Expected one of [";
  for (size_t i = 0; i < possibleExpected.size(); ++i)
  {
    msg << kindToString(possibleExpected[i]);
    if (i < possibleExpected.size() - 1)
    {
      msg << ", "; // Add comma between kinds except for the last one
    }
  }
  msg << "]. Got " << kindToString(actual.getKind()) << " instead.";
  throw std::runtime_error(msg.str());
}

// Checks if next token exists
void check_next_token_exists(int i, int n_tokens)
{
  if (i + 1 >= n_tokens)
  {
    throw std::runtime_error("ERROR: Not enough tokens");
  }
}

/*
 * C++ Starter code for CS241 A3
 *
 * This file contains the main function of your program. By default, it just
 * prints the scanned list of tokens back to standard output.
 */
int main()
{
  std::string line;
  std::unordered_map<std::string, int> symbolTable;

  try
  {
    // Store the Tokens into an istringstream so we can read them twice
    std::string input;
    std::string line;
    while (getline(std::cin, line))
    {
      input += line + "\n"; // Preserve line breaks
    }

    // First pass: Process each line to construct the symbol table
    // Keep track of number of non-null-lines
    // Construct symbol table
    // Check for duplicate label definitions
    std::istringstream stream1(input);
    int non_null_lines = 0;
    while (getline(stream1, line))
    {
      std::vector<Token> tokenLine = scan(line);
      for (long unsigned int i = 0; i < tokenLine.size(); i++)
      {

        if (tokenLine.at(i).getKind() == Token::ID || tokenLine.at(i).getKind() == Token::WORD)
        {
          non_null_lines++;
          break;
        }
        else if (tokenLine.at(i).getKind() == Token::LABEL)
        {
          std::string label = tokenLine.at(i).getLexeme().substr(0, tokenLine.at(i).getLexeme().size() - 1); // Remove the colon before the search
          auto it = symbolTable.find(label);
          if (it != symbolTable.end())
          {
            // Found duplicate label
            throw std::runtime_error("ERROR: Duplicate label found: " + label);
          }
          else
          {
            // Key doesn't exist yet
            symbolTable[tokenLine.at(i).getLexeme().substr(0, tokenLine.at(i).getLexeme().size() - 1)] = (non_null_lines) * 4;
          }
        }
      }
    }

    // Printing the symbol table
    // for (const auto &pair : symbolTable)
    // {
    //   std::cout << "Label: " << pair.first << ", Line Number: " << pair.second << std::endl;
    // }

    // Second pass: using another istringstream
    std::istringstream stream2(input);
    non_null_lines = 0;
    while (getline(stream2, line))
    {
      // This example code just prints the scanned tokens on each line.
      std::vector<Token> tokenLine = scan(line);

      // This code is just an example - you don't have to use a range-based
      // for loop in your actual assembler. You might find it easier to
      // use an index-based loop like this:
      // for(int i=0; i<tokenLine.size(); i++) { ... }
      // for (auto &token : tokenLine)
      // {
      //   std::cout << token << ' ';
      // }

      for (long unsigned int i = 0; i < tokenLine.size(); i++)
      {

        // Handle non_null_lines counter
        // If token is an ID and it's not a label (I.e. not in symbol table)
        // WORD also works
        auto it = symbolTable.find(tokenLine.at(i).getLexeme());
        if (tokenLine.at(i).getKind() == Token::WORD || (tokenLine.at(i).getKind() == Token::ID && it == symbolTable.end()))
        {
          non_null_lines++;
        }
        // Used to check if an unrecognized token is a label
        std::string string_with_colon = tokenLine.at(i).getLexeme().substr(0, tokenLine.at(i).getLexeme().size() - 1);
        it = symbolTable.find(string_with_colon);

        // .word
        if (tokenLine.at(i).getLexeme() == ".word")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token token = tokenLine.at(++i);
          check_token_ok({Token::INT, Token::HEXINT, Token::ID}, token);

          if (token.getKind() == Token::ID)
          {
            // Check if label is not in symbol table
            auto it = symbolTable.find(token.getLexeme());
            if (it == symbolTable.end())
            {
              // Label doesn't exist
              throw std::runtime_error("ERROR: Label used without declaration: " + token.getLexeme());
            }

            // Check if label offset is in decimal range
            check_itmd_range_int(token.toNumber());
            // Print the label offset
            output_uint_to_bin(symbolTable[token.getLexeme()]);
          }
          else
          { // token is an int or hex int
            int64_t int_rep = token.toNumber();
            if (int_rep < 0 || int_rep > 4294967295)
            {
              throw std::out_of_range("ERROR: Value out of range: " + token.getLexeme());
            }
            output_uint_to_bin(int_rep);
          }
        }
        // add
        else if (tokenLine.at(i).getLexeme() == "add")
        {
          // add $1, $2, $3 will look like:
          // Token(ID, add) Token(REG, $1) Token(COMMA, ,) Token(REG, $2) Token(COMMA, ,) Token(REG, $3)
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token d = tokenLine.at(++i);
          if (d.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << d << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }

          // Check register ranges
          int64_t int_d = d.toNumber();
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);
          check_reg_range(int_d);

          // 0000 00ss ssst tttt dddd d000 0010 0000
          unsigned int result = ((int_s << 21) | (int_t << 16) | (int_d << 11) | 0b00000100000);

          output_uint_to_bin(result);
        }
        // sub
        else if (tokenLine.at(i).getLexeme() == "sub")
        {
          // sub $1, $2, $3 will look like:
          // Token(ID, sub) Token(REG, $1) Token(COMMA, ,) Token(REG, $2) Token(COMMA, ,) Token(REG, $3)
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token d = tokenLine.at(++i);
          if (d.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << d << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }

          // Check register ranges
          int64_t int_d = d.toNumber();
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);
          check_reg_range(int_d);

          unsigned int result = ((int_s << 21) | (int_t << 16) | (int_d << 11) | 0b00000100010);

          output_uint_to_bin(result);
        }
        // mult
        else if (tokenLine.at(i).getLexeme() == "mult")
        {
          // mult $1, $2 will look like:
          // Token(ID, mult) Token(REG, $1) Token(COMMA, ,) Token(REG, $2)
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }

          // Check register ranges
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);

          unsigned int result = ((int_s << 21) | (int_t << 16) | 0b0000000000011000);

          output_uint_to_bin(result);
        }
        // multu
        else if (tokenLine.at(i).getLexeme() == "multu")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }

          // Check register ranges
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);

          unsigned int result = ((int_s << 21) | (int_t << 16) | 0b0000000000011001);

          output_uint_to_bin(result);
        }
        // div
        else if (tokenLine.at(i).getLexeme() == "div")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }

          // Check register ranges
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);

          unsigned int result = ((int_s << 21) | (int_t << 16) | 0b0000000000011010);
          output_uint_to_bin(result);
        }
        // divu
        else if (tokenLine.at(i).getLexeme() == "divu")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }

          // Check register ranges
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);

          unsigned int result = ((int_s << 21) | (int_t << 16) | 0b0000000000011011);

          output_uint_to_bin(result);
        }
        // mfhi
        else if (tokenLine.at(i).getLexeme() == "mfhi")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token d = tokenLine.at(++i);
          if (d.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << d << std::endl;
            return 1;
          }
          // Check register ranges
          int64_t int_d = d.toNumber();

          check_reg_range(int_d);

          unsigned int result = ((int_d << 11) | 0b00000010000);

          output_uint_to_bin(result);
        }
        // mflo
        else if (tokenLine.at(i).getLexeme() == "mflo")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token d = tokenLine.at(++i);
          if (d.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << d << std::endl;
            return 1;
          }
          // Check register ranges
          int64_t int_d = d.toNumber();

          check_reg_range(int_d);

          unsigned int result = ((int_d << 11) | 0b00000010010);

          output_uint_to_bin(result);
        }
        // lis
        else if (tokenLine.at(i).getLexeme() == "lis")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token d = tokenLine.at(++i);
          if (d.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << d << std::endl;
            return 1;
          }
          // Check register ranges
          int64_t int_d = d.toNumber();

          check_reg_range(int_d);

          unsigned int result = ((int_d << 11) | 0b00000010100);

          output_uint_to_bin(result);
        }
        // lw
        else if (tokenLine.at(i).getLexeme() == "lw")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token itmd = tokenLine.at(++i);
          check_token_ok({Token::INT, Token::HEXINT}, itmd);

          check_next_token_exists(i, tokenLine.size());
          Token left_bracket = tokenLine.at(++i);
          check_token_ok(Token::LPAREN, left_bracket);

          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }

          check_next_token_exists(i, tokenLine.size());
          Token right_bracket = tokenLine.at(++i);
          check_token_ok(Token::RPAREN, right_bracket);

          // Check register ranges
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);

          // Check immediate
          int int_itmd = itmd.toNumber();
          // Check if it's a hex itmd or int itmd
          if (itmd.getKind() == Token::INT)
          {
            check_itmd_range_int(int_itmd);
          }
          else if (itmd.getKind() == Token::HEXINT)
          {
            check_itmd_range_hex(int_itmd);
          }

          unsigned int result = ((0b100011 << 26) | (int_s << 21) | (int_t << 16) | (int_itmd & 0xffff));
          output_uint_to_bin(result);
        }
        // sw
        else if (tokenLine.at(i).getLexeme() == "sw")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token itmd = tokenLine.at(++i);
          check_token_ok({Token::INT, Token::HEXINT}, itmd);

          check_next_token_exists(i, tokenLine.size());
          Token left_bracket = tokenLine.at(++i);
          check_token_ok(Token::LPAREN, left_bracket);

          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }

          check_next_token_exists(i, tokenLine.size());
          Token right_bracket = tokenLine.at(++i);
          check_token_ok(Token::RPAREN, right_bracket);

          // Check register ranges
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);

          // Check immediate
          int int_itmd = itmd.toNumber();
          // Check if it's a hex itmd or int itmd
          if (itmd.getKind() == Token::INT)
          {
            check_itmd_range_int(int_itmd);
          }
          else if (itmd.getKind() == Token::HEXINT)
          {
            check_itmd_range_hex(int_itmd);
          }

          unsigned int result = ((0b101011 << 26) | (int_s << 21) | (int_t << 16) | (int_itmd & 0xffff));
          output_uint_to_bin(result);
        }
        // slt
        else if (tokenLine.at(i).getLexeme() == "slt")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token d = tokenLine.at(++i);
          if (d.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << d << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }

          // Check register ranges
          int64_t int_d = d.toNumber();
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);
          check_reg_range(int_d);

          unsigned int result = ((int_s << 21) | (int_t << 16) | (int_d << 11) | 0b00000101010);

          output_uint_to_bin(result);
        }
        // sltu
        else if (tokenLine.at(i).getLexeme() == "sltu")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token d = tokenLine.at(++i);
          if (d.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << d << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          comma = tokenLine.at(++i);
          // Check if token is a comma
          if (comma.getKind() != Token::COMMA)
          {
            std::cerr << "ERROR: Missing comma in add! " << std::endl;
            return 1;
          }
          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          if (t.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << t << std::endl;
            return 1;
          }

          // Check register ranges
          int64_t int_d = d.toNumber();
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);
          check_reg_range(int_d);

          unsigned int result = ((int_s << 21) | (int_t << 16) | (int_d << 11) | 0b00000101011);

          output_uint_to_bin(result);
        }
        // beq
        else if (tokenLine.at(i).getLexeme() == "beq")
        {
          // beq $1, $2, 5 will look like:
          // Token(ID, beq) Token(REG, $1) Token(COMMA, ,) Token(REG, $2) Token(COMMA, ,) Token(INT, 5)
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          check_token_ok(Token::REG, s);

          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          check_token_ok(Token::COMMA, comma);

          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          check_token_ok(Token::REG, t);

          check_next_token_exists(i, tokenLine.size());
          comma = tokenLine.at(++i);
          check_token_ok(Token::COMMA, comma);

          check_next_token_exists(i, tokenLine.size());
          Token itmd = tokenLine.at(++i);
          check_token_ok({Token::INT, Token::HEXINT, Token::ID}, itmd);

          // Check register ranges
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);

          int64_t int_itmd;
          if (itmd.getKind() == Token::ID)
          { // If immediate is a label
            // Check if label is not in symbol table
            auto it = symbolTable.find(itmd.getLexeme());
            if (it == symbolTable.end())
            {
              // Undeclared label
              throw std::runtime_error("ERROR: Label used without declaration: " + itmd.getLexeme());
            }

            // Check if label offset is in decimal range
            check_itmd_range_int(itmd.toNumber());

            // std::cout << "non_null_lines: " << non_null_lines << " Label lines bfore: " << symbolTable[itmd.getLexeme()] / 4 << std::endl;
            // Load the label offset
            int_itmd = symbolTable[itmd.getLexeme()] / 4 - non_null_lines;
          }
          else
          { // If immediate is an int or hexint
            int_itmd = itmd.toNumber();
            // Check if it's a hex itmd or int itmd
            if (itmd.getKind() == Token::INT)
            {
              check_itmd_range_int(int_itmd);
            }
            else if (itmd.getKind() == Token::HEXINT)
            {
              check_itmd_range_hex(int_itmd);
            }
            // std::cout << int_d << " " << int_s << " " << int_t << std::endl;
          }
          // 0000 00ss ssst tttt dddd d000 0010 0000
          unsigned int result = ((0b000100 << 26) | (int_s << 21) | (int_t << 16) | (int_itmd & 0xffff));

          output_uint_to_bin(result);
        }
        // bne
        else if (tokenLine.at(i).getLexeme() == "bne")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          check_token_ok(Token::REG, s);

          check_next_token_exists(i, tokenLine.size());
          Token comma = tokenLine.at(++i);
          check_token_ok(Token::COMMA, comma);

          check_next_token_exists(i, tokenLine.size());
          Token t = tokenLine.at(++i);
          check_token_ok(Token::REG, t);

          check_next_token_exists(i, tokenLine.size());
          comma = tokenLine.at(++i);
          check_token_ok(Token::COMMA, comma);

          check_next_token_exists(i, tokenLine.size());
          Token itmd = tokenLine.at(++i);
          check_token_ok({Token::INT, Token::HEXINT, Token::ID}, itmd);

          // Check register ranges
          int64_t int_s = s.toNumber();
          int64_t int_t = t.toNumber();

          check_reg_range(int_s);
          check_reg_range(int_t);

          int64_t int_itmd;
          if (itmd.getKind() == Token::ID)
          { // If immediate is a label
            // Check if label is not in symbol table
            auto it = symbolTable.find(itmd.getLexeme());
            if (it == symbolTable.end())
            {
              // Undeclared label
              throw std::runtime_error("ERROR: Label used without declaration: " + itmd.getLexeme());
            }

            // Check if label offset is in decimal range
            check_itmd_range_int(itmd.toNumber());

            // std::cout << "non_null_lines: " << non_null_lines << " Label lines bfore: " << symbolTable[itmd.getLexeme()] / 4 << std::endl;
            // Load the label offset
            int_itmd = symbolTable[itmd.getLexeme()] / 4 - non_null_lines;
          }
          else
          { // If immediate is an int or hexint
            int_itmd = itmd.toNumber();
            // Check if it's a hex itmd or int itmd
            if (itmd.getKind() == Token::INT)
            {
              check_itmd_range_int(int_itmd);
            }
            else if (itmd.getKind() == Token::HEXINT)
            {
              check_itmd_range_hex(int_itmd);
            }
            // std::cout << int_d << " " << int_s << " " << int_t << std::endl;
          }
          // 0000 00ss ssst tttt dddd d000 0010 0000
          unsigned int result = ((0b000101 << 26) | (int_s << 21) | (int_t << 16) | (int_itmd & 0xffff));

          output_uint_to_bin(result);
        }
        // jr
        else if (tokenLine.at(i).getLexeme() == "jr")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          // Check register ranges
          int64_t int_s = s.toNumber();

          check_reg_range(int_s);

          unsigned int result = ((int_s << 21) | 0b1000);

          output_uint_to_bin(result);
        }
        // jalr
        else if (tokenLine.at(i).getLexeme() == "jalr")
        {
          // Get the next token and convert to binary
          check_next_token_exists(i, tokenLine.size());
          Token s = tokenLine.at(++i);
          if (s.getKind() != Token::REG)
          {
            std::cerr << "ERROR: Expected Register " << s << std::endl;
            return 1;
          }
          // Check register ranges
          int64_t int_s = s.toNumber();

          check_reg_range(int_s);

          unsigned int result = ((int_s << 21) | 0b1001);

          output_uint_to_bin(result);
        }
        else if (it != symbolTable.end())
        { // If it is in the symbolTable
          continue;
        }
        else
        { // Unrecognized token
          throw std::runtime_error("ERROR: Unrecognized Token " + tokenLine.at(i).getLexeme());
        }
      }

      // Remove this when you're done playing with the example program!
      // Printing a random newline character as part of your machine code
      // output will cause you to fail the Marmoset tests.
      // std::cout << std::endl;
    }
  }
  catch (ScanningFailure &f)
  {
    std::cerr << f.what() << std::endl;
    return 1;
  }
  catch (std::runtime_error &e)
  {
    std::cerr << "Runtime error: " << e.what() << std::endl;
    return 1;
  }
  catch (std::out_of_range &e)
  {
    std::cerr << "Out of range error: " << e.what() << std::endl;
    return 1;
  }
  catch (std::exception &e)
  {
    // This will catch any other std::exception-based errors not caught by the above handlers.
    std::cerr << "Standard exception: " << e.what() << std::endl;
    return 1;
  }
  // You can add your own catch clause(s) for other kinds of errors.
  // Throwing exceptions and catching them is the recommended way to
  // handle errors and terminate the program cleanly in C++. Do not
  // use the std::exit function, which may leak memory.

  return 0;
}
