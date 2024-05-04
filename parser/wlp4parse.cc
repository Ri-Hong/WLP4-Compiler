#include <iostream>
#include <memory>  // For std::unique_ptr
#include <queue>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "wlp4data.h"

struct ParseTreeNode {
    std::string value;  // Either the entire prod rule, or the kind and lexeme if its a terminal
    std::vector<ParseTreeNode*> children;

    ParseTreeNode(std::string value) : value(value) {}

    // Recursive function to print the parse tree
    void printPreOrder(int depth = 0) const {
        std::cout << value << std::endl;
        for (auto child : children) {
            child->printPreOrder(depth + 1);
        }
    }

    void printLevelOrder() const {
        std::queue<const ParseTreeNode*> queue;
        queue.push(this);  // Start with the root node

        while (!queue.empty()) {
            int levelSize = queue.size();  // Number of elements at the current level

            for (int i = 0; i < levelSize; ++i) {
                const ParseTreeNode* currentNode = queue.front();
                queue.pop();

                // Print the current node's value
                std::cout << currentNode->value << "; ";

                // Add all children of the current node to the queue
                for (auto child : currentNode->children) {
                    queue.push(child);
                }

                // Space between nodes at the same level
                if (i < levelSize - 1) {
                    std::cout << " ";
                }
            }

            // Newline after each level
            std::cout << std::endl;
        }
    }

    // Destructor to clean up memory used by children
    ~ParseTreeNode() {
        for (auto child : children) {
            delete child;  // Delete each child
        }
    }
};

struct Token {
    std::string kind, lexeme;

    Token(const std::string& kind, const std::string& lexeme) : kind(kind), lexeme(lexeme) {}
};

void printParseTreeStack(const std::vector<ParseTreeNode*>& stack) {
    std::cout << "Current parseTreeStack state:" << std::endl;
    for (auto node : stack) {
        node->printLevelOrder();
        std::cout << "---" << std::endl;  // Separator between trees
    }
}

