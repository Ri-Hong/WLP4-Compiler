#include <iostream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// #include "wlp4data.h"

using namespace std;

struct Token {
    std::string kind, lexeme;

    Token() : kind(""), lexeme("") {}

    Token(const std::string& kind, const std::string& lexeme) : kind(kind), lexeme(lexeme) {}
};

struct ParseTreeNode {
    string prodRuleLHS;
    vector<string> prodRuleRHS;

    Token token;

    std::vector<ParseTreeNode*> children;

    string type;
    bool wellTyped;

    ParseTreeNode(string prodRuleLHS, vector<string> prodRuleRHS, vector<ParseTreeNode*> children) : prodRuleLHS(prodRuleLHS), prodRuleRHS(prodRuleRHS), token(), children(children), type(""), wellTyped(false) {}

    ParseTreeNode(Token t) : token(t), wellTyped(false) {}

    bool isTerminal() const {
        // If kind is not an empty string, then it's a token
        return !token.kind.empty();
    }

    // Destructor to clean up memory used by children
    ~ParseTreeNode() {
        for (auto child : children) {
            delete child;  // Delete each child
        }
    }

    // Friend function to overload operator<< for ParseTreeNode
    friend ostream& operator<<(ostream& os, const ParseTreeNode& node);
};

ostream& operator<<(ostream& os, const ParseTreeNode& node) {
    if (node.isTerminal()) {
        // Print token details if it's a token node, along with its type if available
        os << node.token.kind << " " << node.token.lexeme;
        if (!node.type.empty()) {      // Check if type information is available and not empty
            os << " : " << node.type;  // Append type information
        }
    } else {
        // Print production rule
        os << node.prodRuleLHS << " -> ";
        for (const auto& rhs : node.prodRuleRHS) {
            os << rhs << " ";
        }
        // Optionally, print type information for production rules if needed
        if (!node.type.empty()) {      // Check if type information is available and not empty
            os << " : " << node.type;  // Append type information
        }
    }
    return os;  // Return the ostream object to allow chaining
}

// GLOBAL VARIABLES

// procedure_signature -> (procedure_signature, [variable_name -> type])
// procedure_signature is a vector of types
unordered_map<string, pair<vector<string>, unordered_map<string, string>>> symbol_table;

// indentations = true => indent each level of the tree
void printParseTree(const ParseTreeNode* node, int level, bool indentations) {
    if (node == nullptr) return;

    if (indentations) {
        // Print indentation based on the level in the tree
        for (int i = 0; i < level; ++i) {
            cout << "  ";
        }
    }

    if (node->isTerminal()) {
        // Print token details if it's a token node, along with its type if available
        cout << node->token.kind << " " << node->token.lexeme;
        if (!node->type.empty()) {        // Check if type information is available and not empty
            cout << " : " << node->type;  // Append type information
        }
        cout << endl;
    } else {
        // Print production rule
        cout << node->prodRuleLHS;

        if (indentations) {
            cout << " -> ";
        } else {
            cout << " ";
        }

        for (const auto& rhs : node->prodRuleRHS) {
            cout << rhs << " ";
        }

        // Check if type information is available and not empty
        // Don't want types for arglist
        if (!node->type.empty() && node->prodRuleLHS != "arglist") {
            cout << ": " << node->type;  // Append type information
        }

        cout << endl;

        // Recursively print children, including their types if available
        for (const auto& child : node->children) {
            printParseTree(child, level + 1, indentations);
        }
    }
}

void printSymbolTable() {
    cout << "Symbol Table:" << endl;
    for (const auto& functionEntry : symbol_table) {
        cout << "Function: " << functionEntry.first;  // Function name
        cout << "[";

        for (const string& s : functionEntry.second.first) {  // Function Param Signature
            cout << s << ", ";
        }
        cout << "]" << endl;

        for (const auto& variableEntry : functionEntry.second.second) {
            cout << "  Variable: " << variableEntry.first << ", Type: " << variableEntry.second << endl;  // Variable name and type
        }
    }
}

