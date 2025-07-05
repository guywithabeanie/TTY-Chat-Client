#include "io.h"
#include <ncurses.h>

//Width and height of the terminal.
int g_terminalWidth = 0, g_terminalHeight = 0;
//Window of the message box.
static Window s_messageBox = {0};
//String on the message box.
static std::string s_messageBoxString;
//Maximum x-position per row. Used for arrow key cursor movement.
static std::vector<int> s_maxX;
//Used for displaying "Type Message..."
static bool s_printTypeMessage = false;
//Window of our members list.
static Window s_memberList = {0};
//Amount of members (written rows) in our line.
static int s_memberAmount = 0;
//Window for the chat messages.
static Window s_chatMessages = {0};
//The top of drawable area in the chat message pad, used for scrolling.
static int s_chatTopY = 0;
//Furthest row you can scroll to in the chat message pad.
static int s_maxY = 0;

void Initialize_Screen(){
    //Initialize ncurses.
    initscr();
    //Initialize colors.
    start_color();
    //Don't print inputted characters.
    noecho();
    //Hides cursor.
    curs_set(0);
    //Get width and height of the terminal.
    getmaxyx(stdscr, g_terminalHeight, g_terminalWidth);
    //Clear screen.
    clear();
}

void Initialize_SubWindows(int messageX1, int messageY1, int messageX2, int messageY2,
                           int memberX1,  int memberY1,  int memberX2,  int memberY2,
                           int chatX1,    int chatY1,    int chatX2,    int chatY2){
    //Calculate message box subwindow borders.
    s_messageBox.win = newwin( messageY2 - messageY1, messageX2 - messageX1, messageY1, messageX1 );
    //Set cursor position.
    wmove(s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX);
    //Allows special keys.
    keypad(s_messageBox.win, TRUE);
    //Get width and height of the message subwindow.
    getmaxyx(s_messageBox.win, s_messageBox.height, s_messageBox.width);
    //Non-blocking, single-thread safe.
    wtimeout(s_messageBox.win, 0);

    //Calculate member list subwindow borders.
    s_memberList.win = newwin( memberY2 - memberY1, memberX2 - memberX1, memberY1, memberX1 );
    //Get width and height of the member list subwindow.
    getmaxyx(s_memberList.win, s_memberList.height, s_memberList.width);
    
    //Creates pad ( like a window but can scroll ) for the chat box.
    s_chatMessages.win = newpad(chatX2 - chatX1, 1000);
    //Initializing values.
    s_chatMessages.width = chatX2 - chatX1;   s_chatMessages.height = chatY2 - chatY1;
    s_chatMessages.x = chatX1;                s_chatMessages.y = chatY1;
    //Activate special keys and makes it non-blocking.
    keypad(s_chatMessages.win, TRUE);
    wtimeout(s_chatMessages.win, 0);
}

void End_Screen(){
    //Delete windows.
    delwin(s_messageBox.win);
    delwin(s_memberList.win);
    delwin(s_chatMessages.win);
    //Bring back cursor.
    curs_set(1);
    //Allow for printing inputted characters
    echo();
    //fin.
    endwin();
}

//Really repetetive but hey, that's UI code for you.
void Draw_UI(){
    //Activates bold text.
    attron(A_BOLD);
    //Draws the UI.
    mvvline(1, 0, ACS_VLINE, g_terminalHeight - 2);
    mvvline(1, g_terminalWidth-1, ACS_VLINE, g_terminalHeight - 2);
    mvvline(1, g_terminalWidth-18, ACS_VLINE, g_terminalHeight - 2);
    mvhline(0, 1, ACS_HLINE, g_terminalWidth - 2);
    mvhline(g_terminalHeight - 5, 1, ACS_HLINE, g_terminalWidth - 18);
    mvhline(g_terminalHeight - 1, 1, ACS_HLINE, g_terminalWidth - 2);

    //Draws the corners.
    mvaddch(0, 0, ACS_ULCORNER);
    mvaddch(0, g_terminalWidth-1, ACS_URCORNER);
    mvaddch(g_terminalHeight-1, 0, ACS_LLCORNER);
    mvaddch(g_terminalHeight-1, g_terminalWidth-1, ACS_LRCORNER);
    //Draws intesection points.
    mvaddch(0, g_terminalWidth-18, ACS_TTEE);
    mvaddch(g_terminalHeight-5, g_terminalWidth-18, ACS_BTEE);
    mvaddch(g_terminalHeight-5, 0, ACS_LTEE);
    mvaddch(g_terminalHeight-5, g_terminalWidth-18, ACS_RTEE);
    mvaddch(g_terminalHeight-1, g_terminalWidth-18, ACS_BTEE);
    //Deactivates bold text.
    attroff(A_BOLD);

    //Print the number of clients connected.
    mvwprintw(s_memberList.win, 0, 3, "Members-1");

    //Present to the audience!
    refresh();
    wrefresh(s_memberList.win);
}

