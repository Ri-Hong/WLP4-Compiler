#include <iostream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <vector>

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

    bool isConstant;
    int constantValue;

    ParseTreeNode(string prodRuleLHS, vector<string> prodRuleRHS, vector<ParseTreeNode*> children, string type) : prodRuleLHS(prodRuleLHS), prodRuleRHS(prodRuleRHS), token(), children(children), type(type), wellTyped(false) {}

    ParseTreeNode(Token t, string type) : token(t), wellTyped(false), type(type) {}

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

// variable -> (type, offset from $29)
unordered_map<string, pair<string, string>> symbol_table;
// last offset in our symbol table
int latestOffset = 0;

// Counter used to generate unique label names
int labelCounter = 0;
// Counts number of dcls with values (NUM or NULL). Used to pop correct times in epilogue
int numInits = 0;

// Counts number of deletes (and hence skipDelete labels) we have
int numDeletes = 0;

// varName -> [varValue, varType, dirty?]
// dirty = false when: on initialization (dcl), and if we can reassign
// dirty = true when: variable is reassigned within an if/while block
unordered_map<string, tuple<string, string, bool>> varTable;

// counts how many levels deep we are in an if/while statement. Used for determining if we should do constant propogation on a variable
int ifWhileNestLevel = 0;

// maps: varName -> register #
// used for Simple Register Allocation
unordered_map<string, string> regTable;
vector<string> freeRegisters = {"$28", "$27", "$26", "$25", "$24", "$23", "$22", "$21", "$20", "$19", "$18", "$17", "$16", "$15", "$14", "$13", "$9", "$8"};

// Stores a list of variables where we do & lvalue.
// If a variable is ever dereferenced, we can't put it in registers, it has to go on the stack
vector<string> dereferencedVariables;

string wainParam1Name, wainParam2Name;

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
    cout << "; Symbol Table:" << endl;
    for (const auto& entry : symbol_table) {
        const auto& variableName = entry.first;
        const auto& details = entry.second;  // This is a pair
        const auto& variableType = details.first;
        const auto& offset = details.second;
        cout << "; Variable: " << variableName << ", Type: " << variableType << ", Offset: " << offset << endl;
    }
}

void code(ParseTreeNode* node);

ParseTreeNode* buildParseTree() {
    string s;
    getline(std::cin, s);
    std::istringstream iss(s);  // Use istringstream to treat the string as a stream

    if (isupper(s[0])) {  // Is a terminal
        std::string kind, lexeme;
        iss >> kind;
        iss >> lexeme;

        string colon;
        string type = "";
        if (iss >> colon) {  // If we have a type. Type always follows a colon
            iss >> type;
        }

        Token t = Token(kind, lexeme);
        return new ParseTreeNode(t, type);
    } else {  // Is a production rule
        vector<ParseTreeNode*> children;
        string type = "";

        std::string prodRuleLHS, prodRuleRHSToken;
        vector<string> prodRuleRHS;
        iss >> prodRuleLHS;
        while (iss >> prodRuleRHSToken) {
            if (prodRuleRHSToken == ":") {  // It's a type
                iss >> prodRuleRHSToken;    // type
                type = prodRuleRHSToken;
                break;
            }

            if (prodRuleRHSToken == ".EMPTY") {  // Empty terminates the production rule
                Token t = Token(prodRuleLHS, prodRuleRHSToken);
                return new ParseTreeNode(t, type);
            }

            prodRuleRHS.emplace_back(prodRuleRHSToken);
        }

        for (string prodRuleToken : prodRuleRHS) {
            children.emplace_back(buildParseTree());
        }

        ParseTreeNode* ptn = new ParseTreeNode(prodRuleLHS, prodRuleRHS, children, type);
        return ptn;
    }
}

// given a dcls node, adds it's declarations to the varTable
void addDclsToVarTable(ParseTreeNode* node) {
    // dcls -> .EMPTY
    if (node->prodRuleRHS.size() != 5) {
        return;
    }

    // dcls -> dcls dcl BECOMES NUM SEMI
    if (node->prodRuleRHS[3] == "NUM") {
        string variableName = node->children[1]->children[1]->token.lexeme;
        string variableValue = node->children[3]->token.lexeme;
        string variableType = "int";
        varTable[variableName] = make_tuple(variableValue, variableType, false);
    }
    // dcls -> dcls dcl BECOMES NULL SEMI
    else if (node->prodRuleRHS[3] == "NULL") {
        string variableName = node->children[1]->children[1]->token.lexeme;
        string variableValue = node->children[3]->token.lexeme;
        string variableType = "int*";
        varTable[variableName] = make_tuple("1", variableType, false);
    }
    return addDclsToVarTable(node->children[0]);
}

void printVarTable() {
    cout << "; Variable Table:" << endl;
    for (const auto& entry : varTable) {
        const auto& varName = entry.first;
        const auto& details = entry.second;
        const auto& varValue = std::get<0>(details);
        const auto& varType = std::get<1>(details);
        const auto& dirty = std::get<2>(details) ? "true" : "false";

        cout << "; Variable: " << varName
             << ", Value: " << varValue
             << ", Type: " << varType
             << ", Dirty: " << dirty << endl;
    }
}

bool inDereferencedVars(string s) {
    // Iterate through the dereferencedVariables vector
    for (const string& varName : dereferencedVariables) {
        if (varName == s) {
            // If the variable name matches the input string, return true
            return true;
        }
    }
    // If no match was found, return false
    return false;
}

void printRegTable() {
    cout << "; Register Table:" << endl;
    for (const auto& entry : regTable) {
        const auto& varName = entry.first;   // Variable name
        const auto& regName = entry.second;  // Register name
        cout << "; Variable: " << varName << ", Register: " << regName << endl;
    }
}

void clearRegTable() {
    // Iterate over each entry in regTable
    for (const auto& entry : regTable) {
        const auto& regName = entry.second;  // Extract the register name

        // Add the register back to the freeRegisters list
        freeRegisters.push_back(regName);
    }

    // Clear the regTable to remove all entries
    regTable.clear();
}

// Constant folding done in codegen stage
// Optimizes the parse tree. Current optimizations:
// - Constant folding
// - Constant propogation

// Returns if an optimization was made

// - a varTable maps from <var name> to <var value>

