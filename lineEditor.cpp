#include <stdio.h>
#include <iostream>
#include <conio.h>
#include <windows.h>

#define KEY_NULL        0
#define KEY_ESCAPE      27
#define KEY_ENTER       13
#define KEY_BACKSPACE   8
#define KEY_ARROW_LEFT  75
#define KEY_ARROW_RIGHT 77
#define KEY_DELETE      83
#define KEY_HOME        71
#define KEY_END         79

using namespace std;

void enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

void clearScreen() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count, cellCount;
    COORD homeCoords = {0, 0};

    if (hConsole == INVALID_HANDLE_VALUE) return;

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    FillConsoleOutputCharacter(hConsole, (TCHAR)' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);

    SetConsoleCursorPosition(hConsole, homeCoords);
}

struct LineEditor {
    char* buffer;
    int capacity;
    int length;
    int cursor;
    int x;
    int y;
};

void initEditor(LineEditor* editor, int x, int y,int initialCapacity = 16) {
    editor->buffer = (char *)malloc(initialCapacity + 1);
    editor->capacity = initialCapacity;
    editor->length = 0;
    editor->cursor = 0;
    editor->x = x;
    editor->y = y;
    editor->buffer[0] = '\0';

    cout << "\033[" << (editor->y + 1) << ";" << (editor->x + 1) << "H";
}

void freeBuffer(LineEditor* editor) {
    delete[] editor->buffer;   
    editor->buffer = nullptr;    
}

int isFull(LineEditor* editor) {
    return editor->length >= editor->capacity;
}

int isEmpty(LineEditor* editor) {
    return editor->length == 0;
}

int insertKey(LineEditor* editor, char key){
    if (isFull(editor)) return 0;

    // Shift right
    for (int i = editor->length; i > editor->cursor; i--) {
        editor->buffer[i] = editor->buffer[i - 1];
    }

    // Insert
    editor->buffer[editor->cursor] = key;
    editor->cursor++;
    editor->length++;
    editor->buffer[editor->length] = '\0';

    cout << "\033[s";       // save cursor
    cout << "\033[K";       // clear to end
    cout << editor->buffer + editor->cursor - 1; 
    cout << "\033[" << (editor->y + 1) << ";" << (editor->x + editor->cursor + 1) << "H";
    cout.flush();

    return 1;
}

int removeKey(LineEditor* editor, int position) {
    if (position < 0 || position >= editor->length) return 0;

    // Shift everything left
    for (int i = position; i < editor->length - 1; i++) {
        editor->buffer[i] = editor->buffer[i + 1];
    }

    editor->length--;
    editor->buffer[editor->length] = '\0';

    return 1;
}

int backspaceKey(LineEditor* editor) {
    if (editor->cursor == 0) return 0;

    editor->cursor--;
    removeKey(editor, editor->cursor);

    cout << "\033[D"; // move left
    cout << "\033[s";
    cout << "\033[K";
    cout << editor->buffer + editor->cursor;
    cout << "\033[" << (editor->y + 1) << ";" << (editor->x + editor->cursor + 1) << "H";
    cout.flush();

    return 1;
}

int deleteKey(LineEditor* editor) {
    if (editor->cursor >= editor->length) return 0;

    removeKey(editor, editor->cursor);

    cout << "\033[s";
    cout << "\033[K";
    cout << editor->buffer + editor->cursor;
    cout << "\033[" << (editor->y + 1) << ";" << (editor->x + editor->cursor + 1) << "H";
    cout.flush();

    return 1;
}

void leftArrow(LineEditor* editor){
    if (editor->cursor > 0) {
        editor->cursor--;

        cout << "\033[D";
        cout.flush();
    }  
}

void rightArrow(LineEditor* editor){
    if (editor->cursor < editor->length) {
        editor->cursor++;

        cout << "\033[C";
        cout.flush();
    }
}

void homeKey(LineEditor* editor){
    editor->cursor = 0;
    cout << "\033[" << (editor->y + 1) << ";" << (editor->x + editor->cursor + 1) << "H";
}

void endKey(LineEditor* editor){
    editor->cursor = editor->length;
    cout << "\033[" << (editor->y + 1) << ";" << (editor->x + editor->cursor + 1) << "H";
}
int readKey() {
    int key = _getch();

    if (key == -32) { // special keys prefix
        key = _getch();
        switch (key) {
            case 75: return KEY_ARROW_LEFT;  // Left arrow
            case 77: return KEY_ARROW_RIGHT; // Right arrow
            case 71: return KEY_HOME;        // Home
            case 79: return KEY_END;         // End
            case 83: return KEY_DELETE;      // Delete
            default: return KEY_NULL;
        }
    } else if (key == KEY_ESCAPE) {
        return KEY_ESCAPE; // Escape
    } else if (key == 13) {
        return KEY_ENTER;  // Enter
    } else if (key == 8) {
        return KEY_BACKSPACE;          // Backspace
    } else {
        return key;        // normal key
    }
}

char* lineEditor(int x, int y, int capacity, char startRange, char endRange) {
    LineEditor editor;
    initEditor(&editor, x, y);

    while (true) {
        int key = readKey();
        switch(key) {
            case KEY_ARROW_LEFT: leftArrow(&editor); break;
            case KEY_ARROW_RIGHT: rightArrow(&editor); break;
            case KEY_HOME: homeKey(&editor); break;
            case KEY_END: endKey(&editor); break;
            case KEY_DELETE: deleteKey(&editor); break;
            case KEY_BACKSPACE: backspaceKey(&editor); break;
            case KEY_ENTER: /* finish editing */ break;
            case KEY_ESCAPE: editor.buffer[0] = '\0'; break;
            default: if((key >= startRange && key <= endRange)) insertKey(&editor, (char)key);
        }
        if (key == KEY_ENTER || key == KEY_ESCAPE) break;
    }

    return editor.buffer;
}

int main() {
    enableANSI();
    clearScreen();
    
    int x = 45;
    int y = 10;
    int capacity = 25;
    char startRange = 'a';
    char endRange = 'z';

    char* str = lineEditor(x, y, capacity, startRange, endRange);

    clearScreen();

    if (str[0] != '\0') 
        cout << "You entered: " << str << endl;
    else 
        cout << "Empty input." << endl;

    delete[] str; 

    return 0;
}