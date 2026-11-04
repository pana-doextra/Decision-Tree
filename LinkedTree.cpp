#include "LinkedTree.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace {
// Records are temporary. The finished tree consists of linked, allocated nodes.
struct Record {
    int level = 0, position = 1;
    std::string edge, content;
    std::size_t line = 0;
};

// Remove separators without changing spaces inside a field.
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n\v\f");
    if (first == std::string::npos) return "";
    return text.substr(first, text.find_last_not_of(" \t\r\n\v\f") - first + 1);
}

// Require a whole integer token; stream extraction also rejects int overflow.
bool readInt(const std::string& text, int& value) {
    std::istringstream input(text);
    char extra;
    return (input >> value) && !(input >> extra);
}

// Parse tabs as fields, or read three whitespace tokens followed by full content.
bool parseRecord(const std::string& line, Record& record, std::string& reason) {
    std::istringstream input(line);
    std::string level, position;
    const bool tabs = line.find('\t') != std::string::npos;
    std::vector<std::string> fields;
    if (tabs) {
        std::string field;
        while (std::getline(input, field, '\t')) fields.push_back(field);
        if (line.back() == '\t') fields.push_back("");
        if (fields.size() < 3 || fields.size() > 4) {
            reason = "expected four tab fields (three for a root)";
            return false;
        }
        level = fields[0];
        position = fields[1];
    } else {
        input >> level >> position;
    }
    if (!readInt(level, record.level) || !readInt(position, record.position)) {
        reason = "level and position must be integers within int range";
        return false;
    }
    if (tabs) {
        if (fields.size() == 3 && record.level == 0) {
            record.content = fields[2];
        } else if (fields.size() == 4) {
            record.edge = fields[2];
            record.content = fields[3];
        } else {
            reason = "a non-root record needs four tab fields";
            return false;
        }
    } else {
        if (record.level != 0) input >> record.edge;
        std::getline(input >> std::ws, record.content);
    }
    if (record.level < 0 || record.position <= 0)
        reason = "level must be nonnegative and position must be positive";
    else if (trim(record.content).empty())
        reason = "missing node content";
    else if (record.level == 0 && !trim(record.edge).empty())
        reason = "the root must not have an incoming edge label";
    else if (record.level != 0 && trim(record.edge).empty())
        reason = "missing incoming edge label";
    else {
        if (record.level == 0) record.edge.clear();
        return true;
    }
    return false;
}
} // namespace

// Return the first different child of the same parent, or nullptr.
const LinkedTree::Node* LinkedTree::Node::getSibling() const {
    if (parent_ != nullptr) {
        for (const Node* child : parent_->children_)
            if (child != this) return child;
    }
    return nullptr;
}

// Build a replacement separately so every failed load preserves the current tree.
bool LinkedTree::loadFromFile(const std::string& filename, std::string& error) {
    const auto fail = [&](std::size_t line, const std::string& message) {
        error = filename + (line ? ": line " + std::to_string(line) : "") + ": " + message;
        return false;
    };
    std::ifstream input(filename, std::ios::binary);
    if (!input) return fail(0, "could not open file");

    std::vector<Record> records;
    std::string line, firstLine;
    std::size_t lineNumber = 0, firstLineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (trim(line).empty()) continue;
        if (firstLine.empty()) {
            firstLine = line;
            firstLineNumber = lineNumber;
            continue;
        }

        Record record;
        record.line = lineNumber;
        std::string reason;
        if (!parseRecord(line, record, reason)) return fail(lineNumber, reason);
        records.push_back(std::move(record));
    }
    if (input.bad() || !input.eof()) return fail(lineNumber + 1, "could not read file");
    if (firstLine.empty()) return fail(0, "file contains no nodes");

    // Only the root marker 0 1 selects a whitespace root record. A root later
    // in the file means the first line is also a record in a shuffled input.
    std::istringstream probe(firstLine);
    std::string first, second;
    int level = 0, position = 0;
    probe >> first >> second;
    const bool rootPrefix = readInt(first, level) && level == 0
                         && readInt(second, position) && position == 1;
    const bool rootLater = std::any_of(records.begin(), records.end(), [](const Record& r) {
        return r.level == 0;
    });
    Record root;
    root.line = firstLineNumber;
    if (rootPrefix || rootLater || firstLine.find('\t') != std::string::npos) {
        std::string reason;
        if (!parseRecord(firstLine, root, reason)) return fail(firstLineNumber, reason);
    } else {
        root.content = firstLine;
    }
    records.push_back(std::move(root));

    std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.position < b.position;
    });
    for (std::size_t i = 0; i < records.size(); ++i) {
        const Record& record = records[i];
        if (i > 0 && record.position == records[i - 1].position)
            return fail(record.line, "duplicate preorder position");
        if (static_cast<std::size_t>(record.position) != i + 1)
            return fail(record.line, "preorder positions must be consecutive starting at 1");
        if (i == 0 && record.level != 0)
            return fail(record.line, "position 1 must be the root at level 0");
        if (i > 0 && record.level == 0)
            return fail(record.line, "only one root is allowed");
        if (i > 0 && static_cast<long long>(record.level) > records[i - 1].level + 1LL)
            return fail(record.line, "level increases by more than one in preorder");
    }

    LinkedTree replacement;
    std::vector<Node*> ancestors;
    for (const Record& record : records) {
        ancestors.resize(static_cast<std::size_t>(record.level));
        Node* parent = ancestors.empty() ? nullptr : ancestors.back();
        // Protect a new node until it is attached; replacement then owns it.
        std::unique_ptr<Node> node(new Node(record.content, record.edge, record.position, parent));
        if (parent == nullptr) replacement.root_ = node.get();
        else parent->children_.push_back(node.get());
        Node* attached = node.release();
        ancestors.push_back(attached);
    }
    error.clear();
    std::swap(root_, replacement.root_); // Its destructor releases the old tree.
    return true;
}

// Search through the links using an explicit stack, avoiding recursive calls.
const LinkedTree::Node* LinkedTree::findByPosition(int position) const {
    if (root_ == nullptr || position <= 0) return nullptr;
    std::vector<const Node*> pending{root_};
    while (!pending.empty()) {
        const Node* node = pending.back();
        pending.pop_back();
        if (node->position_ == position) return node;
        for (const Node* child : node->children_) pending.push_back(child);
    }
    return nullptr;
}

// Remove a child link before descending, then return through its parent link.
// This deletes a deep tree without recursion or additional allocations.
void LinkedTree::clear() noexcept {
    Node* node = root_;
    root_ = nullptr;
    while (node != nullptr) {
        if (!node->children_.empty()) {
            Node* child = node->children_.front();
            node->children_.pop_front();
            node = child;
        } else {
            Node* parent = node->parent_;
            delete node;
            node = parent;
        }
    }
}
