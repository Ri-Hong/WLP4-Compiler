#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <utility>
#include <set>
#include <array>
#include "scanner.h"

/*
 * C++ Starter code for CS241 A3
 *
 * This file contains helpers for asm.cc and you don't need to modify it.
 * Read the scanner.h file for a description of the helper functions.
 *
 * This code may be helpful to understand when you write a scanner in a 
 * later assignment. However, you do not need to understand how it works 
 * to write the assembler.
 */

Token::Token(Token::Kind kind, std::string lexeme):
  kind(kind), lexeme(std::move(lexeme)) {}

  Token:: Kind Token::getKind() const { return kind; }
const std::string &Token::getLexeme() const { return lexeme; }

std::ostream &operator<<(std::ostream &out, const Token &tok) {
  out << "Token(";
  switch (tok.getKind()) {
    case Token::ID:         out << "ID";         break;
    case Token::LABEL:      out << "LABEL";      break;
    case Token::WORD:       out << "WORD";       break;
    case Token::COMMA:      out << "COMMA";      break;
    case Token::LPAREN:     out << "LPAREN";     break;
    case Token::RPAREN:     out << "RPAREN";     break;
    case Token::INT:        out << "INT";        break;
    case Token::HEXINT:     out << "HEXINT";     break;
    case Token::REG:        out << "REG";        break;
    case Token::WHITESPACE: out << "WHITESPACE"; break;
    case Token::COMMENT:    out << "COMMENT";    break;
  }
  out << ", " << tok.getLexeme() << ")";

  return out;
}

int64_t Token::toNumber() const {
  std::istringstream iss;
  int64_t result;

  if (kind == INT) {
    iss.str(lexeme);
  } else if (kind == HEXINT) {
    iss.str(lexeme.substr(2));
    iss >> std::hex;
  } else if (kind == REG) {
    iss.str(lexeme.substr(1));
  } else {
    // This should never happen if the user calls this function correctly
    return 0;
  }

  iss >> result;
  return result;
}

ScanningFailure::ScanningFailure(std::string message):
  message(std::move(message)) {}

const std::string &ScanningFailure::what() const { return message; }

/* Representation of a DFA, used to handle the scanning process.
 */
class AsmDFA {
  public:
    enum State {
      // States that are also kinds
      ID = 0,
      LABEL,
      COMMA,
      LPAREN,
      RPAREN,
      INT,
      HEXINT,
      REG,
      WHITESPACE,
      COMMENT,

      // States that are not also kinds
      FAIL,
      START,
      DOT,
      DOTID,
      ZERO,
      ZEROX,
      MINUS,
      DOLLARS,

      // Hack to let this be used easily in arrays. This should always be the
      // final element in the enum, and should always point to the previous
      // element.

      LARGEST_STATE = DOLLARS
    };

  private:
    /* A set of all accepting states for the DFA.
     * Currently non-accepting states are not actually present anywhere
     * in memory, but a list can be found in the constructor.
     */
    std::set<State> acceptingStates;

    /*
     * The transition function for the DFA, stored as a map.
     */

    std::array<std::array<State, 128>, LARGEST_STATE + 1> transitionFunction;

    /*
     * Converts a state to a kind to allow construction of Tokens from States.
     * Throws an exception if conversion is not possible.
     */
    Token::Kind stateToKind(State s) const {
      switch(s) {
        case ID:         return Token::ID;
        case LABEL:      return Token::LABEL;
        case DOTID:      return Token::WORD;
        case COMMA:      return Token::COMMA;
        case LPAREN:     return Token::LPAREN;
        case RPAREN:     return Token::RPAREN;
        case INT:        return Token::INT;
        case ZERO:       return Token::INT;
        case HEXINT:     return Token::HEXINT;
        case REG:        return Token::REG;
        case WHITESPACE: return Token::WHITESPACE;
        case COMMENT:    return Token::COMMENT;
        default: throw ScanningFailure("ERROR: Cannot convert state to kind.");
      }
    }


