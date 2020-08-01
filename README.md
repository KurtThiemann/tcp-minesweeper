# TCP-Minesweeper

### About
TCP-Minesweeper is a small test project to learn C.
It creates a TCP socket to which you can connect using netcat 
and play a weird multiplayer version of Minesweeper.  

Latest Linux x86-64 build in the Releases, everything else you'll have to build yourself.

### Usage
Start the server
```bash
./Minesweeper [<game width> <game height> <amount of mines> <port>]
```
Connect using netcat
```bash
stty raw -echo && nc <host> <port> && stty sane
```

Connect using PuTTY  
 * Session
   * Connection type -> Raw
 * Terminal
   * Local echo -> Force off
   * Local line editing -> Force off