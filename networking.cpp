#include "networking.h"
#include "io.h"
#include "sockets.h"
#include <cstring>
#include <ncurses.h>
#include <ostream>
#include <sys/select.h>
#include <vector>

//Globals, defined in networking.h
Socket g_serverSocket, g_clientSocket;
bool g_host = false;
std::unordered_map<int, std::string> g_socktoName;
std::unordered_map<int, Socket*> g_fdtoSock;
std::vector<Socket> g_commVector;
std::vector<Message> g_messageArchive;
std::vector<std::string> g_memberList;

//Statics
//fdSets for the server, only used when hosting.
static fdSetGroup s_serverfdSets;
//fdSets for the client, always used.
static fdSetGroup s_clientfdSets;
//Name of the user, we'll use this soon.
static std::string s_name;

int InitializeNetwork(int argc, char* argv[]){
    //Reserve some space to lower amount of re-allocations.
    g_commVector.reserve(2);

    //Clear all fd_sets for the client.
    FD_ZERO( &s_clientfdSets.master );
    FD_ZERO( &s_clientfdSets.readfds );
    FD_ZERO( &s_clientfdSets.writefds );

    //Goes through all command-line arguments.
    for( int i = 1; i < argc; i++ ){
        //We're hosting!
        if( !strcmp( argv[i], "--host") ){
            //Creates the listening server socket.
            g_serverSocket = Socket( SERVER, SOCK_STREAM, 6969 );
            g_fdtoSock[ g_serverSocket.sockfd ] = &g_serverSocket;
            //Creates the client socket( the one that'll send / receive on our end ).
            g_clientSocket = Socket( CLIENT, SOCK_STREAM, 6969);
            g_fdtoSock[ g_clientSocket.sockfd ] = &g_clientSocket;
            //Communication socket.
            Socket commSocket;
            //Get file descriptor of the socket.
            if( g_serverSocket.accept( commSocket ) == RESULT_OK){
                g_fdtoSock[ commSocket.sockfd ] = &commSocket;
                //Move the socket to avoid copying.
                g_commVector.push_back( std::move( commSocket ) );
            }
            //Yes, current user is a host.
            g_host = true;
        }
        //We're joining!
        else if( !strcmp( argv[i], "--join") ){
            //Argument next to --join must be structured like "address:port"
            std::string socketAddress(argv[i+1]);
            //Gets index before where we first find a colon.
            size_t colonPos = socketAddress.find(":");
            //Couldn't find a colon </3
            if( colonPos == std::string::npos ){
                End_Screen();
                std::cerr << "Error! --join must be followed by address and port structured in the format address:port!" << std::endl;
                exit(1);
            }
            //Extract address.
            std::string address = socketAddress.substr(0, colonPos);
            //Extract port number.
            int port = std::stoi(socketAddress.substr(colonPos+1));
            //Creates socket with this new-found information.
            g_clientSocket = Socket( CLIENT, SOCK_STREAM, port, address.c_str());
            //Skips next command-line argument because we have already processed it.
            i++;
        }
        //Name.
        else if( !strcmp( argv[i], "--name") ){
            //Say my name...
            s_name = std::string( argv[i+1] );
            i++;
        }
    }

    //--name wasn't in the command-line arguments, give it a default name.
    if( s_name.empty() ){
        s_name = "Mingebag";
    }

    //Put the name on the table.
    g_socktoName[ g_clientSocket.sockfd ] = s_name;

    //Initialize server's fd_sets, put the server socket and communication socket on the master set.
    //Make the maxfd the bigger socket file descriptor.
    if( g_host ){
        FD_ZERO( &s_serverfdSets.master );
        FD_ZERO( &s_serverfdSets.readfds );
        FD_ZERO( &s_serverfdSets.writefds );

        FD_SET( g_serverSocket.sockfd, &s_serverfdSets.master );
        FD_SET( g_commVector.at(0).sockfd, &s_serverfdSets.master );
        s_serverfdSets.maxfd = std::max( g_serverSocket.sockfd, g_commVector.at(0).sockfd );
    }

    //Client socket master list must only have the client socket.
    FD_SET( g_clientSocket.sockfd, &s_clientfdSets.master );
    //There's only one file descriptor anyways...
    s_clientfdSets.maxfd = g_clientSocket.sockfd;

    //We don't really need to send anything as a client if we are hosting.
    if( g_host ) {
        //Put the name on the member list.
        g_memberList.push_back( s_name );
        //Host gets special treatement!
        Insert_Member( (g_host) ? COLOR_YELLOW : COLOR_WHITE, s_name );
    }
    else SendMessage( CONNECT_PACKET , g_clientSocket, {"", s_name });

    return RESULT_OK;
}

int SendMessage( int type, Socket& socket, Message message ){
    Packet sentMessage = {0};

    //Turns the message into a sendable packet.
    sentMessage.header.packetType = type;
    sentMessage.header.messageSize = message.message.size();
    sentMessage.header.nameSize = message.sender.size();
    sentMessage.message = message.message;
    sentMessage.sender = message.sender;
    
    //Send packet!
    return socket.send(sentMessage);
}

int ReceiveMessage( Socket& socket, Message& message ){
    Packet receivedPacket = {0};
    //Receives packet.
    int status = socket.receive(receivedPacket);
    //Socket reception went well.
    if( status == RESULT_OK || status == RESULT_DISCONNECTED ){
        message.message = receivedPacket.message;
        message.sender = receivedPacket.sender;
        //Returns the packet type.
        return (status == RESULT_DISCONNECTED) ? RESULT_DISCONNECTED : receivedPacket.header.packetType;
    }
    //Something wrong happened..
    return status;
}