void Increase_MaxX(int row, int width, int height){
    //Can't go down anymore.
    if( row >= height ) return;
    //Increase maxX on the current row.
    s_maxX.at(row)++;
    //Increase s_maxX on the next row if the current row is bigger than the width.
    if( s_maxX.at( row ) >= width ){
        //Undo the increment on the current row.
        s_maxX.at(row)--;
        //Go to the next row
        row++;
        //Handle MaxX on that row.
        Increase_MaxX(row, width, height);
    }
}

void Decrease_MaxX(int row, int width, int height){
    //Can't go up.
    if( row >= height ) return;
    //Current row is full, check the row under it.
    if( s_maxX.at(row) == width - 1 ){
        Decrease_MaxX( row + 1, width, height);
    }
    //There's nothing below, so decrease MaxX on the current row.
    else{
        s_maxX.at(row)--;
    }
}

std::string Handle_Messages(){
    //Change cursor position.
    wmove(s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX);
    //Show cursor.
    curs_set(1);

    //Print "Type Message..." if message box is empty and if we haven't printed it yet.
    if( !s_messageBoxString.length() && !s_printTypeMessage ){
        //Dim the text.
        wattron(s_messageBox.win, A_DIM);
        mvwprintw(s_messageBox.win, 0, 0, "Type Message...");
        wattroff(s_messageBox.win, A_DIM);
        //Move cursor to it's original position.
        wmove(s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX);
        s_printTypeMessage = true;
    }
    //Remove "Type Message" if the message box has text and we haven't cleared it already
    else if( s_messageBoxString.length() > 0 && s_printTypeMessage ){
        //Clear "Type Message...".
        mvwprintw(s_messageBox.win, 0, 0, "               ");
        //Rewrite the text in the message box.
        mvwprintw(s_messageBox.win, 0, 0, "%s", s_messageBoxString.c_str());
        //Move cursor to it's orignal position.
        wmove(s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX);
        s_printTypeMessage = false;
    }

    //Initialize s_maxX if it wasn't already (uninitialized vectors are always empty).
    //Reserves enough space for all rows so we wouldn't need to realloc.
    if(s_maxX.empty()){
        s_maxX.resize( s_messageBox.height );
        //-1 so that when we increase s_maxX on it, it goes to 0 (which is the leftmost coordinate-).
        std::fill( s_maxX.begin(), s_maxX.end(), -1);
        //Except for the first line.
        s_maxX.at(0) = 0;
    }

    //Retrieve user input.
    int ch = wgetch(s_messageBox.win);
    //No input?
    if( ch == ERR ){
        return "";
    }
    //if Enter was pressed and we have text, clear the message box and return the string.
    else if( ch == '\n' && s_messageBoxString.size() > 0 ){
        //Move to 0, 0.
        s_messageBox.cursorX = 0;    s_messageBox.cursorY = 0;
        wmove( s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX );
        //Clear the message box.
        wclear(s_messageBox.win);
        //Copy the message box string temporarily.
        std::string temp = s_messageBoxString;
        //Clear the original message box string.
        s_messageBoxString.clear();
        //Return the message box string.
        return temp;
    }
    //Character is printable, so print it.
    else if( ch >= 32 && ch <= 126 ){
        //Get current cursor x/y position.
        getyx(s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX);
        //Converts 2D coordinate to 1D array index.
        int index = s_messageBox.cursorY * s_messageBox.width + s_messageBox.cursorX;
        //Inserts character to the string at the index.
        s_messageBoxString.insert(index, 1, (char) ch);
        //Go to the next X position.
        s_messageBox.cursorX++;
        //Wrap to the left border if it exceeds the messageBox width.
        wrap_right(s_messageBox);
        //Redraws all the characters in the message box starting from index to show the
        //character insertion.
        for( int i = index; i < s_messageBoxString.size(); i++ ){
            waddch( s_messageBox.win, (char)s_messageBoxString.at(i));
        }
        //waddch moves the cursor, so move the cursor back to it's original position.
        wmove(s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX);
        //Update MaxX on the current row.
        Increase_MaxX(s_messageBox.cursorY, s_messageBox.width, s_messageBox.height);
    }
    //Delete character before cursor position.
    //Some terminals send '\b' when backspace is pressed.
    else if( ch == KEY_BACKSPACE || ch == '\b' ){
        //Only works if there even is any text and we have something to delete.
        if( s_messageBoxString.length() > 0 && (s_messageBox.cursorX > 0 || s_messageBox.cursorY > 0) ){
            //Decrease cursor position.
            s_messageBox.cursorX--;
            //Wrap around and go up if we reached left boundary.
            wrap_left(s_messageBox);

            //Calculates the index.
            int index = s_messageBox.cursorY * s_messageBox.width + s_messageBox.cursorX;
            //Erase character at that position.
            s_messageBoxString.erase( index, 1 );
            //Redraws the entire message box content from the string buffer
            //Fills the rest with blanks.
            for( int i = 0; i < s_messageBox.width * s_messageBox.height; i++ ){
                int x = i % s_messageBox.width;
                int y = i / s_messageBox.width;
                if(i < s_messageBoxString.size()) mvwaddch(s_messageBox.win, y, x, (char) s_messageBoxString.at(i));
                else mvwaddch(s_messageBox.win, y, x, ' ');;
            }
            //Update cursor position.
            wmove( s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX );
            //Decrease MaxX on the current line.
            Decrease_MaxX(s_messageBox.cursorY, s_messageBox.width, s_messageBox.height);
        }
    }
    //Move cursor to the right.
    else if( ch == KEY_RIGHT ){
        //Increase cursor position.
        s_messageBox.cursorX++;
        //Wrap to the left border.
        wrap_right(s_messageBox);
        //Cursor's x-position musn't be higher than maxX.
        if( s_messageBox.cursorX > s_maxX.at(s_messageBox.cursorY) ) s_messageBox.cursorX = s_maxX.at(s_messageBox.cursorY);
        //Update cursor position.
        wmove(s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX);
    }
    //Move cursor to the left.
    else if( ch == KEY_LEFT ){
        //Decrease cursor x position.
        s_messageBox.cursorX--;
        //Wrap to the right border.
        wrap_left(s_messageBox);
        //The cursor went out-of-bounds, undo what we just did.
        if( s_messageBox.cursorY < 0 ){
            s_messageBox.cursorX++;
            wrap_right(s_messageBox);
        }
        //Update cursor position.
        wmove(s_messageBox.win, s_messageBox.cursorY, s_messageBox.cursorX);
    }
    //Scroll the chat messages up by 3 units and clamp to the top.
    else if( ch == KEY_PPAGE ){
        s_chatTopY -= 3;
        if( s_chatTopY < 0 ) s_chatTopY = 0;
    }
    //Scroll the chat messages down by 3 units and clamp to the bottom.
    else if( ch == KEY_NPAGE ){
        s_chatTopY += 3;
        if( s_chatTopY > s_maxY ) s_chatTopY = s_maxY;
    }

    //Refresh the chat messages pad.
    prefresh( s_chatMessages.win, s_chatTopY, 0, s_chatMessages.y, s_chatMessages.x,
              s_chatMessages.y + s_chatMessages.height, s_chatMessages.x + s_chatMessages.width);

    //Present the new window.
    wrefresh(s_messageBox.win);

    return "";
}

