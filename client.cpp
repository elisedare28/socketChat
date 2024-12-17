#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <csignal>
#include <string.h>
#include <errno.h>

const std::string PORT = "3490";
const int BUFFERSIZE = 100;

int sockfd;
bool connected = true;
std::thread receive_thread;

void handle_sigint(int sig)
{
    std::cout << "\nDisconnecting...\n";
    connected = false;

    if(receive_thread.joinable())
        receive_thread.join();

    close(sockfd);
    std::cout << "\nDisconnected.\n";

    exit(0);
}

void client_login(int client_socketfd)
{
    std::string username;

    while(true)
    { 
        std::cout<<"Enter your username: ";
        std::cin>>username;

        if(send(client_socketfd, username.c_str(), username.length(), 0) == -1)
        {
            std::cerr<<"send";
            continue;
        }
        char response[BUFFERSIZE] = {'\0'}; 

        int bytes_received = recv(client_socketfd, response, sizeof(response) - 1, 0);
        if (bytes_received == -1)
        {
            std::cerr<<"recv";
            continue;
        }

        response[bytes_received] = {'\0'}; 
        std::cout<<response<< std::endl;

        if (std::string(response) == "Welcome " + username + "!\n")
        {
            break;
        }
        else
        {
            std::cout << "Invalid username. Please try again." << std::endl;
        }
    
    }
}


void *get_in_addr(struct sockaddr *sa) 
{
    if (sa->sa_family == AF_INET) 
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}


void receive_message(int client_socketfd)
{
    char buffer[BUFFERSIZE];
    while (true)
    {
        int bytes_received = recv(client_socketfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == -1)
        {
            std::cerr << "Error receiving message." << std::endl;
            break;
        }
        if (bytes_received == 0)
        {
            std::cout << "Server disconnected." << std::endl;
            break;
        }

        buffer[bytes_received] = '\0'; 

        std::cout<<std::endl<<"Received message:\n"<<buffer<<std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) 
    {
        std::cerr << "Usage: client <hostname>" << std::endl;
        return 1;
    }

    signal(SIGINT, handle_sigint);

    int numbytes;  
    char buf[100];
    struct addrinfo hints{}, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];


    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 


    if (rv = getaddrinfo(argv[1], PORT.c_str(), &hints, &servinfo); rv != 0) 
    {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        return 1;
    }

    for (p = servinfo; p != nullptr; p = p->ai_next) 
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) 
        {
            std::cerr<<"client: socket";
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            std::cerr<<"client: connect";
            continue;
        }

        break;  
    }

    if (p == nullptr) 
    {
        std::cerr << "client: failed to connect" << std::endl;
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    std::cout << "client: connecting to " << s << std::endl;

    freeaddrinfo(servinfo); 

    client_login(sockfd);

    receive_thread = std::thread(receive_message, sockfd);

    std::cout<<"Enter recipient and message (username:message): \nPress Ctrl+C to disconnect\n";

    while(connected)
    {
        std::string message;
        std::getline(std::cin, message);
        send(sockfd, message.c_str(), message.length(), 0);
    }

    close(sockfd); 

    return 0;
}