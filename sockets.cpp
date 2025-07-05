#include "sockets.h"
#include "io.h"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>

Socket::Socket(int socketmode, int socket_type, int port, const char* address){
    //Remember if the socket is a client or server.
    this->socketmode = socketmode;
    //Creates the socket, returns a socket file descriptors. ( AF_INET = IPv4 ).
    this->sockfd = socket(AF_INET, socket_type, 0);
    //Couldn't create socket..
    if( sockfd == ERR ){
        End_Screen();
        std::cerr << "Couldn't create socket : " << strerror(errno) << std::endl;
        exit(1);
    }
    //IPv4 Address+Port definition.
    sockaddr_in socketAddress = {0};
    socketAddress.sin_family = AF_INET;
    //The port must be serialized.
    socketAddress.sin_port = htons(port);
    //inet_addr doesn't recognize "localhost" so i have to convert it to INADDR_ANY / 127.0.0.1.
    socketAddress.sin_addr.s_addr = (!strcmp(address, "localhost")) 
                                    ? ( socketmode == SERVER ) 
                                        ? INADDR_ANY : inet_addr("127.0.0.1")
                                    : inet_addr(address);

    //We'll use this later for error checking.
    int bindReturn;
    //Binds the socket to the address and port, have to convert sockaddr_in to
    //the more generic all-purpose sockaddr.
    if( socketmode == SERVER ){
        int opt = 1;
        //Allows up to reuse the address whenever the socket is closed.
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        //Binds the socket.
        bindReturn = bind( sockfd, (sockaddr*)&socketAddress, 
                                            sizeof(socketAddress));
    }
    //Connects to the socket binded to the address and port.
    else{
        bindReturn = connect(sockfd, (sockaddr*)&socketAddress, sizeof(socketAddress));
    }

    //Error checking
    if( bindReturn < 0 ){
        std::string error = (socketmode == CLIENT) ? "Failed to connect socket to address : "
                                                   : "Couldn't bind socket to address : ";
        End_Screen();
        std::cerr << error << strerror(errno) << std::endl;
        exit(1);
    }

    //If we're a server start listening.
    if( socketmode == SERVER ) this->listen();
}

//Not much to say here.
Socket::Socket( int fd, int socketmode ) : socketmode(socketmode), sockfd(fd) {}

void Socket::listen(){
    //Use :: or else it'll think we're referring to Socket::listen.
    //Max Socket Count on the backlog cuz...better safe than sorry?
    if(socketmode == SERVER) {
        int listen = ::listen(sockfd, SOMAXCONN);
        //Error checking.
        if( listen == ERR ) {
            End_Screen();
            std::cerr << "Couldn't make socket a listening socket : " << strerror(errno) << std::endl;
            exit(1);
        }
    }
}

int Socket::accept(Socket& commSocket){
    if( socketmode == SERVER ){
        //Get the socket file descriptor and use it to construct a Socket.
        int commSockfd = ::accept(this->sockfd, NULL, NULL);
        commSocket = Socket( commSockfd );
        if( commSocket.sockfd == ERR ){
            End_Screen();
            std::cerr << "Couldn't make communication socket : " << strerror(errno) << std::endl;
            exit(1);
        }
        return RESULT_OK;
    }
    else return RESULT_ERROR;
}

