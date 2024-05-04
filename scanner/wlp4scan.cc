#include <cctype>  // For isalpha and isdigit
#include <iostream>
#include <sstream>  // For std::istringstream
#include <stdexcept>

bool isValidIDCharacter(char c) {
    // Check if c is an alphabet letter (a-z or A-Z) or a digit (0-9)
    return isalpha(c) || isdigit(c);
}

int main() {
    std::istream& in = std::cin;
    std::string line;
    try {
        while (std::getline(in, line)) {
            bool inComment = false;

            std::istringstream lineStream(line);  // Treat the line as a stream
            std::string s;
            while (lineStream >> s) {
                for (int i = 0; i < s.length(); i++) {
                    char c = s[i];

                    // Check if comment
                    if (c == '/' && i + 1 < s.length() && s[i + 1] == '/') {
                        inComment = true;
                        break;
                    }

                    // Single Length Tokens
                    if (c == '(') {
                        std::cout << "LPAREN (" << std::endl;
                        continue;
                    } else if (c == ')') {
                        std::cout << "RPAREN )" << std::endl;
                        continue;
                    } else if (c == '{') {
                        std::cout << "LBRACE {" << std::endl;
                        continue;
                    } else if (c == '}') {
                        std::cout << "RBRACE }" << std::endl;
                        continue;
                    } else if (c == '+') {
                        std::cout << "PLUS +" << std::endl;
                        continue;
                    } else if (c == '-') {
                        std::cout << "MINUS -" << std::endl;
                        continue;
                    } else if (c == '*') {
                        std::cout << "STAR *" << std::endl;
                        continue;
                    } else if (c == '/') {
                        std::cout << "SLASH /" << std::endl;
                        continue;
                    } else if (c == '%') {
                        std::cout << "PCT %" << std::endl;
                        continue;
                    } else if (c == ',') {
                        std::cout << "COMMA ," << std::endl;
                        continue;
                    } else if (c == ';') {
                        std::cout << "SEMI ;" << std::endl;
                        continue;
                    } else if (c == '[') {
                        std::cout << "LBRACK [" << std::endl;
                        continue;
                    } else if (c == ']') {
                        std::cout << "RBRACK ]" << std::endl;
                        continue;
                    } else if (c == '&') {
                        std::cout << "AMP &" << std::endl;
                        continue;
                    }
                    // Single/Double Length Tokens
                    else if (c == '=') {
                        if (i != s.length() - 1) {  // not the last char
                            if (s[i + 1] == '=') {  // next char is =
                                std::cout << "EQ ==" << std::endl;
                                i += 1;
                                continue;
                            } else {
                                std::cout << "BECOMES =" << std::endl;
                                continue;
                            }
                        } else {  // is last char of the string
                            std::cout << "BECOMES =" << std::endl;
                            continue;
                        }
                    } else if (c == '!') {
                        if (i != s.length() - 1) {  // not the last char
                            if (s[i + 1] == '=') {  // next char is =
                                std::cout << "NE !=" << std::endl;
                                i += 1;
                                continue;
                            } else {
                                throw std::runtime_error("'!' without '='");
                            }
                        } else {  // is last char of the string
                            throw std::runtime_error("'!' without '='");
                        }
                    } else if (c == '<') {
                        if (i != s.length() - 1) {  // not the last char
                            if (s[i + 1] == '=') {  // next char is =
                                std::cout << "LE <=" << std::endl;
                                i += 1;
                                continue;
                            } else {
                                std::cout << "LT <" << std::endl;
                                continue;
                            }
                        } else {  // is last char of the string
                            std::cout << "LT <" << std::endl;
                            continue;
                        }
                    } else if (c == '>') {
                        if (i != s.length() - 1) {  // not the last char
                            if (s[i + 1] == '=') {  // next char is =
                                std::cout << "GE >=" << std::endl;
                                i += 1;
                                continue;
                            } else {
                                std::cout << "GT >" << std::endl;
                                continue;
                            }
                        } else {  // is last char of the string
                            std::cout << "GT >" << std::endl;
                            continue;
                        }
                    }
                    // Reserved keywords
                    std::string keywords[] = {"return", "if", "else", "while", "println", "wain", "int", "new", "delete", "NULL"};
                    std::string tokenNames[] = {"RETURN", "IF", "ELSE", "WHILE", "PRINTLN", "WAIN", "INT", "NEW", "DELETE", "NULL"};

                    bool do_continue = false;  // flag because we can't continue from inside the forloop

                    for (int j = 0; j < sizeof(keywords) / sizeof(keywords[0]); j++) {
                        std::string keyword = keywords[j];
                        std::string tokenName = tokenNames[j];

                        if (i + keyword.length() - 1 < s.length()) {  // check if keyword length is possible
                            if (s.substr(i, i + keyword.length()) == keyword) {
                                if (i + keyword.length() < s.length()) {  // any more chars after keyword?
                                    if (!isValidIDCharacter(s[i + keyword.length()])) {  // can the extra chars make this an ID?
                                        std::cout << tokenName << " " << keyword << std::endl;
                                        i += keyword.length() - 1;
                                        do_continue = true;
                                        break;
                                    }
                                } else {  // no more chars after keyword
                                    std::cout << tokenName << " " << keyword << std::endl;
                                    i += keyword.length() - 1;
                                    do_continue = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (do_continue) {
                        continue;
                    }

                    // Check if NUM
                    int index = i;
                    std::string currString = "";
                    // build the number
                    while (index < s.length() && isdigit(s[index])) {
                        currString += s[index];
                        index++;
                    }
                    // check the number
                    if (currString.length() == 1) {  // 1 digit number
                        std::cout << "NUM " << currString << std::endl;
                        continue;
                    } else if (currString.length() > 10) {  // digit out of range
                        throw std::runtime_error("num out of range: " + currString);
                    } else if (currString.length() >= 2) {  // >= 2 digit number
                        // check first digit not 0
                        if (currString[0] == '0') {
                            throw std::runtime_error("num starting with a '0' detected");
                        }
                        // Check range
                        long long num = std::stoll(currString);
                        if (num <= 2147483647) {
                            std::cout << "NUM " << currString << std::endl;
                            i += currString.length() - 1;
                            continue;
                        } else {
                            throw std::runtime_error("num out of range: " + currString);
                        }
                    }

                    // Check if ID
                    std::string currID;
                    index = i;
                    if (isalpha(c)) {  // first char must be a character
                        currID += c;
                        index++;
                        while (index < s.length() && isValidIDCharacter(s[index])) {
                            currID += s[index];
                            index++;
                        }
                    }
                    if (currID != "") {
                        std::cout << "ID " << currID << std::endl;
                        i += currID.length() - 1;
                        continue;
                    }


                    // Not a recognized character
                    throw std::runtime_error("unrecognized character: " + c);
                }

                if (inComment) {
                    break;
                }
            }
        }
    } catch (const std::runtime_error& e) {
        // Catch the runtime_error
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;  // Return a non-zero value to indicate an error
    }
}
