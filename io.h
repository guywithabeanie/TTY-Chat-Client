//Handles printing and user input.
#pragma once
#include <string>
#include <ncurses.h>
#include <vector>

#define CONNECTED 0
#define DISCONNECTED 1

//Macro for wrapping cursor to the left when it exceeds the right border in a specific window.
#define wrap_right(window) (( window.cursorX > window.width-1 ) ? (( window.cursorY+1 < window.height) ? ( window.cursorX = 0, window.cursorY++) : (window.cursorX--)) : 0)
//Macro for wrapping cursor the right when it exceeds the left border in a specific window.
#define wrap_left(window) (( window.cursorX < 0 ) ? (( window.cursorY >= 0) ? ( window.cursorX = window.width-1, window.cursorY--) : (window.cursorX++)) : 0)

struct Window{
    //NCurses WINDOW struct.
    WINDOW* win;
    //Width/Height of the window.
    int width, height;
    //Self-explanatory.
    int cursorX, cursorY;
    //Position of the top left corner in relation to the terminal.
    int x, y;
};

//Width and height of the terminal.
extern int g_terminalWidth, g_terminalHeight;

//Initializes NCurses and sets it up.
void Initialize_Screen();
//Initializes the screen's subwindows (message box subwindow, member list subwindow and chat messages subwindow).
void Initialize_SubWindows(int messageX1, int messageY1, int messageX2, int messageY2,
                           int memberX1,  int memberY1  ,int memberX2 , int memberY2 ,
                           int chatX1  ,  int chatY1,    int chatX2,    int chatY2);
//Ends NCurses and frees memory.
void End_Screen();
//Draws the interface.
void Draw_UI();
//Increases MaxX's value on a specific line, if the current line is full it increases MaxX's value for the
//MaxX's value for the next line, and if that line is full it increase MaxX's value for the line
//below it...etc.
void Increase_MaxX(int row, int width, int height);
//Decreases MaxX's value on a specific line, if the current line is full it decrease MaX's
//value for the next line if that line isn't empty.
void Decrease_MaxX(int row, int width, int height);
//Handle user input, this includes :
// * Writing text for the message box, returns the text in it if ENTER is pressed.
// * Pressing PgUP and PgDOWN to scroll the chat box.
std::string Handle_Messages();
//Writes a member into a row in our member list.
void Write_Member( short pair, int row, std::string memberName );
//Updates the member counter.
void Update_MemberCount();
//Inserts a member.
void Insert_Member( short color, std::string memberName );
//Finds and removes a member.
void Remove_Member( std::string memberName );
//Writes a chat message sent by the sender on the chat message window.
void Write_Message( std::string message, std::string sender, short color );
//Writes the name of the new connected / disconnected user into the chat box.
void Write_Connection( std::string name, int state );