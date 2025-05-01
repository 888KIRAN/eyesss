
  
```text
 _______       ___    ___ _______   ________   ________   ________      
|\  ___ \     |\  \  /  /|\  ___ \ |\   ____\ |\   ____\ |\   ____\     
\ \   __/|    \ \  \/  / | \   __/|\ \  \___|_\ \  \___|_\ \  \___|_    
 \ \  \_|/__   \ \    / / \ \  \_|/_\ \_____  \\ \_____  \\ \_____  \   
  \ \  \_|\ \   \/  /  /   \ \  \_|\ \|____|\  \\|____|\  \\|____|\  \  
   \ \_______\__/  / /      \ \_______\____\_\  \ ____\_\  \ ____\_\  \ 
    \|_______|\___/ /        \|_______|\_________\\_________\\_________\
             \|___|/                  \|_________\|_________\|_________|
```



# **EYESSS**

**EYESSS** is a sleek, powerful network analysis tool written in C for Linux systems. It uses system libraries to capture and analyze network traffic, features a simple terminal-based UI, and provides multithreading support for maximum efficiency. It’s the eyes you need on your network, all in one compact tool.

## Features

- **Network Packet Capture**: Dive deep into network traffic and capture packets for analysis using `libpcap`.
- **Terminal-Based Interface**: A clean and effective terminal interface powered by `ncurses`—because who needs flashy GUIs?
- **Multithreading Support**: Operates like a pro, using `pthread` for simultaneous tasks—speed and efficiency, all in one.
- **Unicode Support**: Perfect for multilingual environments. It supports all the characters you’ll need to truly "see" the data.
- **Secure Execution**: Built with safety in mind using the `-z noexecstack` flag to prevent common stack-based attacks.
- Security Spy Software

## Dependencies

Before you start unleashing the power of **EYESSS**, make sure your system is ready by installing these essential packages:

## **EYESSS - Command Reference**

Below is a table of available commands for **EYESSS**.

| Command                | Description                  |
|------------------------|------------------------------|
| `<network IP> -Sc`     | Scan the network.            |
| `<network IP> -Ps`     | Scan ports on the network.   |
| `-mode`                | Enable monitor mode.         |
| `help`                 | Show this help message.      |
| `help -All`            | Show all available commands. |
| `exit`                 | Quit the shell.              |
| `clear`                | Clear the screen.            |
| `history`              | Show command history.        |
| `version`              | Show the program version.    |

### Install Dependencies

Simply run the following commands to get everything set up:

```bash
sudo apt update
git clone https://github.com/888KIRAN/eyesss.git
sudo ./eye1
