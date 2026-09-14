 # Whack-a-Mole WebSocket Project

## Architecture

```
RpiGamelogic  ──────────────────►  RpiScore  ──────────────────►  Display.html
(WebSocket server)              (WebSocket client                (browser, opens on
runs on game PC                  TO RpiGamelogic,                external screen
or RPi)                          WebSocket server                connected to RPi)
                                 FOR Display.html,
                                 runs on RPi)
```

- **RpiGamelogic**: WebSocket server. Runs the game logic. Sends `GameState_t` as JSON to connected clients.
- **RpiScore**: Bridge. Connects to RpiGamelogic as a client (`ReceiveData`), tracks highscore locally, and forwards game state to the browser via its own WebSocket server (`DisplayServer`). Runs on the RPi.
- **Display.html**: Browser display. Connects to RpiScore's WebSocket server. Shows score, time, and highscore. Opens in a browser on the RPi's external screen.

## Folder structure

**Kun `Final_kode/` indeholder det endelige kode** — `testfase/` er prototype- og testkode og skal ikke redigeres.

```
Final_kode/
  RpiScore/
    include/
      GameState.hpp       — shared game state struct (JSON serialization)
      RpiScore.hpp
      ReceiveData.hpp     — WebSocket client (connects to RpiGameLogic)
      DisplayServer.hpp   — WebSocket server (serves Display.html)
    src/
      main.cpp
      RpiScore.cpp
      ReceiveData.cpp
      DisplayServer.cpp
  RpiGameLogic/
    inc/
      GameState.hpp       — shared game state struct
      GameLogic.h
      ForceSensor.h
      LED.h
      RoundNumb.h
      Terminal.h
      SendData.h
    src/
      main.cpp
      GameLogic.cpp
      ForceSensor.cpp
      LED.cpp
      RoundNumb.cpp
      Terminal.cpp
      SendData.cpp
  Display.html            — browser display client

testfase/                 — prototype- og testkode (ikke produktionskode)
  RpiGamelogic/
  RpiScore_struktureret/
  test_rpi_gamelogic/
```

## Network / IP setup

- `Display.html` hardcodes the WebSocket IP. This **must be the RPi's IP** (where RpiScore runs).
- RpiGamelogic's IP is passed as a command-line argument when running RpiScore.
- Current IP in `Display.html`: `ws://172.20.10.12:8080` — update this if the RPi's IP changes.
- RpiScore listens for the browser on port **8080**.
- RpiScore connects to RpiGamelogic on port **8080** (different machine, no conflict).

## Build — RpiScore (on the RPi)

Requires OpenSSL dev headers:
```bash
sudo apt-get install libssl-dev
```

Using CMake:
```bash
cd Final_kode/RpiScore
mkdir -p build && cd build
cmake .. && make
./client <IP_of_RpiGameLogic>
```

## Build — RpiGameLogic (on the game PC / RPi)

```bash
cd Final_kode/RpiGameLogic/build
cmake .. && make
./game
```

If port 8080 is already in use:
```bash
sudo fuser -k 8080/tcp
```

## GameState fields

Both `Final_kode/RpiGameLogic/inc/GameState.hpp` and `Final_kode/RpiScore/include/GameState.hpp` must stay in sync on field names and JSON keys.

| Field       | JSON key      | Notes                                      |
|-------------|---------------|--------------------------------------------|
| `score`     | `"score"`     | Current game score                         |
| `round`     | `"round"`     | Current round number                       |
| `time`      | `"time"`      | Remaining time                             |
| `gameState` | `"gameState"` | e.g. `"active"`, `"idle"`                  |
| `highScore` | `"highScore"` | Optional on receive side (defaults to `0`) |

**Important:** `highScore` is **not sent by RpiGameLogic** — `SendData.cpp` never sets it. RpiScore injects `state.highScore = highscore_` (its locally tracked all-time best) before forwarding to `Display.html`. The field is therefore always the running maximum and never resets to 0.

## Highscore logic

- RpiScore loads highscore from `/home/pi/highscore.txt` on startup.
- During the game loop, `updateHighscore` saves to file whenever `score > highscore_`.
- At end of session (loop exits), highscore is saved again as a safety net.
- RpiGameLogic never sends `highScore` — RpiScore injects it itself (`state.highScore = highscore_`) before forwarding to `Display.html`.
- `Display.html` also keeps a `localStorage` backup so the highscore survives a page reload before the server sends its first message.

## Terminal commands (RpiGameLogic)

The terminal in `Final_kode/RpiGameLogic/src/Terminal.cpp` accepts:
- `start` — start a new game
- `quit` — shutdown the program

Input is **case-insensitive** (handled via `std::transform` + `::tolower`). The same prompt is also printed from `GameLogic::waitForStart()` after each game ends.

## RpiGameLogic note

`Final_kode/RpiGameLogic/` indeholder den rigtige hardware-kode med ForceSensor, LED, RoundNumb og Terminal. Dette er ikke længere en placeholder.

