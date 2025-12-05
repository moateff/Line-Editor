#include <stdio.h>
#include <iostream>

using namespace std;

#define KEY_NULL        0
#define KEY_ESCAPE      27
#define KEY_ENTER       13
#define KEY_BACKSPACE   127

#define KEY_ARROW_LEFT  1000
#define KEY_ARROW_RIGHT 1001
#define KEY_HOME        1002
#define KEY_END         1003
#define KEY_DELETE      1004

#if defined(_WIN32)
    #include <conio.h>

    int readKeyRaw() { return _getch(); }

#else
    #include <termios.h>
    #include <unistd.h>

    void enableRawMode() {
        termios term;
        tcgetattr(STDIN_FILENO, &term);
        termios raw = term;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }

    void disableRawMode() {
        termios term;
        tcgetattr(STDIN_FILENO, &term);
        term.c_lflag |= (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }

    int readKeyRaw() {
        char c = 0;
        read(STDIN_FILENO, &c, 1);
        return c;
    }
#endif

int readKey() {
    int c = readKeyRaw();

#if defined(_WIN32)
    // Windows extended keys
    if (c == 0 || c == 224) {
        int k = readKeyRaw();
        switch (k) {
            case 75: return KEY_ARROW_LEFT;
            case 77: return KEY_ARROW_RIGHT;
            case 71: return KEY_HOME;
            case 79: return KEY_END;
            case 83: return KEY_DELETE;
            default: return KEY_NULL;
        }
    }

    if (c == 27) return KEY_ESCAPE;
    if (c == 13) return KEY_ENTER;       // Windows ENTER
    if (c == 8)  return KEY_BACKSPACE;   // Windows BACKSPACE
    return c;

#else
    // Linux escape sequences
    if (c == 27) {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) <= 0) return KEY_ESCAPE;
        if (read(STDIN_FILENO, &seq[1], 1) <= 0) return KEY_ESCAPE;

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'D': return KEY_ARROW_LEFT;
                case 'C': return KEY_ARROW_RIGHT;
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                case '3': { char x; read(STDIN_FILENO, &x, 1); return KEY_DELETE; }
            }
        }
        return KEY_ESCAPE;
    }

    if (c == '\n') return KEY_ENTER;       // Linux ENTER (LF = 10)
    if (c == 127)  return KEY_BACKSPACE;   // Linux BACKSPACE
    return c;
#endif
}


void clearScreen() {
    cout << "\033[2J\033[H";
}

void enableANSI() {
#if defined(_WIN32)
    system(" "); // enables ANSI on modern terminals
#endif
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

char* lineEditor(int x, int y, int capacity, char startRange, char endRange) {
    LineEditor editor;
    initEditor(&editor, x, y, capacity);

#if !defined(_WIN32)
    enableRawMode();
#endif

    while (true) {
        int key = readKey();
        switch(key) {
            case KEY_ARROW_LEFT: leftArrow(&editor); break;
            case KEY_ARROW_RIGHT: rightArrow(&editor); break;
            case KEY_HOME: homeKey(&editor); break;
            case KEY_END: endKey(&editor); break;
            case KEY_DELETE: deleteKey(&editor); break;
            case KEY_BACKSPACE: backspaceKey(&editor); break;
            case KEY_ENTER: /* finish editing */ goto exit_editor;
            case KEY_ESCAPE: editor.buffer[0] = '\0'; goto exit_editor;
            default: if((key >= startRange && key <= endRange)) insertKey(&editor, (char)key);
        }
    }

exit_editor:
#if !defined(_WIN32)
    disableRawMode();
#endif

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