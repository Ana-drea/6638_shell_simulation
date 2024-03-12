#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/wait.h>

// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd
{
  int type; //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd
{
  int type;            // ' '
  char *argv[MAXARGS]; // arguments to the command to be exec-ed
};

struct redircmd
{
  int type;        // < or >
  struct cmd *cmd; // the command to be run (e.g., an execcmd)
  char *file;      // the input/output file
  int mode;        // the mode to open the file with
  int fd;          // the file descriptor number to use for the file
};

struct pipecmd
{
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

int fork1(void); // Fork but exits on failure.
struct cmd *parsecmd(char *);

// Execute cmd.  Never returns.
void

runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;
  if (cmd == 0)
  {
    printf("empty command\n");
    exit(0);
  }

  switch (cmd->type)
  {
  default:
    fprintf(stderr, "unknown runcmd\n");
    exit(-1);

  case ' ':
    ecmd = (struct execcmd *)cmd;
    if (ecmd->argv[0] == 0)
      exit(0);

    execvp(ecmd->argv[0], ecmd->argv);
    fprintf(stderr, "exec not implemented\n");
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd *)cmd;
    // printf("file %s\n", rcmd->file);
    // printf("type %c\n", rcmd->type);
    // printf("mode %d\n", rcmd->mode);
    // printf("subcmd type %c\n", rcmd->cmd->type);
    int fd;
    if (rcmd->type == '<')
    {
      fd = open(rcmd->file, rcmd->mode, 0600);
    }
    else if (rcmd->type == '>')
    {
      fd = open(rcmd->file, rcmd->mode, 0644);
    }
    if (fd < 0)
    {
      perror("open");
      exit(EXIT_FAILURE);
    }

    // 重定向标准输入/输出
    if (dup2(fd, rcmd->fd) < 0)
    {
      perror("dup2");
      exit(EXIT_FAILURE);
    }

    // 关闭不再需要的文件描述符
    close(fd);
    runcmd(rcmd->cmd);
    break;

  case '|':
    pcmd = (struct pipecmd *)cmd;
    // fprintf(stderr, "pipe not implemented\n");
    // printf("func: runcmd, case '|'\n");
    // Your code here ...
    int pipefd[2];
    pid_t pid;

    // 创建管道
    if (pipe(pipefd) == -1)
    {
      perror("pipe");
      exit(EXIT_FAILURE);
    }

    // Create a child process.
    pid = fork();

    if (pid == -1)
    {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    { // child process

      // Redirect standard output to the write end of the pipe, so that the execution result won't be displayed on the console but streamed to the read end of the pipe.
      close(pipefd[0]);
      // In the context of the pipe, the left command (child process) is considered the source of the data stream, so its output is redirected to the write end of the pipe.
      if (dup2(pipefd[1], STDOUT_FILENO) == -1)
      {
        perror("dup2");
        exit(EXIT_FAILURE);
      }

      // Close the write end of the pipe.
      close(pipefd[1]);

      // execute left side of the pipe command

      // struct execcmd *t_cmd = pcmd->left;
      // printf("pipe, ecmd argv[0]: %s", t_cmd->argv[0]);

      // Close the read end of the pipe.

      runcmd(pcmd->left);
      // Exit child process
      exit(EXIT_SUCCESS);
    }   
    else
    { // Parent process
      // Wait for the child process to finish
      

      // Close the write end of the pipe because the parent process is only responsible for reading data
      close(pipefd[1]);

      // Redirect standard input to the read end of the pipe
      if (dup2(pipefd[0], STDIN_FILENO) == -1)
      {
        perror("dup2");
        exit(EXIT_FAILURE);
      }

      // close read end of the pipe
      close(pipefd[0]);
      // struct execcmd *t_cmd = pcmd->left;
      // printf("pipe, ecmd argv[0]: %s", t_cmd->argv[0]);
      // execute right side of the pipe command
      runcmd(pcmd->right);
    }
    waitpid(pid, NULL, 0);
    break;
  }
  exit(0);
}

int getcmd(char *buf, int nbuf)
{

  if (isatty(fileno(stdin)))
    fprintf(stdout, "$ ");
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  if (buf[0] == 0) // EOF
    return -1;
  return 0;
}

int main(void)
{
  static char buf[100];
  int fd, r;

  // Read and run input commands.
  while (getcmd(buf, sizeof(buf)) >= 0)
  {
    if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')
    {
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf) - 1] = 0; // chop \n
      if (chdir(buf + 3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf + 3);
      // else
      // {
      //   printf("moved to %s\n", buf + 3);
      // }
      continue;
    }
    if (fork1() == 0)
    {
      // printf("child running\n");
      // printf("buf: %s", buf);
      runcmd(parsecmd(buf));
    }
    // printf("waiting...\n");
    wait(&r);
  }
  exit(0);
}

