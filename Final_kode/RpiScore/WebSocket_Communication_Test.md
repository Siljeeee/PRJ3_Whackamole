# WebSocket Communication Test
# RPi Spillogik (server) <---> RPi Score (client)

## Structure
```
ws_test/
├── shared/
│   └── GameState.hpp       # shared JSON struct (used by both)
├── rpi_spillogik/
│   └── main.cpp            # WebSocket server - sends test GameState
└── rpi_score/
    └── main.cpp            # WebSocket client - receives and prints
```

## What this test does
- RPi Spillogik starts a WebSocket server on port 8080
- Every 2 seconds it sends a hardcoded GameState as JSON
- RPi Score connects and prints each received d

## Compile & Run

### On RPi Spillogik:
```bash
cd rpi_spillogik
g++ -std=c++17 main.cpp -o server -lrestinio -ljson_dto -lpthread
./server
```

### On RPi Score (replace IP with actual RPi Spillogik IP):
```bash
cd rpi_score
g++ -std=c++17 main.cpp -o client -ljson_dto -lpthread
./client 192.168.x.x
```

## Expected output

### RPi Spillogik (server):
```
[Server] Starting WebSocket server on port 8080...
[Server] Client connected - upgrading to WebSocket.
[Server] Sending: {"score":0,"round":1,"time":30,"gameState":"active"}
[Server] Sending: {"score":10,"round":2,"time":29,"gameState":"active"}
...
```

### RPi Score (client):
```
[Client] Connecting to 192.168.x.x:8080...
[Client] TCP connected. Sending WebSocket handshake...
[Client] WebSocket connected! Waiting for data...

-----------------------------
[Client] Received GameState:
  score:     0
  round:     1
  time:      30
  gameState: active
-----------------------------
```

## Testing on the same machine (no second RPi needed yet)
Run server in one terminal, then in another:
```bash
./client 127.0.0.1
```

## Find RPi IP address
```bash
hostname -I
```
