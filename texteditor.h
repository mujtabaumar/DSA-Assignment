#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <string>

class TextEditor {
public:
    // Insert character at cursor
    void insertChar(char c);

    // Delete character before cursor
    void deleteChar();

    // Move cursor one position left
    void moveLeft();

    // Move cursor one position right
    void moveRight();

    // Return string with cursor position
    std::string getTextWithCursor() const;
};

#endif