void Write_Member( short pair, int row, std::string memberName ){
    //Hide cursor.
    curs_set(0);
    //Change cursor position inside the member list.
    wmove( s_memberList.win, row, 0 );
    //Clear the line.
    wclrtoeol(s_memberList.win );
    //Activate color attribute.
    wattron(s_memberList.win, COLOR_PAIR(pair));
    //Write at most s_memberList.width characters from memberName in our line.
    wprintw(s_memberList.win, "%.*s", s_memberList.width, memberName.c_str() );
    //Disactivate color attribute.
    wattroff(s_memberList.win, COLOR_PAIR(pair));
    //Update window.
    wrefresh( s_memberList.win );
}

void Update_MemberCount(){
    wmove( s_memberList.win, 0, 0 );
    //Clear the line that contained the old member count.
    wclrtoeol( s_memberList.win );
    //Rewrite it.
    mvwprintw( s_memberList.win, 0, 3, "Members-%d", s_memberAmount);
    //Refresh
    wrefresh( s_memberList.win );
}

void Insert_Member( short color, std::string memberName ){
    //Create the color pair.
    init_pair(1, color, COLOR_BLACK );
    //Set the row to the latest row.
    wmove( s_memberList.win, s_memberAmount + 1, 0 );
    //Insert line, which pushes all lines below it down.
    winsertln( s_memberList.win );
    //Write the member name in the now empty line in row, first line is occupied by member counter so add 1.
    Write_Member( 1, s_memberAmount + 1, memberName );
    //Amount of members increased.
    s_memberAmount++;
    //Update the visible member count.
    Update_MemberCount();
}