ParseTreeNode* buildParseTree() {
    string s;
    getline(std::cin, s);
    std::istringstream iss(s);  // Use istringstream to treat the string as a stream

    if (isupper(s[0])) {  // Is a terminal
        std::string kind, lexeme;
        iss >> kind;
        iss >> lexeme;

        Token t = Token(kind, lexeme);
        return new ParseTreeNode(t);
    } else {  // Is a production rule
        vector<ParseTreeNode*> children;

        std::string prodRuleLHS, prodRuleRHSToken;
        vector<string> prodRuleRHS;
        iss >> prodRuleLHS;
        while (iss >> prodRuleRHSToken) {
            if (prodRuleRHSToken == ".EMPTY") {  // Empty terminates the production rule
                Token t = Token(prodRuleLHS, prodRuleRHSToken);
                return new ParseTreeNode(t);
            }

            prodRuleRHS.emplace_back(prodRuleRHSToken);
        }

        for (string prodRuleToken : prodRuleRHS) {
            children.emplace_back(buildParseTree());
        }

        ParseTreeNode* ptn = new ParseTreeNode(prodRuleLHS, prodRuleRHS, children);
        return ptn;
    }
}

void annotateTypes(ParseTreeNode* node, string function_name, string variable_context) {
    // cout << *node << endl;
    if (node->isTerminal()) {
        // NUM 123
        if (node->token.kind == "NUM") {
            node->type = "int";
            return;
        }
        // NULL
        if (node->token.kind == "NULL") {
            node->type = "int*";
            return;
        }
    }

    if (!node->isTerminal()) {
        // start -> BOF procedures EOF
        if (node->prodRuleLHS == "start") {
            // Annotate procedures
            annotateTypes(node->children[1], function_name, variable_context);
        }
        // procedures → main
        else if (node->prodRuleLHS == "procedures" && node->prodRuleRHS[0] == "main") {
            // Annotate main
            annotateTypes(node->children[0], function_name, variable_context);
        }
        // procedures -> procedure procedures
        else if (node->prodRuleLHS == "procedures" && node->prodRuleRHS[0] == "procedure") {
            // Annotate procedure and procedures
            annotateTypes(node->children[0], function_name, variable_context);
            annotateTypes(node->children[1], function_name, variable_context);
        }
        // main -> INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
        else if (node->prodRuleLHS == "main") {
            // Set the global variable function_name
            string new_function_name = "wain";
            ParseTreeNode* firstParam = node->children[3];
            ParseTreeNode* secondParam = node->children[5];

            // Add to symbol table and check for duplicate param names
            annotateTypes(firstParam, new_function_name, new_function_name);   // dcl1
            annotateTypes(secondParam, new_function_name, new_function_name);  // dcl2

            string secondParamType = secondParam->children[1]->type;  // ID : type

            // Check if the second parameter of wain is not int type
            if (secondParamType != "int") {
                throw std::runtime_error("ERROR: The second parameter of wain is not int type.");
            }

            // Annotate the rest of the stuff in main before return
            annotateTypes(node->children[8], new_function_name, new_function_name);  // dcls
            annotateTypes(node->children[9], new_function_name, new_function_name);  // statements

            // Check if the return expression of wain is not int type
            ParseTreeNode* returnExpr = node->children[11];                   // the expr after RETURN
            annotateTypes(returnExpr, new_function_name, new_function_name);  // Evaluate the type of returnExpr

            if (returnExpr && returnExpr->type != "int") {
                throw std::runtime_error("ERROR: The return expression of wain is not int type.");
            }

            // Check well-typedness
            if (secondParamType == "int" && node->children[8]->wellTyped && node->children[9]->wellTyped && returnExpr->type == "int") {
                node->wellTyped = true;
            }
        }

        // procedure → INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
        else if (node->prodRuleLHS == "procedure") {
            // Set the global variable function_name
            string new_function_name = node->children[1]->token.lexeme;
            // Check if function is already defined
            if (symbol_table.find(new_function_name) != symbol_table.end()) {
                throw std::runtime_error("ERROR: Function " + new_function_name + " already declared.");
            }
            symbol_table[new_function_name] = make_pair(vector<string>(), unordered_map<string, string>());

            // Annotate the rest of the stuff in procedure
            annotateTypes(node->children[3], new_function_name, new_function_name);  // params
            annotateTypes(node->children[6], new_function_name, new_function_name);  // dcls
            annotateTypes(node->children[7], new_function_name, new_function_name);  // statements
                                                                                     // Check if the return expression of wain is not int type
            ParseTreeNode* returnExpr = node->children[9];                           // the expr after RETURN
            annotateTypes(returnExpr, new_function_name, new_function_name);         // Evaluate the type of returnExpr

            if (returnExpr && returnExpr->type != "int") {
                throw std::runtime_error("ERROR: The return expression of function " + new_function_name + " is not int type.");
            }
            // Check well-typedness
            if (node->children[6]->wellTyped && node->children[7]->wellTyped && returnExpr->type == "int") {
                node->wellTyped = true;
            }
        }

        // params → paramlist
        else if (node->prodRuleLHS == "params" && node->prodRuleRHS[0] == "paramlist") {
            // Get the dcl type and add it to the function signature vector
            annotateTypes(node->children[0], function_name, variable_context);
        }

        // paramlist → dcl
        // paramlist → dcl COMMA paramlist
        else if (node->prodRuleLHS == "paramlist") {
            // Get the dcl type and add it to the function signature vector
            annotateTypes(node->children[0], function_name, variable_context);
            ParseTreeNode* dclNode = node->children[0];  // dcl -> type ID
            string dclType;

            // Determine the type
            if (dclNode->children[0]->prodRuleRHS.size() > 1 &&  // INT STAR
                dclNode->children[0]->prodRuleRHS[0] == "INT" &&
                dclNode->children[0]->prodRuleRHS[1] == "STAR") {
                dclType = "int*";
            } else if (dclNode->children[0]->prodRuleRHS.size() > 0 &&
                       dclNode->children[0]->prodRuleRHS[0] == "INT") {  // INT
                dclType = "int";
            }

            symbol_table[variable_context].first.emplace_back(dclType);
            if (node->prodRuleRHS.size() == 3) {
                annotateTypes(node->children[2], function_name, variable_context);
            }
        }

        // dcl -> type ID
        else if (node->prodRuleLHS == "dcl") {
            string variableType;
            string variableName = node->children[1]->token.lexeme;  // Get variable Name

            // Determine the type
            if (node->children[0]->prodRuleRHS.size() > 1 &&  // INT STAR
                node->children[0]->prodRuleRHS[0] == "INT" &&
                node->children[0]->prodRuleRHS[1] == "STAR") {
                variableType = "int*";
            } else if (node->children[0]->prodRuleRHS.size() > 0 &&
                       node->children[0]->prodRuleRHS[0] == "INT") {  // INT
                variableType = "int";
            }

            // Check if variable already exists
            if (symbol_table[variable_context].second.find(variableName) != symbol_table[variable_context].second.end()) {
                throw std::runtime_error("ERROR: Duplicate variable " + variableName + " found in function " + function_name);
            } else {
                symbol_table[variable_context].second[variableName] = variableType;
                node->children[1]->type = variableType;
            }
        }

        // dcls -> dcls dcl BECOMES NUM SEMI
        // dcls -> dcls dcl BECOMES NULL SEMI
        else if (node->prodRuleLHS == "dcls" && node->prodRuleRHS.size() == 5) {
            // annotateTypes for dcls and dcl
            annotateTypes(node->children[0], function_name, variable_context);
            annotateTypes(node->children[1], function_name, variable_context);

            // Get the type of dcls and check if it matches NUM or NULL

            string variableName = node->children[1]->children[1]->token.lexeme;

            // Lookup type from symbol table
            string variableType = symbol_table[variable_context].second[variableName];

            if (node->prodRuleRHS[3] == "NUM") {
                if (variableType != "int") {
                    throw std::runtime_error("ERROR: Variable " + variableName + " assigned wrong type (Expected int), got: " + variableType);
                }
                node->children[3]->type = "int";
            } else if (node->prodRuleRHS[3] == "NULL") {
                if (variableType != "int*") {
                    throw std::runtime_error("ERROR: Variable " + variableName + " assigned wrong type (Expected int*), got: " + variableType);
                }
                node->children[3]->type = "int*";
            }

        }

        // Check for variable usage
        // factor -> ID
        // lvalue ->ID
        // need to check prodRuleRHS.size() == 1 because factor has factor ID LPAREN RPAREN
        else if ((node->prodRuleLHS == "factor" || node->prodRuleLHS == "lvalue") && node->prodRuleRHS.size() == 1 && node->prodRuleRHS[0] == "ID") {
            string variableName = node->children[0]->token.lexeme;

            // Check if ID (varaible name) has been declared in synbol table
            if (symbol_table[variable_context].second.find(variableName) == symbol_table[variable_context].second.end()) {
                throw std::runtime_error("ERROR: Variable " + variableName + " used without declaration in function " + function_name);
            }

            // If it exists in symbol table, check it's type, and annotate
            string variableType = symbol_table[variable_context].second[variableName];
            node->type = variableType;               // factor (or lvalue) ID : type
            node->children[0]->type = variableType;  // ID a : type
        }

        // expr -> expr PLUS term
        // expr -> expr MINUS term
        else if (node->prodRuleLHS == "expr" && node->prodRuleRHS.size() == 3) {
            // Get the type of expr and term
            annotateTypes(node->children[0], function_name, variable_context);
            annotateTypes(node->children[2], function_name, variable_context);

            string exprType = node->children[0]->type;
            string termType = node->children[2]->type;

            // If Addition
            if (node->prodRuleRHS[1] == "PLUS") {
                if (exprType == "int" && termType == "int") {
                    node->type = "int";
                } else if (exprType == "int*" && termType == "int") {
                    node->type = "int*";
                } else if (exprType == "int" && termType == "int*") {
                    node->type = "int*";
                } else {
                    throw std::runtime_error("ERROR: Failed to add " + exprType + " and " + termType);
                }
            } else {  // MINUS
                if (exprType == "int" && termType == "int") {
                    node->type = "int";
                } else if (exprType == "int*" && termType == "int") {
                    node->type = "int*";
                } else if (exprType == "int*" && termType == "int*") {
                    node->type = "int";
                } else {
                    throw std::runtime_error("ERROR: Failed to subtract" + termType + " from " + exprType);
                }
            }
            node->wellTyped = true;
        }

        // term → term STAR factor
        // term → term SLASH factor
        // term → term PCT factor
        else if (node->prodRuleLHS == "term" && node->prodRuleRHS.size() == 3) {
            // Get the type of expr and term
            annotateTypes(node->children[0], function_name, variable_context);
            annotateTypes(node->children[2], function_name, variable_context);

            string termType = node->children[0]->type;
            string factorType = node->children[2]->type;

            if (termType == "int" && factorType == "int") {
                node->type = "int";
            } else {
                throw std::runtime_error("ERROR: Failed to multiply, divide, or mod " + termType + " and " + factorType);
            }
        }

        // factor -> AMP lvalue
        else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "AMP") {
            annotateTypes(node->children[1], function_name, variable_context);
            string lvalueType = node->children[1]->type;
            if (lvalueType != "int") {
                throw std::runtime_error("ERROR: Attempting to get an address of a non-integer");
            }
            node->type = "int*";
        }
        
        // factor -> STAR factor
        // lvalue → STAR factor
        else if ((node->prodRuleLHS == "factor" || node->prodRuleLHS == "lvalue") && node->prodRuleRHS[0] == "STAR") {
            annotateTypes(node->children[1], function_name, variable_context);
            string factorType = node->children[1]->type;
            if (factorType != "int*") {
                throw std::runtime_error("ERROR: Attempting to dereference a non-pointer");
            }
            node->type = "int";
        }

        // factor → NEW INT LBRACK expr RBRACK
        else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "NEW") {
            annotateTypes(node->children[3], function_name, variable_context);
            string exprType = node->children[3]->type;
            if (exprType != "int") {
                throw std::runtime_error("ERROR: Attempting to allocate array with non-int size");
            }
            node->type = "int*";
        }

        // lvalue → LPAREN lvalue RPAREN
        else if (node->prodRuleLHS == "lvalue" && node->prodRuleRHS.size() == 3) {
            annotateTypes(node->children[1], function_name, variable_context);
            string lvalueType = node->children[1]->type;
            node->type = lvalueType;
        }

        // factor → ID LPAREN RPAREN
        // factor → ID LPAREN arglist RPAREN
        else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "ID" && node->prodRuleRHS.size() > 1) {
            // Get function (ID) name for the function being called
            string called_function_name = node->children[0]->token.lexeme;

            // Check if the called function has been declared in the symbol table
            if (symbol_table.find(called_function_name) == symbol_table.end()) {
                throw std::runtime_error("ERROR: Function " + called_function_name + " used without declaration.");
            }

            // factor → ID LPAREN RPAREN
            if (node->prodRuleRHS.size() == 3) {
                // Check function accepts 0 arguments
                if (symbol_table[called_function_name].first.size() != 0) {
                    throw std::runtime_error("ERROR: Function " + called_function_name + " called with wrong number of arguments.");
                }
            }

            // Set the context to the called function to validate its arguments
            function_name = called_function_name;

            // Validate the function call, including its arguments if any
            if (node->prodRuleRHS.size() == 4) {
                annotateTypes(node->children[2], function_name, variable_context);  // annotate arglist
            }
            // Since a function call is being processed, assume its return type is int
            node->type = "int";  // factor (or lvalue) ID : type
        }

        // only runs when there is only 1 argument
        // arglist → expr
        else if (node->prodRuleLHS == "arglist" && node->prodRuleRHS.size() == 1) {
            annotateTypes(node->children[0], function_name, variable_context);

            node->type = node->children[0]->type;

            // Go through argvector and compare it with the function signature
            if (symbol_table[function_name].first.size() != 1) {  // only 1 arg
                throw std::runtime_error("ERROR: Function " + function_name + " called with wrong number of arguments.");
            }

            if (node->type != symbol_table[function_name].first[0]) {
                throw std::runtime_error("ERROR: Function " + function_name + " called with wrong argument types.");
            }
        }

        // arglist → expr COMMA arglist
        else if (node->prodRuleLHS == "arglist" && node->prodRuleRHS[0] == "expr" && node->prodRuleRHS[1] == "COMMA") {
            // Traverse to the depth of the arglist branch, storing each arg type in a vector.
            // At the end of the arglist, compare the argvector to the function signature

            vector<string> argVector;
            ParseTreeNode* argNode = node;

            // arglist → expr COMMA arglist
            while (argNode->prodRuleRHS.size() == 3) {
                annotateTypes(argNode->children[0], function_name, variable_context);  // get the type for expr
                                                                                       // annotateTypes(argNode->children[2]);  // annotate arglist

                string argType = argNode->children[0]->type;

                argVector.emplace_back(argType);
                argNode = argNode->children[2];
            }

            // arglist → expr
            annotateTypes(argNode->children[0], function_name, variable_context);  // get the type for expr
            string argType = argNode->children[0]->type;
            argVector.emplace_back(argType);

            // Go through argvector and compare it with the function signature
            if (argVector.size() != symbol_table[function_name].first.size()) {
                throw std::runtime_error("ERROR: Function " + function_name + " called with wrong number of arguments.");
            }

            for (int i = 0; i < symbol_table[function_name].first.size(); i++) {
                if (argVector[i] != symbol_table[function_name].first[i]) {
                    throw std::runtime_error("ERROR: Function " + function_name + " called with wrong argument types.");
                }
            }
        }

        // expr -> term
        else if (node->prodRuleLHS == "expr" && node->prodRuleRHS[0] == "term") {
            // Get the type of term and assign it to the type of expr
            annotateTypes(node->children[0], function_name, variable_context);
            node->type = node->children[0]->type;
            node->wellTyped = true;
        }
        // term -> factor
        else if (node->prodRuleLHS == "term" && node->prodRuleRHS[0] == "factor") {
            // Get the type of factor and assign it to the type of term
            annotateTypes(node->children[0], function_name, variable_context);
            node->type = node->children[0]->type;
        }
        // factor -> NUM
        else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "NUM") {
            node->children[0]->type = "int";
            node->type = "int";
        }
        // factor -> NULL
        else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "NULL") {
            node->children[0]->type = "int*";
            node->type = "int*";
        }
        // factor -> LPAREN expr RPAREN
        else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "LPAREN") {
            // Get the type of expr and assign it to the type of factor
            annotateTypes(node->children[1], function_name, variable_context);
            node->type = node->children[1]->type;
        }
        // statements → .EMPTY
        else if (node->prodRuleLHS == "statements" && node->prodRuleRHS[0] == ".EMPTY") {
            node->wellTyped = true;
        }
        // statements → statements statement
        else if (node->prodRuleLHS == "statements" && node->prodRuleRHS[0] == "statements") {
            // Make sure lvalue and expr have the same type
            annotateTypes(node->children[0], function_name, variable_context);
            annotateTypes(node->children[1], function_name, variable_context);

            if (node->children[0]->wellTyped && node->children[2]->wellTyped) {
                node->wellTyped = true;
            }
        }
        // statement → lvalue BECOMES expr SEMI
        else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "lvalue") {
            // Make sure lvalue and expr have the same type
            annotateTypes(node->children[0], function_name, variable_context);
            annotateTypes(node->children[2], function_name, variable_context);

            if (node->children[0]->type == node->children[2]->type) {
                node->wellTyped = true;
            } else {
                throw std::runtime_error("ERROR: Type mismatch in statement -> lvalue BECOMES expr SEMI ");
            }
        }
        // statement → IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
        else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "IF") {
            // Make sure test is well typed and statements are well typed
            annotateTypes(node->children[2], function_name, variable_context);
            annotateTypes(node->children[5], function_name, variable_context);
            annotateTypes(node->children[9], function_name, variable_context);

            if (node->children[2]->wellTyped && node->children[5]->wellTyped && node->children[9]->wellTyped) {
                node->wellTyped = true;
            }
        }
        // statement → WHILE LPAREN test RPAREN LBRACE statements RBRACE
        else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "WHILE") {
            // Make sure test is well typed and statements are well typed
            annotateTypes(node->children[2], function_name, variable_context);
            annotateTypes(node->children[5], function_name, variable_context);

            if (node->children[2]->wellTyped && node->children[5]->wellTyped) {
                node->wellTyped = true;
            }
        }
        // statement → PRINTLN LPAREN expr RPAREN SEMI
        else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "PRINTLN") {
            // Get the type of expr and assign it to the type of factor
            annotateTypes(node->children[2], function_name, variable_context);
            if (node->children[2]->type == "int") {
                node->wellTyped = true;
            } else {
                throw std::runtime_error("ERROR: Print must have type int");
            }
        }
        // statement → DELETE LBRACK RBRACK expr SEMI
        else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "DELETE") {
            // Get the type of expr and check its type
            annotateTypes(node->children[3], function_name, variable_context);
            if (node->children[3]->type == "int*") {
                node->wellTyped = true;
            } else {
                throw std::runtime_error("ERROR: Delete must have type int*");
            }
        }
        // test → expr EQ expr
        // test → expr NE expr
        // test → expr LT expr
        // test → expr LE expr
        // test → expr GE expr
        // test → expr GT expr
        else if (node->prodRuleLHS == "test") {
            // Get the type of expr1 and expr 2
            annotateTypes(node->children[0], function_name, variable_context);
            annotateTypes(node->children[2], function_name, variable_context);
            if (node->children[0]->type == node->children[2]->type) {
                node->wellTyped = true;
            } else {
                throw std::runtime_error("ERROR: Type mismatch in test compairison");
            }
        }
    }
}

int main() {
    ParseTreeNode* root;

    try {
        root = buildParseTree();
        annotateTypes(root, "", "");
        // printSymbolTable();  // Print the contents of the symbol table
        printParseTree(root, 0, false);
        delete root;  // Ensure we still clean up memory if no exception was thrown

    } catch (const std::runtime_error& e) {
        printSymbolTable();  // Print the contents of the symbol table

        std::cerr << e.what() << std::endl;
        delete root;
        return 1;  // Return a non-zero value to signal an error
    }

    return 0;  // Success
}
