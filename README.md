# Gomoku AI
A bot capable of playing Gomoku (Five in a Row). The bot uses a combination of [alpha-beta pruning](https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning) and [threat-space search](https://www.mimuw.edu.pl/~awojna/SID/referaty/Go-Moku.pdf) to determine its moves.

Play [here](http://gomoku-ai.azurewebsites.net) against the bot (note: it might take a while for the page to load).

## Quick start (using Docker)
To install and run the application using Docker follow these steps:
- Install [Docker](https://www.docker.com/products/docker-desktop)
- Create a Docker image: `docker build --pull --rm -f "Dockerfile" -t gomokuai:latest .`
- Start a Docker container: `docker run --rm -it -p 7682:7682/tcp gomokuai:latest`
- Browse to [http://localhost:7682](http://localhost:7682).

## Manual installation
Instead of using Docker you can also manually install the application.

### Step 1: Install the bot
First you need to build the command line application.

- If you are using **Windows** then you have two options. The first option is to install [Visual Studio](https://visualstudio.microsoft.com/vs/community/) and open the solution `gomoku-ai.sln`. Next, choose the Release configuration and press build. The other option is to install [MSYS2](https://www.msys2.org/) and [GCC](https://gcc.gnu.org/), for example by following [this tutorial](https://github.com/orlp/dev-on-windows/wiki/Installing-GCC--&-MSYS2). Then run `./make` from an MSYS2 terminal.
- If you are using **Linux** then run `./make`.

In both cases you should now have a file `bin/release/gomoku-ai[.exe]`.

### Step 2: Install the server
The command line application has no graphical interface. To play against the bot in the browser, install a [Node.js](https://nodejs.org/) server as follows:
- Install [Node.js](https://nodejs.org/)
- Inside the `gomoku-server` directory, run `npm install && npm start`
- Browse to [http://localhost:7682](http://localhost:7682)

## Controls (browser client)
- **Left mouse button:** Place a stone
- **N:** New game (you are able to specify command line parameters)
- **M:** New game
- **B:** Toggle board
- **L:** Toggle log
- **D:** Toggle dark theme
- **S:** Show board string
- **Delete:** Stop
- **U:** Undo move
- **Enter:** Redo move
- **F:** Let the bots finish (no more breaks)
- **E:** Toggle editor
- **1/2/3/4:** Enable editor and change the left mouse button action to:
  - **1:** Place black stone
  - **2:** Place white stone
  - **3:** Place block
  - **4:** Highlight
- **R:** Toggle highlights
- **Right mouse button:** Remove stone (editor)
- **H:** Enable human supervisor (for debugging)

## Command line arguments
| Argument | Description |
| --- | --- |
| `-h` or `--help` | Show a help message |
| `-w` or `--width` | Set the width and height of the playing field (default=15) |
| `-H` or `--height` | Specify a different height for the playing field (default=0) |
| `-n` or `--stones-to-win` | Set the required minimum number of stones in a row to win (default=5) |
| `-q` or `--quiet` | No verbose logging |
| `-vq` or `--very-quiet` | No slightly-verbose logging |
| `-uv` or `--ultra-verbose` | Enable ultra-verbose logging |
| `-sc` or `--swap-colors` | Player 2 is the black player in the first game |
| `-fc` or `--fixed-colors` | The players do not swap colors after each game |
| `-ms` or `--manual-steps` | Pause after each move in an ai-vs-ai game |
| `-hb` or `--hide-board` | Hide the playing field |
| `-rm` or `--random-moves` | Specify the number of initial forced random moves (default=0) |
| `-rb` or `--random-blocks` | Specify the number of initial random blocks (default=0) |
| `--black` | Specify initial black stones |
| `--white` | Specify initial white stones |
| `--block` | Specify initial blocks |
| `-ws` or `--white-starts` | The white player has the first move |
| `-pb` or `--playback` | Specify the first moves to execute |
| `-tl` or `--time-limit` | Set the turn time limit (default=0.500000) |
| `-hm` or `--human` | Set ai to a human player |
| `-ai` or `--computer` | Set ai to the default ai |
| `-tss` or `--threat-space-search` | Set ai to the threat space search ai |
| `-ab` or `--alpha-beta` | Set ai to the alpha-beta ai |
| `-pns` or `--proof-number-search` | Set ai to the proof-number search ai |
| `-rd` or `--random` | The ai plays random moves |
| `-fd` or `--fixed-depth` | Specify a fixed depth for the alpha-beta search. Time limit is ignored when a fixed depth is set (default=0) |
| `-um` or `--unsafe-moves` | Do not search for safe moves |
| `-nt` or `--no-tracking` | Parameter used by alpha-beta |
| `-mml` or `--max-mask-length` | Parameter used by alpha-beta (default=10) |
| `-ag` or `--aggressiveness` | Determines the aggressiveness of the alpha-beta search (default=0.000000) |
| `-of` or `--only-fours` | Only consider four-threat sequences |
| `-tb` or `--table` | Temporary |
| `-nc` or `--no-combinations` | Skip the combination stages of the threat space search |
| `-nh` or `--no-halt-on-wts` | Do not switch to manual steps when a winning threat sequence is found |
| `-swd` or `--satisfied-with-draw` | The proof-number search ai is also satisfied with a draw instead of with a win only |
| `-jc` or `--json-client` | Interact with a json client instead of the command line |
| `-nsc` or `--no-sanity-checks` | No sanity checks |
| `-as` or `--auto-start` | Automatically start a new game when a game has ended |
| `-b` or `--battle` | Shortcut for `--auto-start --hide-board --no-halt-on-wts --quiet --very-quiet` |
| `-peb` or `--print-every-board` | Print the state of the game after every move |
| `-s` or `--seed` | Initial seed for the random number generator (default=-1) |
| `-hs` or `--human-supervisor` | Have veto rights over the moves of the ai (for debugging) |

Many arguments have player 1 and player 2 counterparts by appending '1' or '2'. For example, to set player 2 to the default ai, use `-ai2`.