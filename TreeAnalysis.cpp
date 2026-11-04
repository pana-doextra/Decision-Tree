#include "TreeAnalysis.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <ostream>

std::vector<const LinkedTree::Node*> TreeAnalysis::preorder() const {
    std::vector<const LinkedTree::Node*> nodes;
    const LinkedTree::Node* root = tree_.getRoot();
    if (root == nullptr) return nodes;

    // Push children in reverse so the leftmost child is processed first.
    std::vector<const LinkedTree::Node*> pending(1, root);
    while (!pending.empty()) {
        const LinkedTree::Node* node = pending.back();
        pending.pop_back();
        nodes.push_back(node);
        const std::list<LinkedTree::Node*>& children = node->getChildren();
        for (std::list<LinkedTree::Node*>::const_reverse_iterator child =
                 children.rbegin();
             child != children.rend(); ++child) {
            pending.push_back(*child);
        }
    }
    return nodes;
}

TreeAnalysis::Metrics TreeAnalysis::analyze() const {
    Metrics result;
    result.height = height();
    result.nodeCount = countNodes();
    result.internalNodeCount = countInternalNodes();
    result.leafNodeCount = countLeafNodes();
    result.binary = isBinary();
    if (result.binary) {
        result.proper = isProper();
        result.perfect = isPerfect();
        result.balanced = isBalanced();
    } else {
        // These properties are defined here only for binary trees.
        result.proper = false;
        result.perfect = false;
        result.balanced = false;
    }
    return result;
}

std::size_t TreeAnalysis::countNodes() const {
    return preorder().size();
}

std::size_t TreeAnalysis::countInternalNodes() const {
    std::size_t count = 0;
    for (const LinkedTree::Node* node : preorder())
        if (!node->getChildren().empty()) ++count;
    return count;
}

std::size_t TreeAnalysis::countLeafNodes() const {
    std::size_t count = 0;
    for (const LinkedTree::Node* node : preorder())
        if (node->getChildren().empty()) ++count;
    return count;
}

int TreeAnalysis::height() const {
    return subtreeHeight(tree_.getRoot());
}

bool TreeAnalysis::isBinary() const {
    for (const LinkedTree::Node* node : preorder())
        if (node->getChildren().size() > 2) return false;
    return true;
}

bool TreeAnalysis::isProper() const {
    return isBinary() && proper(tree_.getRoot());
}

bool TreeAnalysis::isPerfect() const {
    if (!isBinary()) return false;
    int leafLevel = -1;
    return perfect(tree_.getRoot(), 0, leafLevel);
}

bool TreeAnalysis::isBalanced() const {
    return isBinary() && balanced(tree_.getRoot());
}

int TreeAnalysis::subtreeHeight(const LinkedTree::Node* node) {
    if (node == nullptr) return -1;
    // Returning -1 for no child makes a leaf's height equal to zero edges.
    int result = -1;
    for (const LinkedTree::Node* child : node->getChildren())
        result = std::max(result, subtreeHeight(child));
    return result + 1;
}

bool TreeAnalysis::balanced(const LinkedTree::Node* node) {
    if (node == nullptr) return true;
    const std::list<LinkedTree::Node*>& children = node->getChildren();
    if (children.size() > 2) return false;

    int firstHeight = children.empty() ? -1 : subtreeHeight(children.front());
    int secondHeight = -1;
    if (children.size() == 2) {
        // The second height is needed only when this node has two children.
        std::list<LinkedTree::Node*>::const_iterator second = children.begin();
        ++second;
        secondHeight = subtreeHeight(*second);
    }
    if (std::abs(firstHeight - secondHeight) > 1) return false;
    for (const LinkedTree::Node* child : children)
        if (!balanced(child)) return false;
    return true;
}

bool TreeAnalysis::proper(const LinkedTree::Node* node) {
    if (node == nullptr) return true;
    const std::size_t childCount = node->getChildren().size();
    if (childCount != 0 && childCount != 2) return false;
    for (const LinkedTree::Node* child : node->getChildren())
        if (!proper(child)) return false;
    return true;
}

bool TreeAnalysis::perfect(const LinkedTree::Node* node, int level,
                           int& leafLevel) {
    if (node == nullptr) return true;
    const std::size_t childCount = node->getChildren().size();
    if (childCount == 0) {
        // Every leaf must match the level of the first leaf encountered.
        if (leafLevel == -1) leafLevel = level;
        return leafLevel == level;
    }
    if (childCount != 2) return false;
    for (const LinkedTree::Node* child : node->getChildren())
        if (!perfect(child, level + 1, leafLevel)) return false;
    return true;
}

void TreeAnalysis::writeTree(const LinkedTree::Node* node, int level,
                             std::ostream& output) {
    if (node == nullptr) return;
    // Each level contributes exactly two dashes to the report indentation.
    for (int i = 0; i < level; ++i) output << "--";
    if (level > 0) output << "[" << node->getEdgeLabel() << "] ";
    output << node->getContent() << '\n';
    for (const LinkedTree::Node* child : node->getChildren())
        writeTree(child, level + 1, output);
}

void TreeAnalysis::writeNodeList(
    const std::vector<const LinkedTree::Node*>& nodes, std::ostream& output) {
    if (nodes.empty()) {
        output << "None\n";
        return;
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (i != 0) output << ", ";
        output << nodes[i]->getContent();
    }
    output << '\n';
}

void TreeAnalysis::writeReport(std::ostream& output) const {
    const Metrics result = analyze();
    const std::vector<const LinkedTree::Node*> nodes = preorder();
    std::vector<const LinkedTree::Node*> internal;
    std::vector<const LinkedTree::Node*> leaves;
    for (const LinkedTree::Node* node : nodes) {
        if (node->getChildren().empty()) leaves.push_back(node);
        else internal.push_back(node);
    }

    output << "Tree:\n";
    writeTree(tree_.getRoot(), 0, output);
    output << "Root: "
           << (tree_.getRoot() == nullptr ? "None"
                                           : tree_.getRoot()->getContent())
           << '\n';
    output << "Height: " << result.height << '\n';
    output << "Internal node count: " << result.internalNodeCount << '\n';
    output << "Leaf node count: " << result.leafNodeCount << '\n';
    output << "Internal nodes: ";
    writeNodeList(internal, output);
    output << "Leaf nodes: ";
    writeNodeList(leaves, output);
    output << "Binary: " << (result.binary ? "Yes" : "No") << '\n';
    if (result.binary) {
        output << "Proper: " << (result.proper ? "Yes" : "No") << '\n';
        output << "Perfect: " << (result.perfect ? "Yes" : "No") << '\n';
        output << "Balanced: " << (result.balanced ? "Yes" : "No") << '\n';
    } else {
        output << "Proper: N/A\n";
        output << "Perfect: N/A\n";
        output << "Balanced: N/A\n";
    }
}

bool TreeAnalysis::writeReport(const std::string& filename,
                               std::string& error) const {
    std::ofstream output(filename);
    if (!output) {
        error = "could not open output file: " + filename;
        return false;
    }
    writeReport(output);
    if (!output) {
        error = "could not write output file: " + filename;
        return false;
    }
    error.clear();
    return true;
}
