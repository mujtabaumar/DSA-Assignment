// texteditor.cpp
#include "texteditor.h"
#include <string>
#include <map>

// Store state for each TextEditor instance
static std::map<const TextEditor*, std::pair<std::string, std::string>> editorState;

void TextEditor::insertChar(char c) {
    // Map auto-initializes with empty strings if key doesn't exist
    editorState[this].first.push_back(c);
}

void TextEditor::deleteChar() {
    auto& state = editorState[this];
    // Delete character before cursor (only if exists)
    if (!state.first.empty()) {
        state.first.pop_back();
    }
    // Clear everything after cursor
    state.second.clear();
}

void TextEditor::moveLeft() {
    auto& state = editorState[this];
    // Move cursor left only if not at leftmost position
    if (!state.first.empty()) {
        state.second.push_back(state.first.back());
        state.first.pop_back();
    }
}

void TextEditor::moveRight() {
    auto& state = editorState[this];
    // Move cursor right only if not at rightmost position
    if (!state.second.empty()) {
        state.first.push_back(state.second.back());
        state.second.pop_back();
    }
}

std::string TextEditor::getTextWithCursor() const {
    auto it = editorState.find(this);
    if (it == editorState.end()) {
        return "|"; // Empty editor case
    }
    const auto& state = it->second;
    std::string result = state.first + "|";
    // Build right side in reverse (since stored reversed)
    for (int i = (int)state.second.size() - 1; i >= 0; --i) {
        result += state.second[i];
    }
    return result;
}