bool optimizeTree(ParseTreeNode* node) {
    // static int callCounter = 0;  // Static counter to track the number of calls to optimizeTree

    // callCounter++;  // Increment the counter on each call

    // cout << "optimizeTree call #" << callCounter << endl;  // Debug output

    // // If callCounter exceeds a certain threshold, return false to indicate no optimization was done
    // // This prevents infinite recursion or looping by breaking out after a certain number of calls
    // if (callCounter > 15) {  // Example threshold
    //     cout << "Call limit exceeded, exiting to prevent infinite loop." << endl;
    //     return false;  // Or return false if you prefer not to terminate the entire program
    // }

    // cout << "; " << *node << endl;

    // used to set wainParam1Name and wainParam2Name
    // main -> INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
    if (node->prodRuleLHS == "main") {
        wainParam1Name = node->children[3]->children[1]->token.lexeme;
        wainParam2Name = node->children[5]->children[1]->token.lexeme;
    }

    // === 0. Constant Folding
    if (node->prodRuleLHS == "term") {
        // term -> term STAR factor
        if (node->prodRuleRHS.size() == 3 && node->prodRuleRHS[1] == "STAR") {
            // optimize term and factor first
            bool didOptimize = optimizeTree(node->children[0]) | optimizeTree(node->children[2]);

            // if both constants
            if (node->children[0]->children[0]->prodRuleRHS[0] == "NUM" && node->children[2]->prodRuleRHS[0] == "NUM") {
                string newResult = to_string(stoi(node->children[0]->children[0]->children[0]->token.lexeme) * stoi(node->children[2]->children[0]->token.lexeme));

                // create NUM
                Token t = Token("kind", newResult);
                ParseTreeNode* newNum = new ParseTreeNode(t, "int");

                // create factor -> NUM
                ParseTreeNode* newFactorNum = new ParseTreeNode("factor", {"NUM"}, {newNum}, "int");

                // replace term STAR factor with factor
                for (auto& child : node->children) {
                    delete child;
                }
                node->prodRuleRHS = {"factor"};

                node->children = {newFactorNum};
                didOptimize = true;
            }
            return didOptimize;
        }
        // term -> term SLASH factor
        else if (node->prodRuleRHS.size() == 3 && node->prodRuleRHS[1] == "SLASH") {
            // optimize term and factor first
            bool didOptimize = optimizeTree(node->children[0]) | optimizeTree(node->children[2]);

            // if both constants
            if (node->children[0]->children[0]->prodRuleRHS[0] == "NUM" && node->children[2]->prodRuleRHS[0] == "NUM") {
                string newResult = to_string(stoi(node->children[0]->children[0]->children[0]->token.lexeme) / stoi(node->children[2]->children[0]->token.lexeme));

                // create NUM
                Token t = Token("kind", newResult);
                ParseTreeNode* newNum = new ParseTreeNode(t, "int");

                // create factor -> NUM
                ParseTreeNode* newFactorNum = new ParseTreeNode("factor", {"NUM"}, {newNum}, "int");

                // replace term STAR factor with factor
                for (auto& child : node->children) {
                    delete child;
                }
                node->prodRuleRHS = {"factor"};

                node->children = {newFactorNum};
                didOptimize = true;
            }
            return didOptimize;
        }
        // term -> term PCT factor
        else if (node->prodRuleRHS.size() == 3 && node->prodRuleRHS[1] == "PCT") {
            // optimize term and factor first
            bool didOptimize = optimizeTree(node->children[0]) | optimizeTree(node->children[2]);

            // if both constants
            if (node->children[0]->children[0]->prodRuleRHS[0] == "NUM" && node->children[2]->prodRuleRHS[0] == "NUM") {
                string newResult = to_string(stoi(node->children[0]->children[0]->children[0]->token.lexeme) % stoi(node->children[2]->children[0]->token.lexeme));

                // create NUM
                Token t = Token("kind", newResult);
                ParseTreeNode* newNum = new ParseTreeNode(t, "int");

                // create factor -> NUM
                ParseTreeNode* newFactorNum = new ParseTreeNode("factor", {"NUM"}, {newNum}, "int");

                // replace term STAR factor with factor
                for (auto& child : node->children) {
                    delete child;
                }
                node->prodRuleRHS = {"factor"};

                node->children = {newFactorNum};
                didOptimize = true;
            }
            return didOptimize;
        }
    } else if (node->prodRuleLHS == "expr") {
        // expr -> expr PLUS term
        if (node->prodRuleRHS.size() == 3 && node->prodRuleRHS[1] == "PLUS") {
            // optimize expr and term first
            bool didOptimize = optimizeTree(node->children[0]) | optimizeTree(node->children[2]);

            // if both constants
            if (node->children[0]->children[0]->children[0]->prodRuleRHS[0] == "NUM" && node->children[2]->children[0]->prodRuleRHS[0] == "NUM") {
                string newResult = to_string(stoi(node->children[0]->children[0]->children[0]->children[0]->token.lexeme) + stoi(node->children[2]->children[0]->children[0]->token.lexeme));

                // create NUM
                Token t = Token("kind", newResult);
                ParseTreeNode* newNum = new ParseTreeNode(t, "int");

                // create factor -> NUM
                ParseTreeNode* newFactorNum = new ParseTreeNode("factor", {"NUM"}, {newNum}, "int");

                // create term -> factor
                ParseTreeNode* newTermFactor = new ParseTreeNode("term", {"factor"}, {newFactorNum}, "int");

                // replace expr PLUS term with term
                for (auto& child : node->children) {
                    delete child;
                }
                node->prodRuleRHS = {"term"};

                node->children = {newTermFactor};
                didOptimize = true;
            }
            return didOptimize;
        }
        // expr -> expr MINUS term
        else if (node->prodRuleRHS.size() == 3 && node->prodRuleRHS[1] == "MINUS") {
            // optimize expr and term first
            bool didOptimize = optimizeTree(node->children[0]) | optimizeTree(node->children[2]);

            // if both constants
            if (node->children[0]->children[0]->children[0]->prodRuleRHS[0] == "NUM" && node->children[2]->children[0]->prodRuleRHS[0] == "NUM") {
                string newResult = to_string(stoi(node->children[0]->children[0]->children[0]->children[0]->token.lexeme) - stoi(node->children[2]->children[0]->children[0]->token.lexeme));

                // create NUM
                Token t = Token("kind", newResult);
                ParseTreeNode* newNum = new ParseTreeNode(t, "int");

                // create factor -> NUM
                ParseTreeNode* newFactorNum = new ParseTreeNode("factor", {"NUM"}, {newNum}, "int");

                // create term -> factor
                ParseTreeNode* newTermFactor = new ParseTreeNode("term", {"factor"}, {newFactorNum}, "int");

                // replace expr PLUS term with term
                for (auto& child : node->children) {
                    delete child;
                }
                node->prodRuleRHS = {"term"};

                node->children = {newTermFactor};
                didOptimize = true;
            }
            return didOptimize;
        }
    }

    // === 1. Constant propogation
    // ====== 1.1  declaration and reassignment of variables
    // main -> INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
    if (node->prodRuleLHS == "main") {
        // optimize everything before LBRACE (can't optimize params, so nothing to do)
        addDclsToVarTable(node->children[8]);

        // optimize statements and expr
        bool didOptimize = optimizeTree(node->children[9]) | optimizeTree(node->children[11]);

        varTable.clear();
        return didOptimize;
    }
    // procedure -> INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
    else if (node->prodRuleLHS == "procedure") {
        // optimize everything before LBRACE (can't optimize params, so nothing to do)
        addDclsToVarTable(node->children[6]);

        // optimize statements and expr
        bool didOptimize = optimizeTree(node->children[7]) | optimizeTree(node->children[9]);

        varTable.clear();
        return didOptimize;
    }
    // statement -> lvalue BECOMES expr SEMI
    else if (node->prodRuleLHS == "statement" && node->prodRuleRHS.size() == 4) {
        bool didOptimize = optimizeTree(node->children[2]);

        // if reassignment took place inside an if/while block and lvalue is ID, mark var as dirty
        if (ifWhileNestLevel != 0 && node->children[0]->prodRuleRHS[0] == "ID") {
            string varName = node->children[0]->children[0]->token.lexeme;
            get<2>(varTable[varName]) = true;
            // cout << "; " << varName << " marked as Dirty" << endl;
            // printVarTable();
        }

        // if lvalue is ID and expr can't resolve to constant, mark it as dirty
        else if (node->children[0]->prodRuleRHS[0] == "ID" && node->children[2]->children[0]->children[0]->prodRuleRHS[0] != "NUM") {
            string varName = node->children[0]->children[0]->token.lexeme;
            get<2>(varTable[varName]) = true;
            // cout << "; " << varName << " marked as Dirty" << endl;
            // printVarTable();
        }

        // if expr is a constant, and lvalue is ID, add the entry to the varTable
        else if (node->children[0]->prodRuleRHS[0] == "ID" && node->children[2]->children[0]->children[0]->prodRuleRHS[0] == "NUM") {
            string varName = node->children[0]->children[0]->token.lexeme;
            string varValue = node->children[2]->children[0]->children[0]->children[0]->token.lexeme;
            varTable[varName] = make_tuple(varValue, "int", false);
        }

        return didOptimize;
    }
    // statement -> WHILE LPAREN test RPAREN LBRACE statements RBRACE
    else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "WHILE") {
        // enter LBRACE
        ifWhileNestLevel++;
        // don't want to apply constant propogation to test.
        // E.g. int x = 3; while (x < 15) {};

        bool didOptimize = optimizeTree(node->children[2]);

        didOptimize = didOptimize | optimizeTree(node->children[5]);

        // exit RBRACE
        ifWhileNestLevel--;

        return didOptimize;
    }

    // statement -> IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements
    else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "IF") {
        // enter LBRACE
        ifWhileNestLevel++;
        // don't want to apply constant propogation to test.
        // E.g. int x = 3; while (x < 15) {};

        bool didOptimize = optimizeTree(node->children[2]);

        didOptimize = didOptimize | optimizeTree(node->children[5]);  // stmts1

        didOptimize = didOptimize | optimizeTree(node->children[9]);  // stmts2

        // exit RBRACE
        ifWhileNestLevel--;

        return didOptimize;
    }

    // ====== 1.2  usage variables
    // factor -> ID
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS.size() == 1 && node->prodRuleRHS[0] == "ID") {
        // look for our variable in varTable.
        // If found, change to factor NUM
        string varName = node->children[0]->token.lexeme;
        bool didOptimize = false;

        if (ifWhileNestLevel == 0) {                                                                    // if not in an if/while block
            if (varTable.find(varName) != varTable.end() && !get<2>(varTable.find(varName)->second)) {  // found and not dirty
                cout << "; " << varName << " FOUND AND NOT DIRTY" << endl;
                // printVarTable();
                didOptimize = true;
                tuple<string, string, bool> result = varTable.find(varName)->second;

                // Change node from factor -> ID to factor -> NUM
                node->prodRuleRHS[0] = "NUM";
                delete node->children[0];

                Token t = Token("kind", get<0>(result));
                node->children[0] = new ParseTreeNode(t, get<1>(result));
                node->prodRuleRHS = {"NUM"};
            }
        }
        return didOptimize;
    }

    bool didOptimize = false;
    for (int i = 0; i < node->children.size(); i++) {
        if (node->prodRuleRHS[i] != ".EMPTY" && !isupper((node->prodRuleRHS[i])[0])) {
            // cout << "Fallthrough call" << endl;
            didOptimize = didOptimize | optimizeTree(node->children[i]);
        }
    }
    return didOptimize;
}

