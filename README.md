# Custom Unix Shell Implementation

## Features
**Basic Functionality**  
- Command execution (e.g., `ls`, `grep`)  
- Background execution (`command &`)  
- Built-in `exit` command  
- Command history tracking (10 commands)  

**Advanced Features**  
- Pipelining: `cmd1 | cmd2 | cmd3`  
- I/O Redirection:  
  - Input: `cmd < file.txt`  
  - Output: `cmd > file.txt`  
  - Append: `cmd >> file.txt`  
- Named Pipes: `mkfifo mypipe; cmd1 > mypipe & cmd2 < mypipe`  
- History:  
  - View: `history`  
  - Recall: `!!` (last cmd), `!3` (cmd #3)  

## Technical Implementation
**Core Components**  
- Command parser handling tokens/special characters  
- Process management using fork/exec  
- File descriptor manipulation for redirection  
- Circular buffer for command history  

**Specifications**  
| Component          | Capacity           |
|--------------------|--------------------|
| Command Length     | 200 characters max |
| Arguments          | 20 per command     |
| History Storage    | 10 commands        |

## Building & Execution
```bash
# Compile
g++ "Custom Shell.cpp" -o myshell

# Run
./myshell
> ls -l | grep ".txt" > output.txt
