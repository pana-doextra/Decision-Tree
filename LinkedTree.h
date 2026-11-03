#ifndef LINKED_TREE_H
#define LINKED_TREE_H

#include <list>
#include <string>

// Owns a general linked tree. Returned node pointers are borrowed, never owned.
class LinkedTree {
public:
    class Node {
    public:
        // Nodes cannot be copied because their links belong to one tree.
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        // Return the stored text, incoming edge label, and input position.
        const std::string& getContent() const { return content_; }
        const std::string& getEdgeLabel() const { return edgeLabel_; }
        int getPosition() const { return position_; }

        // Return borrowed links; absent relationships are represented by nullptr.
        const Node* getParent() const { return parent_; }
        const std::list<Node*>& getChildren() const { return children_; }
        const Node* getFirstChild() const {
            return children_.empty() ? nullptr : children_.front();
        }
        const Node* getSibling() const;

    private:
        friend class LinkedTree;

        // Only LinkedTree creates nodes and connects their links.
        Node(const std::string& content, const std::string& edgeLabel,
             int position, Node* parent)
            : content_(content), edgeLabel_(edgeLabel), position_(position), parent_(parent) {}

        std::string content_;
        std::string edgeLabel_;
        int position_;
        Node* parent_;
        std::list<Node*> children_;
    };

    // Start empty and release all nodes on destruction.
    LinkedTree() : root_(nullptr) {}
    ~LinkedTree() { clear(); }

    // Disallow shallow ownership transfers and accidental double deletion.
    LinkedTree(const LinkedTree&) = delete;
    LinkedTree& operator=(const LinkedTree&) = delete;
    LinkedTree(LinkedTree&&) = delete;
    LinkedTree& operator=(LinkedTree&&) = delete;

    // On file/format errors, return false, set error, and preserve the old tree.
    // On success, clear error and invalidate all previously returned node pointers.
    // Allocation exceptions propagate, also preserving the old tree.
    bool loadFromFile(const std::string& filename, std::string& error);

    // Return nullptr for an empty tree or an unknown position, respectively.
    const Node* getRoot() const { return root_; }
    const Node* findByPosition(int position) const;

    // Release every node without recursion; safe to call on an empty tree.
    void clear() noexcept;

private:
    Node* root_;
};

#endif