void checkForDereferences(ParseTreeNode* node) {
    // we want to check for factor -> AMP lvalue, and where lvalue -> ID
    // if we have factor -> AMP lvalue and then lvalue -> LPAREN lvalue RPAREN,
    //  we need to unwrap the lvalue as it might be an lvalue -> ID at the end

    // factor -> AMP lvalue
    if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "AMP") {
        // node = lvalue
        node = node->children[1];

        while (true) {  // used to unwrap LPAREN lvalue RPAREN
            if (node->prodRuleRHS.size() != 3) {
                // lvalue -> ID
                if (node->prodRuleRHS[0] == "ID") {
                    string varName = node->children[0]->token.lexeme;
                    dereferencedVariables.emplace_back(varName);
                    cout << "; " << varName << " added to dereferenced variables list" << endl;
                }
                return;
            } else {  // lvalue -> LPAREN lvalue RPAREN
                node = node->children[1];
            }
        }
    }

    for (int i = 0; i < node->children.size(); i++) {
        if (node->prodRuleRHS[i] != ".EMPTY" && !isupper((node->prodRuleRHS[i])[0])) {
            checkForDereferences(node->children[i]);
        }
    }
}

// checks if an expr, term, or factor node is a ID.
// Returns the register that the variable stored in
// If variable is not an ID or not stored in regTable, then return ""
string resolveToID(ParseTreeNode* node) {
    cout << "; " << *node << endl;
    if (node->prodRuleLHS == "expr") {
        if (node->prodRuleRHS[0] == "term") {
            node = node->children[0];
            // term -> factor
            if (node->prodRuleRHS[0] == "factor") {
                node = node->children[0];
                // factor -> ID
                if (node->prodRuleRHS[0] == "ID") {
                    string varName = node->children[0]->token.lexeme;
                    if (regTable.find(varName) != regTable.end()) {  // found
                        return regTable[varName];
                    }
                }
            }
        }
    } else if (node->prodRuleLHS == "term") {
        // term -> factor
        if (node->prodRuleRHS[0] == "factor") {
            node = node->children[0];
            // factor -> ID
            if (node->prodRuleRHS[0] == "ID") {
                string varName = node->children[0]->token.lexeme;
                if (regTable.find(varName) != regTable.end()) {  // found
                    return regTable[varName];
                }
            }
        }
    } else if (node->prodRuleLHS == "factor") {
        // factor -> ID
        if (node->prodRuleRHS[0] == "ID") {
            string varName = node->children[0]->token.lexeme;
            if (regTable.find(varName) != regTable.end()) {  // found
                return regTable[varName];
            }
        }
    }
    return "";
}

