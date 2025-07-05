#include "io.h"
#include "sockets.h"
#include "networking.h"
#include <csignal>

#define COLOR_GRAY 8

bool running = false;

//Cleans up everything in case SIGINT was called.
void CleanUp( int signal ){
    close( g_serverSocket.sockfd );
    close( g_clientSocket.sockfd );
    End_Screen();
    running = false;
    exit(0);
}

int main(int argc, char *argv[]){
    //Initialize the screen.
    Initialize_Screen();
    //Initialize the screen's subwindows.
    Initialize_SubWindows(1, g_terminalHeight-4, g_terminalWidth-18, g_terminalHeight-1,
                          g_terminalWidth - 17, 1, g_terminalWidth - 1, g_terminalHeight - 1,
                          1, 1, g_terminalWidth - 19, g_terminalHeight -  6);

    //Draws the UI.
    Draw_UI();

    //Initializes important network stuff.
    InitializeNetwork(argc, argv);
    
    //Run this in case SIGINT was called. (^C)
    std::signal(SIGINT, CleanUp);

    running = true;

    while(running){
        std::string message = Handle_Messages();
        PollMessagesClient(message);
        if( g_host ) PollMessagesServer();
    }

    End_Screen();
    return 0;
}