int fork1(void)
{
  int pid;

  pid = fork();
  if (pid == -1)
    perror("fork");
  return pid;
}

struct cmd *
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd *)cmd;
}

struct cmd *
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->mode = (type == '<') ? O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd *)cmd;
}

struct cmd *
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd *)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

// 首先，它从输入字符串 s 开始扫描，跳过了前导的空白字符（whitespace）。

// 然后，它检查了当前字符的类型：

// 如果当前字符是空字符（'\0'），则说明已经到达了输入字符串的结尾，函数返回 0。
// 如果当前字符是管道符号 '|'、输入重定向符号 '<' 或输出重定向符号 '>' 中的一个，那么函数返回对应的 ASCII 码。
// 否则，当前字符应该是一个普通的单词字符，函数将继续扫描直到遇到下一个空白字符或特殊符号，并将这段连续的字符作为一个单词 token，并返回字符 'a' 的 ASCII 码。
// 在扫描 token 的过程中，函数会将 token 的起始地址存储在 q 指针指向的位置，并将 token 的结束地址存储在 eq 指针指向的位置。如果调用者不需要这些信息，则可以将 q 和 eq 参数传递为 NULL。

// 最后，函数将指针 ps 指向当前 token 的下一个位置，并返回 token 的类型或结束标志。

// 这段代码使用了两次循环：第一个循环用于跳过前导的空白字符，第二个循环用于跳过 token 之后可能存在的额外空白字符。这样可以保证函数能够正确地提取出下一个 token，并将输入指针 ps 指向下一个未处理的位置。
int gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;
  if (q)
    *q = s;
  ret = *s;
  switch (*s)
  {
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if (eq)
    *eq = s;

  while (s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char *mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n + 1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}


struct cmd *
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  // printf("func: parsecmd\n");
  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if (s != es)
  {
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd *
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  // printf("func: parseline\n");
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd *
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  // printf("func: parsepipe\n");
  cmd = parseexec(ps, es);
  if (peek(ps, es, "|"))
  {
    // gettoken(ps, es, 0, 0)跳过当前管道符号 |，以便继续处理后面的命令。它会将指针 ps 向后移动，跳过当前的管道符号，使得 ps 指向下一个待处理的位置。
    gettoken(ps, es, 0, 0);
    // 传入两个参数分别是|左边的命令和|右边的命令
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd *
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  // printf("func: parseredirs\n");
  // 使用 peek 函数检查当前位置是否为重定向符号 < 或 >
  while (peek(ps, es, "<>"))
  {
    // 如果当前位置是重定向符号，则使用 gettoken 函数获取下一个标记（文件名）
    tok = gettoken(ps, es, 0, 0);
    // printf("parseredirs tok: %c\n", tok);
    if (gettoken(ps, es, &q, &eq) != 'a')
    {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    // 使用 switch 语句根据重定向符号的类型执行相应的操作：
    switch (tok)
    {
      // 如果是 <，则调用 redircmd 函数将输入重定向应用于命令，并将结果更新为新的命令结构
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      // printf("func parseredirs cmd->file: %s",cmd->file);
      // printf("func parseredirs cmd->file: %s",cmd->mode);
      break;
      // 如果是 >，则调用 redircmd 函数将输出重定向应用于命令，并将结果更新为新的命令结构
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
    // 循环执行上述步骤，直到没有更多的重定向符号
  }
  // 返回最终更新后的命令结构
  return cmd;
}

struct cmd *
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  // printf("func: parseexec\n");
  ret = execcmd();
  cmd = (struct execcmd *)ret;

  argc = 0;
  // 调用 parseredirs 函数解析命令中的重定向符号，并将结果更新到 ret 中
  ret = parseredirs(ret, ps, es);
  // 使用循环来解析命令中的参数，直到遇到管道符 | 或者结束符号为止。
  while (!peek(ps, es, "|"))
  {
    // 使用 gettoken 函数获取下一个标记，并检查其类型，返回0则说明已经到达了输入字符串的结尾
    if ((tok = gettoken(ps, es, &q, &eq)) == 0)
      break;
    // 如果不是合法的标记类型，则输出语法错误信息并退出程序。
    if (tok != 'a')
    {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    // 如果标记类型是合法的参数标记类型 'a'，则使用 mkcopy 函数创建参数的副本，并将其存储在执行命令结构体的参数数组中。
    cmd->argv[argc] = mkcopy(q, eq);
    // printf("parseexec tok: %c, argc: %d, cmd->argv[argc]: %s\n", tok, argc, cmd->argv[argc]);
    argc++;
    // 如果参数数量超过了 MAXARGS，则输出错误信息并退出程序。
    if (argc >= MAXARGS)
    {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    // 调用 parseredirs 函数解析命令中的重定向符号，并将结果更新到 ret 中。
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
