#ifndef TREE_ANALYSIS_H
#define TREE_ANALYSIS_H

#include "LinkedTree.h"

#include <iosfwd>
#include <string>
#include <vector>

class TreeAnalysis {
public:
    struct Metrics {
        int height = -1;
        std::size_t nodeCount = 0;
        std::size_t internalNodeCount = 0;
        std::size_t leafNodeCount = 0;
        bool binary = true;
        bool proper = true;
        bool perfect = true;
        bool balanced = true;
    };

    explicit TreeAnalysis(const LinkedTree& tree) : tree_(tree) {}

    std::vector<const LinkedTree::Node*> preorder() const;
    Metrics analyze() const;

    std::size_t countNodes() const;
    std::size_t countInternalNodes() const;
    std::size_t countLeafNodes() const;
    int height() const;
    bool isBinary() const;
    bool isProper() const;
    bool isPerfect() const;
    bool isBalanced() const;

    // Write the complete analysis report. Returns false and sets error on failure.
    bool writeReport(const std::string& filename, std::string& error) const;
    void writeReport(std::ostream& output) const;

private:
    const LinkedTree& tree_;

    static int subtreeHeight(const LinkedTree::Node* node);
    static bool balanced(const LinkedTree::Node* node);
    static bool proper(const LinkedTree::Node* node);
    static bool perfect(const LinkedTree::Node* node, int level, int& leafLevel);
    static void writeTree(const LinkedTree::Node* node, int level,
                          std::ostream& output);
    static void writeNodeList(const std::vector<const LinkedTree::Node*>& nodes,
                              std::ostream& output);
};

#endif