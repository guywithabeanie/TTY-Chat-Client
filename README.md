# TTY Chat Client
 A terminal based chat client written in C++ using NCurses and Berkeley sockets, this code was written for Linux using Linux, though i do think that it might be able to run on BSD, however it can't in this current state run on Windows, although i think it can work using PDCurses and Winsock (which is based on Berkeley sockets) alongside other minor modifications.

# How to use :
 At the moment you can use "--host" to host a server, where the server will be hosted on port 6969 (nice) and on the machine's address, and "--join" to join a server, where "--join" must be followed by the address of the host, while the port is automatically set to 6969 (nice). Alongside that you can use "--name" followed by the a string to...go figure. If a name wasn't provided the name will be automatically set to "Mingebag".
