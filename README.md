# gomoku-ai
A bot capable of playing Gomoku (Five in a Row). The bot uses a combination of alpha-beta pruning and [Threat Space Search](https://www.mimuw.edu.pl/~awojna/SID/referaty/Go-Moku.pdf) to determine its moves.

Play against the bot at [http://www.gomoku-ai.ewps.nl](http://www.gomoku-ai.ewps.nl).

## Build process
- **Windows:** Either use Visual Studio to open `gomoku-ai.sln`, or run `./make` inside an MSYS2 terminal.

- **Linux:** Run `./make`.

To get a list of all command line parameters run `gomoku-ai -h`.

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

## How to start your own Gomoku server
- Make sure the Gomoku bot has been installed in `bin/release`.
- Install Node.js.
- Move to the `gomoku-server` directory.
- Run `npm install`.
- Start the server with `npm start`.
- Open your browser and navigate to [http://localhost:7682](http://localhost:7682).

Optionally, if you want the server to automatically restart when there are changes to `server.coffee`, you can follow the following steps:

- Install CoffeeScript `npm install --global coffee-script`.
- Install node-supervisor `npm install --global supervisor`.
- Inside the `gomoku-server` directory, run `start-and-watch.bat` or `./start-and-watch`, depending on your platform.
