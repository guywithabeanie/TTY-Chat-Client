#include "sockets.h"
#include "io.h"
#include <set>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <vector>

struct fdSetGroup{
    //FD_SET containing all the active sockets.
    fd_set master;
    //FD_SET containing all socket file descriptors ready to be read from.
    fd_set readfds;
    //FD_SET contaning all socket file descriptors ready to be written to.
    fd_set writefds;
    //Furthest file descriptor, used for select().
    int maxfd;
};

//Message struct.
struct Message{
    std::string message;
    std::string sender;
};

//Global variables.
//Listening server socket (only used when hosting).
extern Socket g_serverSocket;
//User's client socket.
extern Socket g_clientSocket;
//Is the current user a host or a client?
extern bool g_host;
//Map for retrieving names from socket file descriptors..
extern std::unordered_map<int, std::string> g_sockToName;
//Retrieves Socket from file descriptor.
extern std::unordered_map<int, Socket*> g_fdtoSock;
//An archive of all the messages sent on our chatroom.
extern std::vector< Message > g_messageArchive;
//The list of members.
extern std::vector< std::string > g_membersList;
//Put all the communication sockets here just so they don't go out of scope and DIE.
//(only used by the server.)
extern std::vector <Socket> g_commVector;

//Initializes our user's sockets based on the command-line arguments.
int InitializeNetwork(int argc, char* argv[]);
//Converts message to packet and sends it.
int SendMessage( int type, Socket& socket, Message message );
//Receives packet and turns it into a message and returns the packet type.
int ReceiveMessage( Socket& socket, Message& message );
//Polls messages received to the client.
int PollMessagesClient(std::string& message);
int PollMessagesServer();