void push(string registerX) {
    // cout << "sw " << registerX << ", -4($30) ; push(" << registerX << ")" << endl;
    cout << "sw " << registerX << ", -4($30) ; push(" << registerX << ")" << endl;

    cout << "sub $30, $30, $4" << endl;
}

void pop(string registerX) {
    cout << "add $30, $30, $4 ; pop(" << registerX << ")" << endl;
    cout << "lw " << registerX << ", -4($30)" << endl;
}

void generatePrologue() {
    cout << ".import print" << endl;
    cout << ".import init" << endl;
    cout << ".import new" << endl;
    cout << ".import delete" << endl;
    cout << "lis $4" << endl;
    cout << ".word 4" << endl;
    cout << "sub $29, $30, $4 ; setup frame pointer" << endl;

    if (inDereferencedVars(wainParam1Name)) {
        push("$1");
    }
    if (inDereferencedVars(wainParam2Name)) {
        push("$2");
    }

    cout << "lis $11" << endl;
    cout << ".word 1" << endl;
    cout << "lis $10" << endl;
    cout << ".word print" << endl;
    cout << "beq $0, $0, wain" << endl;
    cout << "; END OF PROLOGUE" << endl;
}

void generateEpilogue() {
    cout << "; START OF EPILOGUE" << endl;
    if (inDereferencedVars(wainParam2Name)) {
        pop("$2");
    }
    if (inDereferencedVars(wainParam1Name)) {
        pop("$1");
    }
    cout << "jr $31" << endl;
}

void initHeap(ParseTreeNode* dcl1) {
    cout << "; START OF INITHEAP" << endl;

    push("$31");
    push("$2");

    // Check if we were called with mips.twoints
    // dcl -> type ID
    // get type of ID
    if (dcl1->children[1]->type == "int") {  // called with mips.twoints
        cout << "add $2, $0, $0" << endl;    // $2 = 0
    }

    // Call init
    cout << "lis $3" << endl;
    cout << ".word init" << endl;
    cout << "jalr $3" << endl;
    pop("$2");
    pop("$31");
    cout << "; END OF INITHEAP" << endl;
}

void generateLabel(string label) {
    cout << label << to_string(labelCounter) << ":" << endl;
    labelCounter++;
}

string getVarNameFromLvalue(ParseTreeNode* node) {
    string answer;
    if (node->prodRuleRHS.size() == 3) {  // lvalue -> LPAREN lvalue RPAREN
        answer = getVarNameFromLvalue(node->children[1]);
    } else {
        // lvalue -> ID
        answer = node->children[0]->token.lexeme;
    }
    return answer;
}

// Pushes the arguments in arglist in the rule factor -> ID LPAREN arglist RPAREN
// Returns number of arguments pushed
int pushArgs(ParseTreeNode* node) {
    // arglist -> expr
    if (node->prodRuleRHS.size() == 1) {
        code(node->children[0]);  // code(expr)
        push("$3");
        return 1;
    }
    // arglist -> expr COMMA arglist
    else {
        code(node->children[0]);  // code(expr)
        push("$3");
        return 1 + pushArgs(node->children[2]);
    }
}

// Increments all offets in the symbol table by value inc. Used for procedure calls
void incrementSymbolTable(int inc) {
    for (auto& entry : symbol_table) {
        // Extract current offset as an integer
        int offset = stoi(entry.second.second);
        // Increment the offset
        offset += inc;
        // Update the symbol table entry with the new offset
        entry.second.second = to_string(offset);
    }
}

// Helper for getNumParams. node must be of type paramlist
int getNumParamsRec(ParseTreeNode* node) {
    // paramlist -> dcl
    if (node->prodRuleRHS.size() == 1) {
        return 1;
    }
    // paramlist -> dcl COMMA paramlist
    else {
        return 1 + getNumParamsRec(node->children[2]);
    }
}

// node must be a params node.
// Returns number of params
int getNumParams(ParseTreeNode* node) {
    // params -> .EMPTY
    if (node->prodRuleRHS.size() == 0) {
        return 0;
    }
    // params -> paramlist
    else {
        return getNumParamsRec(node->children[0]);
    }
}

// Used to push all registers in procedures
void pushAllRegisters() {
    push("$5");
    push("$6");
    push("$7");
}

// Used to pop all registers in procedures
void popAllRegisters() {
    pop("$7");
    pop("$6");
    pop("$5");
}