int main() {
    std::istringstream in(WLP4_COMBINED);  // Use istringstream to treat the WLP4 string as a stream
    std::string s;

    std::vector<std::pair<std::string, std::vector<std::string>>> productionRules;

    // Should really be stacks, but printing stacks is costly (requires popping then pushing back all elements)
    std::vector<Token> unread;
    int unread_position = 0;
    std::vector<std::string> stack;  // stack

    // transitions[from_state] = [(symbol, to_state), ...]
    std::unordered_map<int, std::vector<std::pair<std::string, int>>> transitions;

    // reductions[state-number] = [(rule-number, tag), ...]
    std::unordered_map<int, std::vector<std::pair<int, std::string>>> reductions;

    std::vector<int> stateStack = {0};  // Used to keep track of our current state in the automaton

    std::vector<ParseTreeNode*> parseTreeStack;

    std::getline(in, s);  // CFG section (skip header)

    while (std::getline(in, s)) {
        if (s == ".TRANSITIONS") {
            break;
        } else if (!s.empty()) {  // Ensure the line is not empty
            // Parse the transition function. Look for first space and partition s accordingly
            std::istringstream iss(s);  // Use istringstream to treat the string as a stream

            std::string from;
            iss >> from;

            std::string token;
            std::vector<std::string> tokens;
            while (iss >> token) {
                tokens.emplace_back(token);
            }

            productionRules.emplace_back(std::make_pair(from, tokens));
        } else {
            continue;
        }
    }

    // std::cout << "Production Rules:" << std::endl;
    // for (const auto& transition : productionRules) {
    //     std::cout << transition.first << " -> [";

    //     for (const auto& to : transition.second) {
    //         std::cout << to << ",";
    //     }

    //     std::cout << "]" << std::endl;
    // }

    // .TRANSITIONS section
    while (std::getline(in, s)) {
        if (s == ".REDUCTIONS") {
            break;
        } else if (!s.empty()) {  // Ensure the line is not empty
            // Parse the transition function. Look for first space and partition s accordingly
            std::istringstream iss(s);  // Use istringstream to treat the string as a stream
            std::string symbol;
            int from_state, to_state;
            iss >> from_state;
            iss >> symbol;
            iss >> to_state;

            // Check if the from_state already exists in the map
            auto it = transitions.find(from_state);
            if (it != transitions.end()) {
                // from_state found, append the new transition to the existing vector
                it->second.emplace_back(symbol, to_state);
            } else {
                // from_state not found, create a new vector and insert it into the map
                transitions[from_state] = {{symbol, to_state}};
            }
        } else {
            continue;
        }
    }

    // std::cout << "Transitions:\n";
    // for (const auto& transition : transitions) {
    //     std::cout << "State " << transition.first << ":\n";
    //     for (const auto& symbol_state_pair : transition.second) {
    //         std::cout << "\tSymbol: " << symbol_state_pair.first << ", To State: " << symbol_state_pair.second << std::endl;
    //     }
    // }

    // .REDUCTIONS section
    while (std::getline(in, s)) {
        if (s == ".END") {
            break;
        } else if (!s.empty()) {  // Ensure the line is not empty
            // Parse the reductions. Look for first space and partition s accordingly
            std::istringstream iss(s);  // Use istringstream to treat the string as a stream
            std::string tag;
            int state_number, rule_number;
            iss >> state_number;
            iss >> rule_number;
            iss >> tag;

            // Check if the from_state already exists in the map
            auto it = reductions.find(state_number);
            if (it != reductions.end()) {
                // state_number found, append the new transition to the existing vector
                it->second.emplace_back(rule_number, tag);
            } else {
                // state_number not found, create a new vector and insert it into the map
                reductions[state_number] = {{rule_number, tag}};
            }
        } else {
            continue;
        }
    }

    // std::cout << "Reductions:\n";
    // for (const auto& reduction : reductions) {
    //     std::cout << "State " << reduction.first << ":\n";
    //     for (const auto& rule_tag_pair : reduction.second) {
    //         std::cout << "\tRule Number: " << rule_tag_pair.first << ", Tag: " << rule_tag_pair.second << std::endl;
    //     }
    // }

    // .INPUT section
    // Add BOF and EOF to the input
    unread.emplace_back("BOF", "BOF");

    while (std::getline(std::cin, s)) {
        std::istringstream iss(s);  // Use istringstream to treat the string as a stream
        std::string kind, lexeme;
        iss >> kind;
        iss >> lexeme;
        unread.emplace_back(kind, lexeme);
    }
    unread.emplace_back("EOF", "EOF");

    try {
        bool accepted = false;

        while (unread_position < unread.size() || !accepted) {
            // Print the stack, bookmark, and unread:
            // for (const auto& token : stack) {
            //     std::cout << token << " ";
            // }
            // std::cout << '.' << " ";

            // for (int i = unread_position; i < unread.size() - 1; i++) {
            //     std::cout << unread[i].kind << " ";
            // }
            // if (unread_position < unread.size()) {
            //     std::cout << unread.back().kind;  // used to fix ending space
            // }

            // std::cout << std::endl;
            // End Print

            // Go through stack to see which transition state we are in
            int currentState = stateStack.back();
            // Get the current state: DEPRECATED for stateStack
            // int currentState = 0;

            // for (std::string c : stack) {
            //     bool transitionFound = false;
            //     for (const auto& transition : transitions[currentState]) {
            //         if (transition.first == c) {  // transition.symbol
            //             currentState = transition.second;
            //             transitionFound = true;
            //             break;
            //         }
            //     }
            //     if (!transitionFound) {
            //         throw std::runtime_error("ERROR at " + std::to_string(unread_position));
            //     }
            // }
            // std::cout << "CS: " << currentState << "  SS: " << stateStack.back() << std::endl;

            std::string action = "shift";
            int ruleNumber;
            // At our state, check all rules in the state. If there is a valid reduction, take it
            for (const auto& reduction : reductions[currentState]) {
                if (unread_position < unread.size() && reduction.second == unread[unread_position].kind) {  // reduction.tag
                    action = "reduce";
                    ruleNumber = reduction.first;
                    break;
                } else if (reduction.second == ".ACCEPT") {  // reduction.tag
                    action = "reduce";
                    ruleNumber = reduction.first;
                    accepted = true;
                    break;
                }
            }

            if (action == "shift") {
                stack.emplace_back(unread[unread_position].kind);

                ParseTreeNode* newNode = new ParseTreeNode(unread[unread_position].kind + " " + unread[unread_position].lexeme);
                parseTreeStack.push_back(newNode);

                // Update stateStack
                bool transitionFound = false;
                for (const auto& transition : transitions[currentState]) {
                    if (transition.first == unread[unread_position].kind) {  // transition.symbol
                        stateStack.emplace_back(transition.second);
                        transitionFound = true;
                        break;
                    }
                }
                unread_position++;
                if (!transitionFound) {
                    throw std::runtime_error("ERROR at " + std::to_string(unread_position - 1));  // - 1 to account of BOF
                }

                // std::cout << "After shift operation:\n";
                // if (!parseTreeStack.empty()) {
                //     printParseTreeStack(parseTreeStack);  // Print the current state of parseTreeStack
                // }
                // std::cout << std::endl;  // Extra newline for readability

            } else {  // "reduce"
                std::vector<std::string> ruleRHS = productionRules[ruleNumber].second;
                std::string ruleLHS = productionRules[ruleNumber].first;

                // Update parseTreeStack
                std::vector<ParseTreeNode*> children;
                // Pop the required number of nodes from the stack
                // std::cout << "-------------- POPPING. Size  = " << ruleRHS.size() << std::endl;
                std::string productionRule = ruleLHS;
                for (const auto& rhs : ruleRHS) {
                    productionRule += " " + rhs;
                }

                // compare ruleRHS with stack from right to left. If match, pop from stack

                for (int i = ruleRHS.size() - 1; i >= 0; i--) {
                    if (ruleRHS[i] == ".EMPTY") {
                        break;
                    }

                    if (ruleRHS[i] != stack.back()) {
                        throw std::runtime_error("Can't match tokens on reduce");
                    }

                    // Before popping, add the node from parseTreeStack to children
                    // Only if the stack is not empty and the current symbol is not terminal
                    if (!parseTreeStack.empty() /* && isNonTerminal(ruleRHS[i]) */) {
                        children.insert(children.begin(), parseTreeStack.back());
                        parseTreeStack.pop_back();
                    }

                    stack.pop_back();
                    stateStack.pop_back();
                }
                ParseTreeNode* newNode = new ParseTreeNode(productionRule);
                newNode->children = children;
                parseTreeStack.push_back(newNode);
                stack.emplace_back(ruleLHS);

                for (const auto& transition : transitions[stateStack.back()]) {
                    if (transition.first == stack.back()) {  // transition.symbol
                        stateStack.emplace_back(transition.second);
                        break;
                    }
                }

                // std::cout << "After reduce operation:\n";
                // if (!parseTreeStack.empty()) {
                //     printParseTreeStack(parseTreeStack);  // Print the current state of parseTreeStack
                // }
                // std::cout << std::endl;  // Extra newline for readability
            }
        }

        // Print the stack, bookmark, and unread:
        // for (const auto& token : stack) {
        //     std::cout << token << " ";
        // }
        // std::cout << '.' << " ";

        // for (int i = unread_position; i < unread.size() - 1; i++) {
        //     std::cout << unread[i].kind << " ";
        // }
        // if (unread_position < unread.size()) {
        //     std::cout << unread.back().kind;  // used to fix ending space
        // }

        // std::cout << std::endl;
        // End Print

        if (!parseTreeStack.empty()) {
            parseTreeStack.back()->printPreOrder();  // Directly call print on the root node
            delete parseTreeStack.back();
        }

    } catch (const std::runtime_error& e) {
        // Handle runtime error
        std::cerr << e.what() << std::endl;
        if (!parseTreeStack.empty()) {
            for (auto& node : parseTreeStack) {
                delete node;
            }
            parseTreeStack.clear();
        }
        return 1;  // Return a non-zero value to indicate error
    } catch (...) {
        // Catch-all for any other exceptions
        if (!parseTreeStack.empty()) {
            for (auto& node : parseTreeStack) {
                delete node;
            }
            parseTreeStack.clear();
        }
        std::cerr << "An unexpected error occurred." << std::endl;
        return 1;
    }

    return 0;  // Indicate successful execution
}
