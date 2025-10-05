// main.cpp
#include "texteditor.h"
#include <iostream>

int main() {
    TextEditor ed;
    
    ed.insertChar('a');
    std::cout << "After insert 'a': " << ed.getTextWithCursor() << std::endl;
    
    ed.insertChar('b');
    std::cout << "After insert 'b': " << ed.getTextWithCursor() << std::endl;
    
    ed.moveLeft();
    std::cout << "After move left: " << ed.getTextWithCursor() << std::endl;
    
    ed.insertChar('c');
    std::cout << "After insert 'c': " << ed.getTextWithCursor() << std::endl;
    
    ed.deleteChar();
    std::cout << "After delete: " << ed.getTextWithCursor() << std::endl;

    ed.moveLeft();
    ed.moveLeft();
    std::cout << "After move left twice: " << ed.getTextWithCursor() << std::endl;

    ed.moveRight();
    ed.moveRight();
    std::cout << "After move right twice: " << ed.getTextWithCursor() << std::endl;
    
    return 0;
}