void Remove_Member( std::string memberName ){
    //Member names are cut off after the memberList window's width.
    std::string writtenName = memberName.substr(0, s_memberList.width);
    int i;
    //Find in which row the memberName is.
    for( i = 0; i < s_memberAmount; i++ ){
        char cnameinRow[s_memberList.width + 1];
        //Gets the text on the current line, i+1 because the first line is occupied by the member count.
        mvwinnstr(s_memberList.win, i+1, 0, cnameinRow, s_memberList.width);
        //Convert it to an std::string.
        std::string nameinRow( cnameinRow );
        //Trims the string
        nameinRow.erase( nameinRow.find_last_not_of(" ") + 1 );
        //We found it, i contains the row now.
        if( nameinRow == writtenName ) break;
    }

    //Deletes the line where the name is located.
    wmove( s_memberList.win, i + 1, 0 );
    wdeleteln( s_memberList.win );

    //Decrease member list.
    s_memberAmount--;

    Update_MemberCount();

    //Refresh.
    wrefresh( s_memberList.win );
}

//Message will be formatted as "<sender> : message".
void Write_Message(std::string message, std::string sender, short color ){
    //Hide cursor
    curs_set(0);
    //Initialize color pair.
    init_pair(2, color, COLOR_BLACK);
    //Change the cursor's position.
    wmove(s_chatMessages.win, s_chatMessages.cursorY, 0);
    //Print "<"
    wprintw(s_chatMessages.win, "<");
    //Activate color pair.
    wattron(s_chatMessages.win, COLOR_PAIR(2));
    //Print the sender string so that only it will be differently colored.
    wprintw(s_chatMessages.win, "%s", sender.c_str());
    //Turn off color pair.
    wattroff(s_chatMessages.win, COLOR_PAIR(2));
    wprintw(s_chatMessages.win, "> : ");
    //Get current x position for use in the next loop.
    s_chatMessages.cursorX = getcurx( s_chatMessages.win );
    //Print the message with wrapping.
    for( int i = 0; i < message.length(); i++ ){
        mvwaddch(s_chatMessages.win, s_chatMessages.cursorY, s_chatMessages.cursorX, message.at(i));
        s_chatMessages.cursorX++;
        wrap_right(s_chatMessages);
    }
    //Get the new cursor y position..
    s_chatMessages.cursorY = getcury(s_chatMessages.win);
    //Y position points to the last line of our current message, increase it to go below it.
    s_chatMessages.cursorY++;
    s_chatMessages.cursorX = 0;
    //Update the maxY variable if the y cursor exceeds the visible height.
    if( s_chatMessages.cursorY > s_chatMessages.height ) s_maxY = s_chatMessages.cursorY - s_chatMessages.height;
    //Automatically scroll if we are at the bottom.
    if( s_chatTopY == s_maxY - 1 ) s_chatTopY = s_maxY;

    //Refresh the pad window.
    prefresh(s_chatMessages.win, s_chatTopY, 0, s_chatMessages.y, s_chatMessages.x,
             s_chatMessages.y + s_chatMessages.height, s_chatMessages.x + s_chatMessages.width);
}

void Write_Connection(std::string name, int state ){
    //Hide cursor.
    curs_set(0);
    //Create color pair.
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    //Move cursor.
    wmove( s_chatMessages.win, s_chatMessages.cursorY, 0 );
    //Print the connection message.
    wattron( s_chatMessages.win, COLOR_PAIR(3) );
    if( state == CONNECTED )    wprintw( s_chatMessages.win, "-- %s connected! --", name.c_str());
    else                        wprintw( s_chatMessages.win, "-- %s disconnected! --", name.c_str());
    wattroff( s_chatMessages.win, COLOR_PAIR(3) );
    //Get the new y position.
    s_chatMessages.cursorY = getcury( s_chatMessages.win );
    //Make cursorY point to the line below the connection message.
    s_chatMessages.cursorY++;
    //the x cursor'll always be 0 :`)
    s_chatMessages.cursorX = 0;

    //Refresh the pad.
    prefresh(s_chatMessages.win, s_chatTopY, 0, s_chatMessages.y, s_chatMessages.x, 
             s_chatMessages.y + s_chatMessages.height, s_chatMessages.x + s_chatMessages.width);
}