#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <syncstream>
#include <csignal>

const std::string PORT = "3490";
const int BUFFERSIZE = 1024;
const int BACKLOG = 10;

bool connected = true;

int sockfd;
std::vector<std::thread> client_threads;
std::unordered_map<std::string, int> clients;
std::ofstream log_file;
std::mutex log_mutex;

void handle_sigint(int sig)
{
    std::cout << "\nServer shutting down...\n";
    connected = false;

    for(auto& t: client_threads)
        if(t.joinable())
            t.join();

    close(sockfd);
    log_file.close();

    std::cout << "\nServer terminated.\n";
    
    exit(0);
}


void logs(std::string username, std::string receiver, std::string message)
{
    std::lock_guard<std::mutex> lock(log_mutex);
    
    std::string log = username + " to " + receiver + ": " + message;
    log_file<<log<<std::endl<<std::flush;
    std::osyncstream(std::cout)<<log<<std::endl;

}   

void handle_message(std::string sender, std::string receiver, std::string message)
{
    if(clients.find(receiver) != clients.end())
    {
        std::string fullmessage = sender+":"+message;
        send(clients[receiver], fullmessage.c_str(), fullmessage.length(), 0);
        logs(sender, receiver, message);
    }   
    else
    {
        std::string error = "Invalid username\n";
        send(clients[sender], error.c_str(), error.length(), 0);
    }
}    


void client_connection(int client_socketfd, std::string username)
{
    char buffer[BUFFERSIZE];
    int message_received;
    std::string receiver, message;

    while(message_received = recv(client_socketfd, buffer, sizeof(buffer), 0))
    {
        buffer[message_received] = '\0'; 

        std::string received_message(buffer);

        auto delimiter = received_message.find(':');
        
        receiver = received_message.substr(0, delimiter);  
        message = received_message.substr(delimiter + 1); 
        
        handle_message(username, receiver, message);
    }

    clients.erase(username);
    
    close(client_socketfd);
}

void client_login(int client_socketfd)
{
    char buffer[BUFFERSIZE];

    int bytes_received = recv(client_socketfd, buffer, sizeof(buffer) - 1, 0);

    buffer[bytes_received] = '\0'; 
    std::string username(buffer);
    std::cout<<"Received username: "<<username<<std::endl;

    if (clients.contains(username))
    {
        if(client_socketfd == clients[username])
        {
            std::string welcome_message = "Welcome " + username + "!\n";
            send(client_socketfd, welcome_message.c_str(), welcome_message.length(), 0);
        }
        else    
        {
            std::string error_message = "User is already logged in from a different socket. \n";
            send(client_socketfd, error_message.c_str(), error_message.length(), 0);
        }
        
    }
    else
    {
        std::string welcome_message = "Welcome " + username + "!\n";
        send(client_socketfd, welcome_message.c_str(), welcome_message.length(), 0);
        clients[username] = client_socketfd;
    }

    std::thread client_thread(client_connection, client_socketfd, username);
    client_threads.push_back(std::move(client_thread));
}

void *get_in_addr(struct sockaddr *sa) 
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    log_file.open("chats.txt", std::ios::out | std::ios::app);
   
    signal(SIGINT, handle_sigint);

    int new_fd; 
    struct addrinfo hints{}, *servinfo, *p;
    struct sockaddr_storage their_addr; 
    socklen_t sin_size;

    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (rv = getaddrinfo(nullptr, PORT.c_str(), &hints, &servinfo); rv != 0) 
    {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        return 1;
    }

    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
        {
            std::cerr<<"server: socket";
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
        {
            std::cerr<<"setsockopt";
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
        {
            close(sockfd);
            std::cerr<<"server: bind";
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); 

    if (p == nullptr) 
    {
        std::cerr << "server: failed to bind" << std::endl;
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) 
    {
        std::perror("listen");
        exit(1);
    }

    std::cout << "server: waiting for connections..." << std::endl;

    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while(connected) 
    { 
        read_fds = master_set;
        int select_result = select(sockfd + 1, &read_fds, nullptr, nullptr, &tv);

        if (select_result == -1)
        {
            std::cerr<<"select";
            break;
        }

        if(FD_ISSET(sockfd, &read_fds))
        {
            sin_size = sizeof their_addr;
            new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        
            if (new_fd == -1) 
            {
                if(!connected)
                    break;
                std::cerr<<"accept";
                continue;
            }

            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
            std::cout << "server: got connection from " << s << std::endl;

            client_login(new_fd);
        }

    }

    return 0;
}