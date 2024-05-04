#ifndef CS241_SCANNER_H
#define CS241_SCANNER_H
#include <string>
#include <vector>
#include <set>
#include <cstdint>
#include <ostream>

/*
 * C++ Starter code for CS241 A3
 *
 * This file contains helpers for asm.cc and should not need to be modified by
 * you. However, its comments contain important information and you should
 * read the entire file before you start modifying asm.cc.
 */

class Token;

/* Scans a single line of input and produces a list of tokens.
 *
 * Scan returns tokens with the following kinds:
 * ID: identifiers and keywords.
 * LABEL: labels (identifiers ending in a colon).
 * WORD: the special ".word" keyword.
 * COMMA: a comma.
 * LPAREN: a left parenthesis.
 * RPAREN: a right parenthesis.
 * INT: a signed or unsigned 32-bit integer written in decimal.
 * HEXINT: an unsigned 32-bit integer written in hexadecimal.
 * REG: a register between $0 and $31.
 */

std::vector<Token> scan(const std::string &input);

/* A scanned token produced by the scanner.
 * The "kind" tells us what kind of token it is
 * while the "lexeme" tells us exactly what text
 * the programmer typed. For example, the token
 * "abc" might have kind "ID" and lexeme "abc".
 *
 * While you can create tokens with any kind and
 * lexeme, the list of kinds produced by the
 * starter code can be found in the documentation
 * for scan above.
 */
class Token {
  public:
    enum Kind {
      ID = 0,
      LABEL,
      WORD,
      COMMA,
      LPAREN,
      RPAREN,
      INT,
      HEXINT,
      REG,
      WHITESPACE,
      COMMENT
    };

  private:
    Kind kind;
    std::string lexeme;

  public:
    Token(Kind kind, std::string lexeme);

    Kind getKind() const;
    const std::string &getLexeme() const;

    /* Converts a token to the corresponding number.
     * Only works on tokens of type INT, HEXINT, or REG.
     */
    int64_t toNumber() const;
    /* Further notes about the toNumber function:
     *
     * This function returns a 64-bit integer instead of 32-bit because
     * .word accepts both signed and unsigned 32-bit integers. The union 
     * of the signed and unsigned ranges cannot be represented by a single 
     * 32-bit type. Even when not working with .word, you should store the
     * result of this function in an int64_t variable, or you might get
     * unexpected behaviour due to C++'s automatic type conversions.
     *
     * If the number represented by the token does not fit in the signed 
     * 64-bit range, there are two possibilities:
     * - If the token is a positive decimal integer or a hexadecimal 
     *   integer, then 2^{63}-1 (the maximum int64_t value) is returned.
     * - If the token is a negative decimal integer, then -2^{63} (the
     *   minimum int64_t value) is returned.
     *
     * Hexadecimal integers are always interpreted as unsigned. For 
     * example, the integer 0xffffffffffffffff (the 64-bit two's 
     * complement representation of -1) will actually be interpreted as 
     * 2^{64}-1, which is out of the signed 64-bit range. Thus, by the above 
     * rule, the value 2^{63}-1 is returned. This behaviour may seem unusual,
     * but is actually important for detecting out of range hex integers.
     * For example, consider: beq $0, $0, 0xffffffffffffffff
     * If the hex integer here was interpreted as -1, that would be bad!
     */

};

/* Prints a string representation of a token.
 * Mainly useful for debugging.
 */
std::ostream &operator<<(std::ostream &out, const Token &tok);

/* An exception class thrown when an error is encountered while scanning.
 */
class ScanningFailure {
    std::string message;

  public:
    ScanningFailure(std::string message);

    // Returns the message associated with the exception.
    const std::string &what() const;
};

#endif
