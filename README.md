# Tic-Tac-Toe Network Game

A networked implementation of the classic Tic-Tac-Toe game with both TCP and UDP variants. Players can connect remotely to play against each other in real-time.

## Features

- Two separate implementations: TCP and UDP
- Two-player gameplay
- Real-time game board updates
- Input validation
- Play again functionality
- Network timeout handling
- Clean disconnection handling

## Project Structure

```
.
├── TCP/
│   ├── client.c
│   └── server.c
├── UDP/
│   ├── client.c
│   └── server.c
└── README.md
```

## Building the Project

### TCP Version
```bash
cd TCP
gcc -o server server.c
gcc -o client client.c
```

### UDP Version
```bash
cd UDP
gcc -o server server.c
gcc -o client client.c
```

## Running the Game

1. Start the server first:
   ```bash
   ./server
   ```

2. Start two client instances in separate terminals:
   ```bash
   ./client
   ```

## Game Rules

1. Players take turns placing their marks (X or O) on a 3x3 grid
2. First player uses 'X', second player uses 'O'
3. Input format: "row col" (e.g., "2 3" for second row, third column)
4. Game ends when:
   - One player gets three marks in a row (horizontal, vertical, or diagonal)
   - Board is full (draw)
   - Either player disconnects

## Network Details

### TCP Implementation
- Uses reliable stream-based communication
- Port: 2000
- Supports full-duplex communication
- Handles connection maintenance

### UDP Implementation
- Uses connectionless datagrams
- Port: 8080
- Implements basic reliability mechanisms
- Handles packet loss scenarios

## Technical Features

- Select()-based multiplexing for handling multiple clients
- Timeout mechanisms for unresponsive clients
- Error handling for network failures
- Clean termination of connections
- Board state synchronization
- Input validation and sanitization

## Error Handling

The game handles various error scenarios:
- Invalid moves
- Network disconnections
- Timeout situations
- Buffer overflows
- Invalid input formats