int Socket::send(Packet& packet ){
    //Packet that we're gonna send.
    Packet sentPacket = packet;
    //Total data sent.
    size_t totalDataSent = 0;
    //Serializes packet header.
    sentPacket.header.messageSize = htons( sentPacket.header.messageSize );
    sentPacket.header.nameSize = htons( sentPacket.header.nameSize );
    //The packet header in raw byte form.
    const char* data = (const char*) &sentPacket.header;
    //Total size of the packet header.
    size_t packetHeaderSize = sizeof(PacketHeader);
    //Sends the packet header first, then send the payload.
    //This method of sending ensures that by the end of the loop all the data is sent.
    while( totalDataSent < packetHeaderSize ){
        //Send the data and also get how much data was actual sent( in bytes ).
        ssize_t dataSent = ::send( sockfd, data + totalDataSent, 
                                  packetHeaderSize - totalDataSent, 0);
        //The socket we were sending to disconnected.
        if( dataSent == 0 ) return RESULT_DISCONNECTED;
        else if( dataSent == ERR ) return RESULT_ERROR;
        //Increase the total data sent.
        totalDataSent += dataSent;
    }
    totalDataSent = 0;
    //Now send the payload, starting with the message.
    while( totalDataSent < sentPacket.message.size() ){
        //Send the actual string data and not a pointer to the string data.
        //Because addresses are machine dependant.
        ssize_t dataSent = ::send( sockfd, sentPacket.message.data() + totalDataSent,
                                 sentPacket.message.size() - totalDataSent, 0);
        //Disconnected.
        if( dataSent == 0 ) return RESULT_DISCONNECTED;
        //Error!
        else if( dataSent == ERR ) return RESULT_ERROR;
        //Increase total data sent.
        totalDataSent += dataSent;
    }
    totalDataSent = 0;
    //And now we send the sender's name.
    while( totalDataSent < sentPacket.sender.size() ){
        //Send the string data.
        ssize_t dataSent = ::send( sockfd, sentPacket.sender.data() + totalDataSent,
                                   sentPacket.sender.size() - totalDataSent, 0);
        //Just doing the same thing.
        if( dataSent == 0 ) return RESULT_DISCONNECTED;
        else if( dataSent == ERR ) return RESULT_ERROR;
        totalDataSent += dataSent;
    }
    //All done!
    return RESULT_OK;
}

int Socket::receive( Packet& outPacket ){
    //The resulting packet that will be received.
    Packet resultingPacket = {0};
    //Size of PacketHeader, good for knowing when and if we finished receiving all the header data.
    size_t packetHeaderSize = sizeof(PacketHeader);
    //The resulting packet header in raw byte form.
    char* data = (char*)&resultingPacket.header;
    //Total amount of data received by the socket in bytes.
    size_t totalDataReceived = 0;
    //Receive the whole header even if but a small amount was received at a time.
    while( totalDataReceived < packetHeaderSize ){
        ssize_t dataReceived = recv(sockfd, data + totalDataReceived,
                                    packetHeaderSize - totalDataReceived, 0);
        //They left us to rot...
        if( dataReceived == 0 ) return RESULT_DISCONNECTED;
        //error error chicken error.
        else if( dataReceived == ERR ) return RESULT_ERROR;
        totalDataReceived += dataReceived;
    }
    //De-serialize the packet header.
    resultingPacket.header.messageSize = ntohs( resultingPacket.header.messageSize );
    resultingPacket.header.nameSize = ntohs( resultingPacket.header.nameSize );

    totalDataReceived = 0;
    //This'll hold the raw message, + 1 to account for the null terminator.
    char* message = new char[ resultingPacket.header.messageSize + 1 ];
    while( totalDataReceived < resultingPacket.header.messageSize ){
        //Receive the data and get how much data was actually received.
        ssize_t dataReceived = recv( sockfd, message + totalDataReceived,
                                     resultingPacket.header.messageSize - totalDataReceived, 0);
        //Check for cases.
        if( dataReceived == 0 ) return RESULT_DISCONNECTED;
        else if( dataReceived == ERR ) return RESULT_ERROR;
        totalDataReceived += dataReceived;
    }
    //Null terminate.
    message[resultingPacket.header.messageSize] = '\0';

    totalDataReceived = 0;
    //This'll hold the raw name data + 1 to account for the null terminator.
    char* name = new char[ resultingPacket.header.nameSize + 1] ;
    while( totalDataReceived < resultingPacket.header.nameSize ){
        //This is getting kinda repetitive...
        ssize_t dataReceived = recv( sockfd, name + totalDataReceived,
                                     resultingPacket.header.nameSize - totalDataReceived, 0);
        if( dataReceived == 0 ) return RESULT_DISCONNECTED;
        else if( dataReceived == ERR ) return RESULT_ERROR;
        totalDataReceived += dataReceived;
    }
    //Null terminate.
    name[resultingPacket.header.nameSize] = '\0';

    //Turn the message and the name c strings to std::strings and put them on the packet.
    resultingPacket.message = std::string(message );
    resultingPacket.sender = std::string( name );
    //Store the processed packet to outPacket.
    outPacket = resultingPacket;
    //Free the memory used by the c strings.
    delete[] message;
    delete[] name;
    //Done!
    return RESULT_OK;
}

Socket::~Socket(){
    //gone
    close(sockfd);
}