  public:
    /* Tokenizes an input string according to the Simplified Maximal Munch
     * scanning algorithm.
     */
    std::vector<Token> simplifiedMaximalMunch(const std::string &input) const {
      std::vector<Token> result;

      State state = start();
      std::string munchedInput;

      // We can't use a range-based for loop effectively here
      // since the iterator doesn't always increment.
      for (std::string::const_iterator inputPosn = input.begin();
           inputPosn != input.end();) {

        State oldState = state;
        state = transition(state, *inputPosn);

        if (!failed(state)) {
          munchedInput += *inputPosn;
          oldState = state;

          ++inputPosn;
        }

        if (inputPosn == input.end() || failed(state)) {
          if (accept(oldState)) {
            result.push_back(Token(stateToKind(oldState), munchedInput));

            munchedInput = "";
            state = start();
          } else {
            if (failed(state)) {
              munchedInput += *inputPosn;
            }
            throw ScanningFailure("ERROR: Simplified maximal munch failed on input: "
                                 + munchedInput);
          }
        }
      }

      return result;
    }

    /* Initializes the accepting states for the DFA.
     */
    AsmDFA() {
      acceptingStates = {ID, LABEL, DOTID, HEXINT,
                           INT, ZERO, COMMA, REG,
                           LPAREN, RPAREN, WHITESPACE, COMMENT};
      //Non-accepting states are DOT, MINUS, ZEROX, DOLLARS, START

      // Initialize transitions for the DFA
      for (size_t i = 0; i < transitionFunction.size(); ++i) {
        for (size_t j = 0; j < transitionFunction[0].size(); ++j) {
          transitionFunction[i][j] = FAIL;
        }
      }

      registerTransition(START, isalpha, ID);
      registerTransition(START, ".", DOT);
      registerTransition(START, "0", ZERO);
      registerTransition(START, "123456789", INT);
      registerTransition(START, "-", MINUS);
      registerTransition(START, ";", COMMENT);
      registerTransition(START, isspace, WHITESPACE);
      registerTransition(START, "$", DOLLARS);
      registerTransition(START, ",", COMMA);
      registerTransition(START, "(", LPAREN);
      registerTransition(START, ")", RPAREN);
      registerTransition(ID, isalnum, ID);
      registerTransition(ID, ":", LABEL);
      registerTransition(DOT, isalpha, DOTID);
      registerTransition(DOTID, isalpha, DOTID);
      registerTransition(ZERO, "x", ZEROX);
      registerTransition(ZERO, isdigit, INT);
      registerTransition(ZEROX, isxdigit, HEXINT);
      registerTransition(HEXINT, isxdigit, HEXINT);
      registerTransition(MINUS, isdigit, INT);
      registerTransition(INT, isdigit, INT);
      registerTransition(COMMENT, [](int c) -> int { return c != '\n'; },
          COMMENT);
      registerTransition(WHITESPACE, isspace, WHITESPACE);
      registerTransition(DOLLARS, isdigit, REG);
      registerTransition(REG, isdigit, REG);
    }

    // Register a transition on all chars in chars
    void registerTransition(State oldState, const std::string &chars,
        State newState) {
      for (char c : chars) {
        transitionFunction[oldState][c] = newState;
      }
    }

    // Register a transition on all chars matching test
    // For some reason the cctype functions all use ints, hence the function
    // argument type.
    void registerTransition(State oldState, int (*test)(int), State newState) {

      for (int c = 0; c < 128; ++c) {
        if (test(c)) {
          transitionFunction[oldState][c] = newState;
        }
      }
    }

    /* Returns the state corresponding to following a transition
     * from the given starting state on the given character,
     * or a special fail state if the transition does not exist.
     */
    State transition(State state, char nextChar) const {
      return transitionFunction[state][nextChar];
    }

    /* Checks whether the state returned by transition
     * corresponds to failure to transition.
     */
    bool failed(State state) const { return state == FAIL; }

    /* Checks whether the state returned by transition
     * is an accepting state.
     */
    bool accept(State state) const {
      return acceptingStates.count(state) > 0;
    }

    /* Returns the starting state of the DFA
     */
    State start() const { return START; }
};

std::vector<Token> scan(const std::string &input) {
  static AsmDFA theDFA;

  std::vector<Token> tokens = theDFA.simplifiedMaximalMunch(input);

  // We need to:
  // * Throw exceptions for WORD tokens whose lexemes aren't ".word".
  // * Remove WHITESPACE and COMMENT tokens entirely.

  std::vector<Token> newTokens;

  for (auto &token : tokens) {
    if (token.getKind() == Token::WORD) {
      if (token.getLexeme() == ".word") {
        newTokens.push_back(token);
      } else {
        throw ScanningFailure("ERROR: DOTID token unrecognized: " +
            token.getLexeme());
      }
    } else if (token.getKind() != Token::WHITESPACE
        && token.getKind() != Token::Kind::COMMENT) {
      newTokens.push_back(token);
    }
  }

  return newTokens;
}
