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


***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.
