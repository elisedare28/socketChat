## Requirements:

- **C++ Compiler**: g++ with support for C++20.
- **Linux/Unix Environment**: Required for sockets and signals.
- **Terminal**: For running the server and client processes.

## Usage:

### Compile the Server:
```
    bash
g++ server.cpp -std=c++20 -o server

```

### Compile the Client:
```
    bash
g++ client.cpp -std=c++20 -o client

```

### Run the Server
``` 
    bash
    ./server

```

### Run the Clients
```
    bash
    ./client <hostname>

```

## Working
### Server Workflow:
1. The server listens for incoming client connections on port `3490`.
2. Upon connection, the client sends a username to the server.
3. The server accepts the username and manages client connections.
4. Clients can send messages in the format:  
   ```
   recipient_username:message

   ```
5. The server forwards the message to the intended recipient (if online) and logs it to `chats.txt`.
6. The server gracefully handles `Ctrl+C` to shut down and close all connections.

### Client Workflow:
1. The client connects to the server and prompts the user to enter a username.
2. After a successful login, the client can send messages to other connected users.
3. Messages from other users are displayed in real-time.
4. Use `Ctrl+C` to disconnect from the server.

**You can run multiple clients on separate terminals.**
