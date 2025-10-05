#include "texteditor.h"
#include <string>
#include <map>
using namespace std;

// map to store text state for each editor (left and right of cursor)
static map<const TextEditor*, pair<string, string>> editorState;

void TextEditor::insertChar(char c) {
    // if editor doesn't exist yet in map, it's auto-added
    editorState[this].first.push_back(c);
}

void TextEditor::deleteChar() {
    auto& state = editorState[this];
    // delete the character before the cursor (if any)
    if (!state.first.empty()) {
        state.first.pop_back();
    }
    // also clear the right side since cursor moves
    state.second.clear();
}

void TextEditor::moveLeft() {
    auto& state = editorState[this];
    // move cursor left (only if there's something on left)
    if (!state.first.empty()) {
        state.second.push_back(state.first.back());
        state.first.pop_back();
    }
}

void TextEditor::moveRight() {
    auto& state = editorState[this];
    // move cursor right (only if there's something on right)
    if (!state.second.empty()) {
        state.first.push_back(state.second.back());
        state.second.pop_back();
    }
}

string TextEditor::getTextWithCursor() const {
    auto it = editorState.find(this);
    if (it == editorState.end()) {
        return "|"; // empty editor, just cursor
    }
    const auto& state = it->second;
    string result = state.first + "|";
    // rebuild right side (stored reversed internally)
    for (int i = (int)state.second.size() - 1; i >= 0; --i) {
        result += state.second[i];
    }
    return result;
}