#include "LinkedTree.h"
#include "TreeAnalysis.h"

#include <iostream>
#include <sstream>
#include <string>

namespace {
bool parsePosition(const std::string& input, int& position) {
    std::istringstream stream(input);
    char extra;
    return (stream >> position) && !(stream >> extra) && position > 0;
}

void printRelationship(const char* label, const LinkedTree::Node* node) {
    std::cout << label << ": "
              << (node == nullptr ? "None" : node->getContent()) << '\n';
}

void exploreNode(const LinkedTree::Node* node) {
    std::cout << "Node content: " << node->getContent() << '\n';
    printRelationship("Parent", node->getParent());
    printRelationship("First child", node->getFirstChild());
    printRelationship("Sibling", node->getSibling());
}
}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input-file>\n";
        return 1;
    }

    LinkedTree tree;
    std::string error;
    if (!tree.loadFromFile(argv[1], error)) {
        std::cerr << "Error loading tree: " << error << '\n';
        return 1;
    }

    TreeAnalysis analysis(tree);
    if (!analysis.writeReport("about_tree.txt", error)) {
        std::cerr << "Error writing about_tree.txt: " << error << '\n';
        return 1;
    }

    while (true) {
        std::cout << "Which node would you like to explore (enter position or \"exit\"):";
        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "Goodbye!\n";
            return 0;
        }
        if (input == "exit") {
            std::cout << "Goodbye!\n";
            return 0;
        }

        int position = 0;
        const LinkedTree::Node* node =
            parsePosition(input, position) ? tree.findByPosition(position) : nullptr;
        if (node == nullptr) {
            std::cout << "Invalid input. Please try again.\n";
            continue;
        }
        exploreNode(node);
    }
}