int PollMessagesClient(std::string& message){
    //select() overrides it's arguments and we don't want that, so we copy.
    s_clientfdSets.readfds = s_clientfdSets.master;
    s_clientfdSets.writefds = s_clientfdSets.master;

    //timeval of 0 seconds make select() non blocking.
    struct timeval timeout = {0, 0};

    //Monitors active socket file descriptors and checks if they are ready to read / write.
    int ready = select( s_clientfdSets.maxfd + 1, &s_clientfdSets.readfds, &s_clientfdSets.writefds, nullptr, &timeout );
    //There's nothing, so skip. 
    if( ready == 0 ){
        return RESULT_SLEEP;
    }
    //There's an error creeping around here!
    else if( ready == ERR ){
        return RESULT_ERROR;
    }

    //Now we're talking!
    //Socket wants to read ( aka recv() ).
    if( FD_ISSET( g_clientSocket.sockfd, &s_clientfdSets.readfds ) ){
        Message receivedMessage;

        int packet = ReceiveMessage(g_clientSocket, receivedMessage);
        switch( packet ){
            case CONNECT_PACKET :
                Insert_Member(COLOR_WHITE, receivedMessage.sender);
                Write_Connection(receivedMessage.sender, CONNECTED );
                break;
            case MESSAGE_PACKET :
                Write_Message( receivedMessage.message, receivedMessage.sender, COLOR_WHITE);
                break;
            case DISCONNECT_PACKET:
                Remove_Member( receivedMessage.sender );
                Write_Connection(receivedMessage.sender, DISCONNECTED );
                break;
            case RESULT_DISCONNECTED:
                End_Screen();
                printf("The host has disconnected, thank's for using this.\n");
                exit(0);
                break;
            //Error.
            default :
                std::cerr << "Packet reception failed : " << strerror(errno) << std::endl;
                break;
        }
    }
    //Socket is ready to write ( aka send() ).
    if( FD_ISSET( g_clientSocket.sockfd, &s_clientfdSets.writefds ) ){
        //Only send when we actually have a message to send.
        if( message != ""){
            SendMessage(MESSAGE_PACKET, g_clientSocket, {message, s_name});
        }
    }
    return RESULT_OK;
}

int PollMessagesServer(){
    //select() overrides, so copy the master value.
    s_serverfdSets.readfds = s_serverfdSets.master;
    s_serverfdSets.writefds = s_serverfdSets.master;

    //timeval of 0 seconds make select() non-blocking.
    struct timeval timeout = {0, 50};

    int ready = select( s_serverfdSets.maxfd + 1, &s_serverfdSets.readfds, &s_serverfdSets.writefds, nullptr, &timeout);
    //Nothing..
    if( ready == 0 ) return RESULT_SLEEP;
    //Error!
    else if( ready == ERR ) return RESULT_ERROR;

        //If the serverSocket wants to read, that means a client is trying to connect to it.
        //Accept.
        if( FD_ISSET(g_serverSocket.sockfd, &s_serverfdSets.readfds) ){
            //Communication socket.
            Socket commSocket;
            //Plug commSocket into accept().
            g_serverSocket.accept( commSocket );
            //Update the max file descriptor.
            s_serverfdSets.maxfd = std::max( s_serverfdSets.maxfd, commSocket.sockfd );
            //Move it to the vector or else it will be automatically destroyed.
            g_commVector.push_back( std::move(commSocket) );
            //Add it to our master set.
            FD_SET( g_commVector.back().sockfd, &s_serverfdSets.master );

            //Send to the client the member list for them to print.
            for( std::string s : g_memberList ) SendMessage(CONNECT_PACKET, g_commVector.back(), {"", s});
            //Send to the client all the messages for them to print.
            for( Message m : g_messageArchive ) SendMessage( MESSAGE_PACKET, g_commVector.back(), m);
        }

    //Loop through all the active communication sockets and check if they wanna read.
    for( auto i = g_commVector.begin() ; i != g_commVector.end(); ){
        int packetType;
        if( FD_ISSET(i->sockfd, &s_serverfdSets.readfds) ){
            Message receivedMessage;
            //Receive the message from the socket and send it to all communication sockets.
            packetType = ReceiveMessage(*i, receivedMessage);
            //Put the message on the message archive.
            switch( packetType ) {
                //Received a message.
                case MESSAGE_PACKET:
                    g_messageArchive.push_back( receivedMessage );
                    //Broad cast message to all the communication sockets, which in turn will send to the clients.
                    for( Socket& j : g_commVector ) SendMessage(packetType, j, receivedMessage );
                    break;

                //We received a client's name.
                case CONNECT_PACKET:
                    g_memberList.push_back( receivedMessage.sender );
                    g_socktoName[ i->sockfd ] = receivedMessage.sender;
                    //Broad cast message to all the communication sockets, which in turn will send to the clients.
                    for( Socket& j : g_commVector ) SendMessage(packetType, j, receivedMessage );
                    break;

                //Someone disconnected.
                case RESULT_DISCONNECTED: {
                    std::string& disconnectedName = g_socktoName[i->sockfd];
                    //Gets iterator from g_memberList containing the name of the disconnected socket.
                    auto name = std::find( g_memberList.begin(), g_memberList.end(), disconnectedName);
                    //Erase from the list.
                    g_memberList.erase( name );
                    FD_CLR( i->sockfd, &s_serverfdSets.master);
                    i = g_commVector.erase(i);
                    for( Socket& j : g_commVector ) SendMessage(DISCONNECT_PACKET, j, {"", disconnectedName} );
                    break;
                }

                //Error.
                case RESULT_ERROR:
                    std::cerr << "Error sending packet : " << strerror(errno) << std::endl;
                    break;
            }
        }
        //RESULT_DISCONNECTED already handles interator advancement.
        if( packetType != RESULT_DISCONNECTED) i++;
    }
    return RESULT_OK;
}