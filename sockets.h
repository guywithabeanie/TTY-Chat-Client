//Handles the sockets.
//Sockets are also blocking but that shouldn't be too much of an issues for what i'm trying to do here.
#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <map>

//Macros.

#define CLIENT 0
#define SERVER 1

//Packet type when a user first joins, sender contains their name.
#define CONNECT_PACKET 2
//Packet type when a message is sent, message contains the message and sender contains the sender.
#define MESSAGE_PACKET 3
//Packet type when a user disconnects, sender contains their name.
#define DISCONNECT_PACKET 4

//Return values for the socket functions.
#define RESULT_OK 5
#define RESULT_DISCONNECTED 6
#define RESULT_ERROR 7
#define RESULT_SLEEP 8


//Header of the packet.
struct PacketHeader{
    //What kind of packet is it?
    uint8_t packetType;
    //Size of the message data in the payload.
    uint16_t messageSize;
    //Size of the name in the payload.
    uint16_t nameSize;
};

//The data send / received by the sockets.
struct Packet{
    //Packet header.
    PacketHeader header;
    //Payload.

    //Message data.
    std::string message;
    //Name of the sender.
    std::string sender;
};

class Socket{
    public:
        //Creates a socket.
        Socket(int socketmode, int socket_type, int port, const char* address = "localhost");
        //Default constructor.
        Socket() = default;
        //Creates a socket from a file descriptor.
        Socket( int fd, int socketmode = CLIENT );
        //Unmakes a socket.
        ~Socket();
        //Get rid of the copy constructor and copy assignment operators.
        //Every socket has it's own unique socket file descriptor, so copying will only cause trouble.
        //Moving is more appropirate.
        Socket(const Socket& other) = delete;
        Socket& operator=(const Socket&) = delete;
        
        //Move constructor.
        Socket(Socket&& other) noexcept {
            sockfd = other.sockfd;
            socketmode = other.socketmode;
            other.sockfd = -1;
            other.socketmode = -1;
        }
        //Move assignement operator.
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                if (sockfd >= 0) close(sockfd);
                sockfd = other.sockfd;
                socketmode = other.socketmode;
                other.sockfd = -1;
                other.socketmode = -1;
            }
            return *this;
        }

        //Start listening to clients (only servers can do this).
        void listen();
        //Accept connection of a socket and puts it on commSocket.
        int accept(Socket& commSocket);
        //Send packet to socket, returns RESULT_DISCONNECTED if the other socket disconnected.
        int send( Packet& packet );
        //Receives packet from another socket, return RESULT_DISONNECTED if...go figure.
        int receive( Packet& outPacket );
        //Socket file descriptor.
        int sockfd = -1;
    private:
        //Is the socket a Client or a Server?
        int socketmode = SERVER;
};