void code(ParseTreeNode* node) {
    // start -> BOF procedures EOF
    if (node->prodRuleLHS == "start") {
        // Codegen procedures
        code(node->children[1]);
    }
    // procedures → main
    else if (node->prodRuleLHS == "procedures" && node->prodRuleRHS[0] == "main") {
        // Codegen main
        code(node->children[0]);
    }
    // procedures -> procedure procedures
    else if (node->prodRuleLHS == "procedures" && node->prodRuleRHS[0] == "procedure") {
        // codegen procedure and procedures
        code(node->children[0]);
        code(node->children[1]);
    }
    // procedure → INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
    else if (node->prodRuleLHS == "procedure") {
        cout << "; Symbol table cleared" << endl;
        symbol_table.clear();
        // clearRegTable();
        latestOffset = 0;
        cout << "F" << node->children[1]->token.lexeme << ":" << endl;  // prepend ID with F
        cout << "sub $29, $30, $4" << endl;
        code(node->children[3]);  // code(params)
        code(node->children[6]);  // code(dcls)
        cout << "; Push All Registers" << endl;
        pushAllRegisters();

        printSymbolTable();

        // params
        int numParams = getNumParams(node->children[3]);
        incrementSymbolTable(4 * numParams);

        cout << "; Add 4 * # params to symbol table???" << endl;
        printSymbolTable();

        code(node->children[7]);  // code(stmts)
        code(node->children[9]);  // code(expr)
        cout << "; Pop All Registers" << endl;
        popAllRegisters();
        cout << "add $30, $29, $4" << endl;

        // for (int i = 0; i < numInits; i++) {
        //     cout << "add $30, $30, $4 ; resetting stack)" << endl;
        // }

        cout << "jr $31" << endl;
    }
    // main -> INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
    else if (node->prodRuleLHS == "main") {
        symbol_table.clear();
        // clearRegTable();
        latestOffset = 0;
        // print label for wain
        cout << "wain:" << endl;

        // check if dereferenced. If not, add to regTable. Else, add to symbol_table (codegen)

        if (!inDereferencedVars(node->children[3]->children[1]->token.lexeme)) {
            // Add register to regTable
            string reg = "$1";
            regTable[node->children[3]->children[1]->token.lexeme] = reg;
            cout << "; Variable " << node->children[3]->children[1]->token.lexeme << " assigned to register "
                 << "$1" << endl;
        } else {
            // code dcl1
            code(node->children[3]);
        }

        if (!inDereferencedVars(node->children[5]->children[1]->token.lexeme)) {
            // Add register to regTable
            string reg = "$2";
            regTable[node->children[5]->children[1]->token.lexeme] = reg;
            cout << "; Variable " << node->children[5]->children[1]->token.lexeme << " assigned to register "
                 << "$2" << endl;
        } else {
            // code dcl2
            code(node->children[5]);
        }

        // initHeap(dcl1);
        initHeap(node->children[3]);
        // code dcls
        code(node->children[8]);
        // code stmts
        code(node->children[9]);
        // code expr for return expression
        code(node->children[11]);
    }
    // params → paramlist
    else if (node->prodRuleLHS == "params" && node->prodRuleRHS[0] == "paramlist") {
        code(node->children[0]);
    }

    // paramlist → dcl
    // paramlist → dcl COMMA paramlist
    else if (node->prodRuleLHS == "paramlist") {
        code(node->children[0]);  // code dcl
        // paramlist -> dcl COMMA paramlist
        if (node->prodRuleRHS.size() == 3) {
            code(node->children[2]);  // code paramlist
        }
    }

    // dcls -> dcls dcl BECOMES NUM SEMI
    else if (node->prodRuleLHS == "dcls" && node->prodRuleRHS.size() == 5 && node->prodRuleRHS[3] == "NUM") {
        // code for dcls
        code(node->children[0]);

        // get the value of NUM and assign it to the variable in dcl
        string numValue = node->children[3]->token.lexeme;
        string variableName = node->children[1]->children[1]->token.lexeme;  // Get variable name of dcl

        // Check if there is space in regTable.
        // If there is, put it in regTable (registers).
        // Otherwise, put it on symbolTable (stack)

        // Check if we have free registers and the variable is not dereferenced
        if (freeRegisters.size() != 0 && !inDereferencedVars(variableName)) {
            // Add register to regTable
            string reg = freeRegisters.back();
            regTable[variableName] = reg;
            freeRegisters.pop_back();

            cout << "; Variable " << variableName << " assigned to register " << reg << endl;

            cout << "lis " << reg << endl;
            cout << ".word " << numValue << endl;

        } else {  // no free registers, use conventional method
            // code for dcl
            code(node->children[1]);  // Adds variable to symbol table

            string offset = symbol_table[variableName].second;

            cout << "lis $3" << endl;
            cout << ".word " << numValue << endl;
            push("$3");
        }
    }

    // dcls -> dcls dcl BECOMES NULL SEMI
    else if (node->prodRuleLHS == "dcls" && node->prodRuleRHS.size() == 5 && node->prodRuleRHS[3] == "NULL") {
        // code for dcls
        code(node->children[0]);

        string variableName = node->children[1]->children[1]->token.lexeme;  // Get variable name of dcl

        // Check if there is space in regTable.
        // If there is, put it in regTable (registers).
        // Otherwise, put it on symbolTable (stack)

        // Check if we have free registers and the variable is not dereferenced
        if (freeRegisters.size() != 0 && !inDereferencedVars(variableName)) {
            // Add register to regTable
            string reg = freeRegisters.back();
            regTable[variableName] = reg;
            freeRegisters.pop_back();

            cout << "; Variable " << variableName << " assigned to register " << reg << endl;

            cout << "lis " << reg << endl;
            cout << ".word 1" << endl;
        } else {  // no free registers, use conventional method
            // code for dcl
            code(node->children[1]);  // Adds variable to symbol table

            string offset = symbol_table[variableName].second;

            cout << "lis $3" << endl;
            cout << ".word 1" << endl;
            push("$3");
        }
    }

    // dcl -> type ID
    // should only run on param and main dcls
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
        // Add variable to the symbol table
        cout << "; Variable " << variableName << " added to symbol table with offset " << latestOffset << endl;
        symbol_table[variableName] = make_pair(variableType, to_string(latestOffset));
        latestOffset -= 4;
    }

    // statements → statements statement
    else if (node->prodRuleLHS == "statements" && node->prodRuleRHS[0] == "statements") {
        code(node->children[0]);
        code(node->children[1]);
    }
    // statement → lvalue BECOMES expr SEMI
    else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "lvalue") {
        ParseTreeNode* expr = node->children[2];

        // node = lvalue
        node = node->children[0];

        while (true) {  // used to unwrap LPAREN lvalue RPAREN
            if (node->prodRuleRHS.size() != 3) {
                // lvalue -> ID
                if (node->prodRuleRHS[0] == "ID") {
                    // code(expr)
                    code(expr);
                    string varName = node->children[0]->token.lexeme;

                    // check if variable is in register or symbol_table
                    if (regTable.find(varName) != regTable.end()) {  // in regTable
                        cout << "add " << regTable.find(varName)->second << ", $0, $3" << endl;
                    } else {
                        cout << "sw $3, " << symbol_table[varName].second << "($29)" << endl;
                    }
                }
                // lvalue -> STAR factor
                else if (node->prodRuleRHS.size() == 2) {
                    string exprReg = resolveToID(expr);
                    string factorReg = resolveToID(node->children[1]);

                    // if not in register. Load code like normal
                    if (exprReg == "" && factorReg == "") {
                        // code(expr)
                        code(expr);
                        push("$3");
                        // code(factor)
                        code(node->children[1]);
                        pop("$5");

                        exprReg = "$5";
                        factorReg = "$3";
                    } else if (factorReg == "") {
                        // code(factor)
                        code(node->children[1]);
                        factorReg = "$3";
                    } else if (exprReg == "") {
                        // code(expr)
                        code(expr);
                        exprReg = "$3";
                    }

                    cout << "sw " << exprReg << ", 0(" << factorReg << ")" << endl;
                }
                break;
            } else {  // lvalue -> LPAREN lvalue RPAREN
                node = node->children[1];
            }
        }

    }
    // statement → IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
    else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "IF") {
        int currentLabelCounterValue = labelCounter;
        labelCounter++;
        cout << "; If" << endl;
        code(node->children[2]);  // code(test)
        cout << "beq $3, $0, else" << currentLabelCounterValue << endl;
        code(node->children[5]);  // code(statements1)
        cout << "beq $0, $0, endif" << currentLabelCounterValue << endl;
        cout << "else" << currentLabelCounterValue << ":" << endl;
        code(node->children[9]);  // code(statements2)
        cout << "endif" << currentLabelCounterValue << ":" << endl;
    }
    // statement → WHILE LPAREN test RPAREN LBRACE statements RBRACE
    else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "WHILE") {
        int currentLabelCounterValue = labelCounter;
        labelCounter++;
        cout << "; While" << endl;
        cout << "loop" << currentLabelCounterValue << ":" << endl;
        code(node->children[2]);  // code(test)
        cout << "beq $3, $0, endWhile" << currentLabelCounterValue << endl;
        code(node->children[5]);  // code(statements)
        cout << "beq $0, $0, loop" << currentLabelCounterValue << endl;
        cout << "endWhile" << currentLabelCounterValue << ":" << endl;
    }
    // statement → PRINTLN LPAREN expr RPAREN SEMI
    else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "PRINTLN") {
        push("$1");
        code(node->children[2]);  // code(expr)
        cout << "add $1, $3, $0" << endl;
        push("$31");
        cout << "lis $5" << endl;
        cout << ".word print" << endl;
        cout << "jalr $5" << endl;
        pop("$31");
        pop("$1");
    }
    // statement → DELETE LBRACK RBRACK expr SEMI
    else if (node->prodRuleLHS == "statement" && node->prodRuleRHS[0] == "DELETE") {
        // code(expr)
        code(node->children[3]);
        push("$1");
        cout << "beq $3, $11, skipDelete" << numDeletes << endl;
        cout << "add $1, $3, $0" << endl;
        push("$31");
        cout << "lis $5" << endl;
        cout << ".word delete" << endl;
        cout << "jalr $5" << endl;
        pop("$31");
        pop("$1");
        cout << "skipDelete" << numDeletes << ":" << endl;
        numDeletes++;
    }
    // expr -> term
    else if (node->prodRuleLHS == "expr" && node->prodRuleRHS[0] == "term") {
        // Check if term is a constant
        if (node->children[0]->children[0]->prodRuleRHS[0] == "NUM") {
            cout << "lis $3" << endl;
            cout << ".word " << node->children[0]->children[0]->children[0]->token.lexeme << endl;
            return;
        }

        // code(term);
        code(node->children[0]);
    }
    // expr -> expr PLUS term
    // expr -> expr MINUS term
    else if (node->prodRuleLHS == "expr" && node->prodRuleRHS.size() == 3) {
        // if term and expr are both ints
        if (node->children[0]->type == "int" && node->children[2]->type == "int") {
            string exprReg = resolveToID(node->children[0]);
            string termReg = resolveToID(node->children[2]);

            // if not in register. Load code like normal
            if (exprReg == "" && termReg == "") {
                // code(expr)
                code(node->children[0]);
                push("$3");
                // code(term)
                code(node->children[2]);
                pop("$5");

                // $3 = term, $5 = expr
                exprReg = "$5";
                termReg = "$3";
            } else if (termReg == "") {
                // code(term)
                code(node->children[2]);
                termReg = "$3";
            } else if (exprReg == "") {
                // code(factor)
                code(node->children[0]);
                exprReg = "$3";
            }

            // If PLUS
            if (node->prodRuleRHS[1] == "PLUS") {
                // cout << "add $3, $5, $3" << endl;
                cout << "add $3, " << exprReg << ", " << termReg << endl;
            } else {  // MINUS
                // cout << "sub $3, $5, $3" << endl;
                cout << "sub $3, " << exprReg << ", " << termReg << endl;
            }
        }

        // expr -> expr PLUS term; expr : int* and term = int
        else if (node->prodRuleRHS[1] == "PLUS" && node->children[0]->type == "int*" && node->children[2]->type == "int") {
            string exprReg = resolveToID(node->children[0]);
            string termReg = resolveToID(node->children[2]);

            // if not in register. Load code like normal
            if (exprReg == "" && termReg == "") {
                code(node->children[0]);  // code(expr)
                push("$3");
                code(node->children[2]);  // code(term)
                cout << "mult $3, $4" << endl;
                cout << "mflo $3" << endl;
                pop("$5");
                cout << "add $3, $5, $3" << endl;
            } else if (termReg == "") {
                code(node->children[2]);  // code(term)
                cout << "mult $3, $4" << endl;
                cout << "mflo $3" << endl;
                cout << "add $3, " << exprReg << ", $3" << endl;
            } else if (exprReg == "") {
                cout << "mult " << termReg << ", $4" << endl;
                cout << "mflo $5" << endl;
                code(node->children[0]);  // code(expr)
                cout << "add $3, $5, $3" << endl;
            } else {  // BOTH in registers
                cout << "mult " << termReg << ", $4" << endl;
                cout << "mflo $3" << endl;
                cout << "add $3, " << exprReg << ", $3" << endl;
            }
        }
        // expr -> expr PLUS term; expr : int and term = int*
        else if (node->prodRuleRHS[1] == "PLUS" && node->children[0]->type == "int" && node->children[2]->type == "int*") {
            string exprReg = resolveToID(node->children[0]);
            string termReg = resolveToID(node->children[2]);

            // if not in register. Load code like normal
            if (exprReg == "" && termReg == "") {
                code(node->children[2]);  // code(term)
                push("$3");
                code(node->children[0]);  // code(expr)
                cout << "mult $3, $4" << endl;
                cout << "mflo $3" << endl;
                pop("$5");
                cout << "add $3, $5, $3" << endl;
            } else if (termReg == "") {
                cout << "mult " << exprReg << ", $4" << endl;
                cout << "mflo $5" << endl;
                code(node->children[2]);  // code(term)
                cout << "add $3, $5, $3" << endl;
            } else if (exprReg == "") {
                code(node->children[0]);  // code(expr)
                cout << "mult $3, $4" << endl;
                cout << "mflo $3" << endl;
                cout << "add $3, " << termReg << ", $3" << endl;
            } else {  // BOTH in registers
                cout << "mult " << exprReg << ", $4" << endl;
                cout << "mflo $3" << endl;
                cout << "add $3, " << termReg << ", $3" << endl;
            }
        }
        // expr -> expr MINUS term; expr : int* and term = int
        else if (node->prodRuleRHS[1] == "MINUS" && node->children[0]->type == "int*" && node->children[2]->type == "int") {
            string exprReg = resolveToID(node->children[0]);
            string termReg = resolveToID(node->children[2]);

            cout << "; expr MINUS term. ER: " << exprReg << " TR:" << termReg << endl;
            // if not in register. Load code like normal
            if (exprReg == "" && termReg == "") {
                code(node->children[0]);  // code(expr)
                push("$3");
                code(node->children[2]);  // code(term)
                cout << "mult $3, $4" << endl;
                cout << "mflo $3" << endl;
                pop("$5");
                cout << "sub $3, $5, $3" << endl;
            } else if (termReg == "") {
                code(node->children[2]);  // code(term)
                cout << "mult $3, $4" << endl;
                cout << "mflo $3" << endl;
                cout << "sub $3, " << exprReg << ", $3" << endl;
            } else if (exprReg == "") {
                cout << "mult " << termReg << ", $4" << endl;
                cout << "mflo $5" << endl;
                code(node->children[0]);  // code(expr)
                cout << "sub $3, $5, $3" << endl;
            } else {  // BOTH in registers
                cout << "mult " << termReg << ", $4" << endl;
                cout << "mflo $3" << endl;
                cout << "sub $3, " << exprReg << ", $3" << endl;
            }
        }
        // expr -> expr MINUS term; expr : int* and term = int*
        else if (node->prodRuleRHS[1] == "MINUS" && node->children[0]->type == "int*" && node->children[2]->type == "int*") {
            string exprReg = resolveToID(node->children[0]);
            string termReg = resolveToID(node->children[2]);

            // if not in register. Load code like normal
            if (exprReg == "" && termReg == "") {
                code(node->children[0]);  // code(expr)
                push("$3");
                code(node->children[2]);  // code(term)
                pop("$5");
                cout << "sub $3, $5, $3" << endl;
                cout << "div $3, $4" << endl;
                cout << "mflo $3" << endl;
            } else if (termReg == "") {
                code(node->children[2]);  // code(term)
                cout << "sub $3, " << exprReg << ", $3" << endl;
                cout << "div $3, $4" << endl;
                cout << "mflo $3" << endl;
            } else if (exprReg == "") {
                code(node->children[0]);  // code(expr)
                cout << "sub $3, $3, " << termReg << endl;
                cout << "div $3, $4" << endl;
                cout << "mflo $3" << endl;
            } else {  // BOTH in registers
                cout << "sub $3, " << exprReg << ", " << termReg << endl;
                cout << "div $3, $4" << endl;
                cout << "mflo $3" << endl;
            }
        }
    }
    // term -> factor
    else if (node->prodRuleLHS == "term" && node->prodRuleRHS[0] == "factor") {
        // code(factor);
        code(node->children[0]);
    }
    // term → term STAR factor
    // term → term SLASH factor
    // term → term PCT factor
    else if (node->prodRuleLHS == "term" && node->prodRuleRHS.size() == 3) {
        string termReg = resolveToID(node->children[0]);
        string factorReg = resolveToID(node->children[2]);

        // if not in register. Load code like normal
        if (termReg == "" && factorReg == "") {
            // code(term)
            code(node->children[0]);
            push("$3");
            // code(factor)
            code(node->children[2]);
            pop("$5");

            termReg = "$5";
            factorReg = "$3";
        } else if (termReg == "") {
            // code(term)
            code(node->children[0]);
            termReg = "$3";
        } else if (factorReg == "") {
            // code(factor)
            code(node->children[2]);
            factorReg = "$3";
        }

        // If MULT
        if (node->prodRuleRHS[1] == "STAR") {
            // cout << "mult $5, $3" << endl;
            cout << "mult " << termReg << ", " << factorReg << endl;
            cout << "mflo $3" << endl;
        } else if (node->prodRuleRHS[1] == "SLASH") {
            // cout << "div $5, $3" << endl;
            cout << "div " << termReg << ", " << factorReg << endl;
            cout << "mflo $3" << endl;
        } else {  // PCT
                  // cout << "div $5, $3" << endl;
            cout << "div " << termReg << ", " << factorReg << endl;
            cout << "mfhi $3" << endl;
        }
    }
    // factor -> NUM
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "NUM") {
        string value = node->children[0]->token.lexeme;
        cout << "lis $3" << endl;
        cout << ".word " << value << endl;
    }
    // factor -> NULL
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "NULL") {
        cout << "add $3, $0, $11 ;" << endl;
    }
    // factor -> ID
    // need to check prodRuleRHS.size() == 1 because factor has factor ID LPAREN RPAREN
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS.size() == 1 && node->prodRuleRHS[0] == "ID") {
        string variableName = node->children[0]->token.lexeme;

        // Check if var is stored in register or symbol table
        if (regTable.find(variableName) != regTable.end()) {
            cout << "add $3, $0, " << regTable.find(variableName)->second << endl;
        } else {
            // Get the offset from symbol table
            string offset = symbol_table[variableName].second;
            cout << "lw $3, " << offset << "($29)" << endl;
        }
    }
    // factor -> LPAREN expr RPAREN
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "LPAREN") {
        // code(expr);
        // cout << "; LPAREN expr RPAREN" << endl;
        // printSymbolTable();
        // printRegTable();
        code(node->children[1]);
        // cout << "; end LPAREN expr RPAREN" << endl;
    }
    // factor -> AMP lvalue
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "AMP") {
        // node = lvalue
        node = node->children[1];

        while (true) {
            if (node->prodRuleRHS.size() != 3) {
                // lvalue -> ID
                if (node->prodRuleRHS[0] == "ID") {
                    string variableName = node->children[0]->token.lexeme;

                    // dump the value of lvalue -> ID into $3

                    // Check if var is stored in register or symbol table
                    if (regTable.find(variableName) != regTable.end()) {
                        cout << "add $3, $0, " << regTable.find(variableName)->second << endl;
                    } else {
                        cout << "lis $3" << endl;
                        string offset = symbol_table[variableName].second;
                        cout << ".word " << offset << endl;
                        cout << "add $3, $3, $29" << endl;
                    }

                }
                // lvalue -> STAR factor
                else if (node->prodRuleRHS.size() == 2) {
                    // code(factor)
                    code(node->children[1]);
                }
                break;
            } else {  // lvalue -> LPAREN lvalue RPAREN
                node = node->children[1];
            }
        }

    }
    // factor → ID LPAREN RPAREN
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "ID" && node->prodRuleRHS.size() == 3) {
        push("$29");
        push("$31");

        cout << "lis $5" << endl;
        cout << ".word F" << node->children[0]->token.lexeme << endl;
        cout << "jalr $5" << endl;

        pop("$31");
        pop("$29");
    }

    // factor → ID LPAREN arglist RPAREN
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "ID" && node->prodRuleRHS.size() == 4) {
        push("$29");
        push("$31");

        cout << "; Push Args" << endl;
        int numArgsPushed = pushArgs(node->children[2]);

        cout << "lis $5" << endl;
        cout << ".word F" << node->children[0]->token.lexeme << endl;
        cout << "jalr $5" << endl;

        for (int i = 0; i < numArgsPushed; i++) {
            pop("$31");
        }

        pop("$31");
        pop("$29");
    }

    // factor -> STAR factor
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "STAR") {
        code(node->children[1]);  // code(factor2)
        cout << "lw $3, 0($3)" << endl;
    }

    // factor → NEW INT LBRACK expr RBRACK
    else if (node->prodRuleLHS == "factor" && node->prodRuleRHS[0] == "NEW") {
        // code(expr)
        code(node->children[3]);
        push("$1");
        cout << "add $1, $3, $0" << endl;
        push("$31");
        cout << "lis $5" << endl;
        cout << ".word new" << endl;
        cout << "jalr $5" << endl;
        pop("$31");
        cout << "bne $3, $0, 1" << endl;
        cout << "add $3, $11, $0" << endl;
        pop("$1");
    }

    // lvalue → LPAREN lvalue RPAREN
    else if (node->prodRuleLHS == "lvalue" && node->prodRuleRHS.size() == 3) {
        code(node->children[1]);
    }
    // test → expr EQ expr
    // test → expr NE expr
    // test → expr LT expr
    // test → expr LE expr
    // test → expr GE expr
    // test → expr GT expr
    else if (node->prodRuleLHS == "test") {
        string expr1Reg = resolveToID(node->children[0]);
        string expr2Reg = resolveToID(node->children[2]);

        // if not in register. Load code like normal
        if (expr1Reg == "" && expr2Reg == "") {
            code(node->children[0]);  // code(expr1)
            push("$3");
            code(node->children[2]);  // code(expr2)
            pop("$5");

            expr1Reg = "$5";
            expr2Reg = "$3";
        } else if (expr1Reg == "") {
            code(node->children[0]);  // code(expr1)
            expr1Reg = "$3";
        } else if (expr2Reg == "") {
            code(node->children[2]);  // code(expr2)
            expr2Reg = "$3";
        }

        // both exprs are ints
        if (node->children[0]->type == "int") {
            if (node->prodRuleRHS[1] == "LT") {
                // cout << "slt $3, $5, $3" << endl;
                cout << "slt $3, " << expr1Reg << ", " << expr2Reg << endl;
            } else if (node->prodRuleRHS[1] == "GT") {
                // cout << "slt $3, $3, $5" << endl;
                cout << "slt $3, " << expr2Reg << ", " << expr1Reg << endl;
            } else if (node->prodRuleRHS[1] == "NE") {
                // cout << "slt $6, $3, $5" << endl;  // $6 = $3 < $5
                cout << "slt $6, " << expr2Reg << ", " << expr1Reg << endl;
                // cout << "slt $7, $5, $3" << endl;  // $7 = $5 < $3
                cout << "slt $7, " << expr1Reg << ", " << expr2Reg << endl;
                cout << "add $3, $6, $7" << endl;
            } else if (node->prodRuleRHS[1] == "EQ") {
                // cout << "slt $6, $3, $5" << endl;  // $6 = $3 < $5
                cout << "slt $6, " << expr2Reg << ", " << expr1Reg << endl;
                // cout << "slt $7, $5, $3" << endl;  // $7 = $5 < $3
                cout << "slt $7, " << expr1Reg << ", " << expr2Reg << endl;
                cout << "add $3, $6, $7" << endl;
                cout << "sub $3, $11, $3" << endl;
            } else if (node->prodRuleRHS[1] == "LE") {
                // cout << "slt $6, $3, $5" << endl;   // $6 = $3 < $5 : expr2 < expr1 : expr1 > expr2
                cout << "slt $6, " << expr2Reg << ", " << expr1Reg << endl;
                cout << "sub $3, $11, $6" << endl;  // !(expr1 > expr2) : expr1 <= expr2
            } else if (node->prodRuleRHS[1] == "GE") {
                // cout << "slt $6, $5, $3" << endl;   // $6 = $5 < $3 : $3 > $5 : expr2 > expr1 : expr1 < expr2
                cout << "slt $6, " << expr1Reg << ", " << expr2Reg << endl;
                cout << "sub $3, $11, $6" << endl;  // !(expr1 < expr2) : expr1 >= expr2
            }
        }
        // both exprs are int*
        else {
            if (node->prodRuleRHS[1] == "LT") {
                // cout << "slt $3, $5, $3" << endl;
                cout << "sltu $3, " << expr1Reg << ", " << expr2Reg << endl;
            } else if (node->prodRuleRHS[1] == "GT") {
                // cout << "slt $3, $3, $5" << endl;
                cout << "sltu $3, " << expr2Reg << ", " << expr1Reg << endl;
            } else if (node->prodRuleRHS[1] == "NE") {
                // cout << "slt $6, $3, $5" << endl;  // $6 = $3 < $5
                cout << "sltu $6, " << expr2Reg << ", " << expr1Reg << endl;
                // cout << "slt $7, $5, $3" << endl;  // $7 = $5 < $3
                cout << "sltu $7, " << expr1Reg << ", " << expr2Reg << endl;
                cout << "add $3, $6, $7" << endl;
            } else if (node->prodRuleRHS[1] == "EQ") {
                // cout << "slt $6, $3, $5" << endl;  // $6 = $3 < $5
                cout << "sltu $6, " << expr2Reg << ", " << expr1Reg << endl;
                // cout << "slt $7, $5, $3" << endl;  // $7 = $5 < $3
                cout << "sltu $7, " << expr1Reg << ", " << expr2Reg << endl;
                cout << "add $3, $6, $7" << endl;
                cout << "sub $3, $11, $3" << endl;
            } else if (node->prodRuleRHS[1] == "LE") {
                // cout << "slt $6, $3, $5" << endl;   // $6 = $3 < $5 : expr2 < expr1 : expr1 > expr2
                cout << "sltu $6, " << expr2Reg << ", " << expr1Reg << endl;
                cout << "sub $3, $11, $6" << endl;  // !(expr1 > expr2) : expr1 <= expr2
            } else if (node->prodRuleRHS[1] == "GE") {
                // cout << "slt $6, $5, $3" << endl;   // $6 = $5 < $3 : $3 > $5 : expr2 > expr1 : expr1 < expr2
                cout << "sltu $6, " << expr1Reg << ", " << expr2Reg << endl;
                cout << "sub $3, $11, $6" << endl;  // !(expr1 < expr2) : expr1 >= expr2
            }
        }
    }
}

int main() {
    ParseTreeNode* root;

    try {
        root = buildParseTree();

        bool didOptimize = optimizeTree(root);
        int optimizeCounter = 0;
        while (didOptimize) {
            varTable.clear();
            didOptimize = optimizeTree(root);
            optimizeCounter++;
        }
        cout << "; Optimizations: " << optimizeCounter << endl;
        // printParseTree(root, 0, true);

        checkForDereferences(root);

        generatePrologue();
        code(root);  // Generates code for the body

        // printSymbolTable();  // Print the contents of the symbol table
        // printParseTree(root, 0, false);
        generateEpilogue();
        // printSymbolTable();  // Print the contents of the symbol table

        delete root;  // Ensure we still clean up memory if no exception was thrown

    } catch (const std::runtime_error& e) {
        printSymbolTable();  // Print the contents of the symbol table

        std::cerr << e.what() << std::endl;
        delete root;
        return 1;  // Return a non-zero value to signal an error
    }

    return 0;  // Success
}
