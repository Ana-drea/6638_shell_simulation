This is a simplified version of a shell program. The shell supports basic command execution, input/output redirection, and piping.

Features
Command Execution: The shell can execute commands entered by the user.
Input/Output Redirection: Commands can redirect their input (<) or output (>).
Piping: Multiple commands can be connected through pipes (|).

Code Structure
sh.c: Contains the main loop for reading and executing user commands.
t.sh: Contains multi-line commands to test the shell program

How to Use
Compile: Compile the code using a C compiler (e.g., gcc sh.c -o sh.out).
Run: Execute the compiled binary (./sh.out).
Enter Commands: Enter commands at the prompt ($) and press Enter to execute.

Supported Commands
Basic Execution: Enter any valid shell command.
Redirection: Redirect input (<) or output (>) of a command.
Usage Examples
Piping: Connect multiple commands using the pipe operator (|).

Basic Command Execution
$ ls
Command with Input Redirection
$ cat < x.txt
Command with Output Redirection
$ echo "6.828 is cool" > x.txt
Command with Piping
$  ls | sort | uniq | wc