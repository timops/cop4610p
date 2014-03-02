/*
 *
 * myshell.c
 * Tim Green
 * 1/25/14
 * version 1.0
 *
 * Project 1 - UNIX Shell
 *
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_ARGS 9
#define MAX_ARG_SIZE 81
#define MAX_PATH 1025
#define MAX_HISTORY 100
#define MAX_ENV 33

// stores all useful information about the command to be executed. 
struct cmd_struct
{
  int args;
  char * cmd;
  char *str_args[MAX_ARGS];
  char infile[MAX_ARG_SIZE];
  char outfile[MAX_ARG_SIZE];
};

// see function implementations for documentation.
char * EvalToken(const char * token);
int UpdateHistory(const char * mycmd, char **history, int history_size);
char * GetFullPath(struct cmd_struct * prc_cmd);
void InitCommand(struct cmd_struct * prc_cmd);
void ExecvWrapper(const struct cmd_struct * prc_cmd);
void ShowPrompt();

int main()
{
  int finished = 0;
  int temp = 0;
  int syntaxerr = 0;

  char * history[MAX_HISTORY];
  int history_size = 0;

  const char * whitespace = " \n\r\f\t\v";
  char * token;

  char mycmd[MAX_ARG_SIZE];
  char curr_arg[MAX_ARG_SIZE];
  char var_name[MAX_ENV];

  mycmd[0] = '\0';
  curr_arg[0] = '\0';
  var_name[0] = '\0';

  struct cmd_struct prc_cmd;

  // main loop for user interface.  exit command to escape.
  while(!finished)
  {
    syntaxerr = 0;
    InitCommand(&prc_cmd);

    ShowPrompt();
    if (fgets(mycmd, MAX_ARG_SIZE, stdin) == NULL)
    {
      perror("[ERROR]");
      continue;
    }
    else if (mycmd[0] == '\n')                                               
      syntaxerr = 1;

    // add entry to the history, while handling special case where history is 
    // MAX_HISTORY size.
    history_size = UpdateHistory(mycmd, history, history_size);

    token = strtok(mycmd, whitespace);
    while (token != NULL)
    {
      // special cases on the cli that must be dealt with: '$', '<', '>'
      switch (token[0])
      {
        case '$': 
          strcpy(curr_arg, EvalToken(token));
          if(strlen(curr_arg) == 0)
            // if the variable is undefined, then save it for error message we will print later.
            strcpy(var_name, token);
          prc_cmd.str_args[prc_cmd.args] = (char *) malloc((strlen(curr_arg)+1) * sizeof(char));
          strcpy(prc_cmd.str_args[prc_cmd.args], curr_arg);
          prc_cmd.args++;
          break;

        case '<':
          token = strtok(NULL, whitespace);
          if (token != NULL)
            strcpy(prc_cmd.infile, token);
          else
          {
            syntaxerr = 1;
            printf("[ERROR]: missing argument after <\n");
          }
          break;

        case '>':
          token = strtok(NULL, whitespace);
          if (token != NULL)
            strcpy(prc_cmd.outfile, token);
          else
          {
            syntaxerr = 1;
            printf("[ERROR]: missing argument after >\n");
          }
          break;

        default:
          prc_cmd.str_args[prc_cmd.args] = (char *) malloc((strlen(token)+1) * sizeof(char));
          strcpy(prc_cmd.str_args[prc_cmd.args], token);
          prc_cmd.args++;
          break;
      }
      if (syntaxerr)
        break;
      token = strtok(NULL, whitespace);
    }
    prc_cmd.cmd = prc_cmd.str_args[0];

    // if the operators above are mispositioned, don't continue parsing the command.
    if(!syntaxerr) 
    {
      if (strcmp(prc_cmd.cmd, "exit") == 0)
        finished = 1;
      else if(strcmp(prc_cmd.cmd, "cd") == 0)
      {
        // prc_cmd.args is actually n+1 arguments because it includes the command 
        // that is invoked as the first arg.
        if(prc_cmd.args > 2) 
          printf("[ERROR] too many arguments: %d\n", (prc_cmd.args-1));
        // if the environment variable isn't set, then print an error message (tcsh behavior).
        else if( (prc_cmd.args == 2) && (strlen(prc_cmd.str_args[1]) == 0) )
          printf("%s: Undefined variable.\n", var_name);
        // no arguments will perform a 'cd $HOME' per the project specification.
        // this is standard behavior for tcsh.
        else if (prc_cmd.args == 1)
        {
          if(chdir(getenv("HOME")) == -1)
            perror("[ERROR]");
        }
        // the common case: single argument will change to the directory if it exists.
        else 
        {
          if(chdir(prc_cmd.str_args[1]) == -1)
            perror("[ERROR]");
        }
      }
      else if(strcmp(prc_cmd.cmd, "echo") == 0)
      {
        // if *only one* of the variables is undefined, print an error message.
        // eg: echo $HOME $BLAH should print an error assuming blah is undefined.
        if(strlen(var_name) != 0)
          printf("%s: Undefined variable.", var_name);
        else 
        {
          int i;
          for (i=1; i<prc_cmd.args; i++)
            printf("%s ", prc_cmd.str_args[i]);
        }
      }
      else if(strcmp(prc_cmd.cmd, "history") == 0)
      {
        int i;
        for (i=0; i<history_size; i++)
          printf("%s", history[i]);
      }
      // catch-all for any command that isn't a built-in.
      else
      {
        // get the full path from $PATH.
        if (strchr(prc_cmd.cmd, (int) '/') == NULL)
          prc_cmd.cmd = GetFullPath(&prc_cmd);

        pid_t pid = fork();
        if (pid < 0)
        {
          perror("[ERROR]");
          exit(EXIT_FAILURE);
        }
        if (pid == 0)
        {
          ExecvWrapper(&prc_cmd);
          perror("[ERROR]");
          exit(EXIT_FAILURE);
        }
        waitpid(pid, NULL, 0);
      }
      for (temp=0; temp<prc_cmd.args; temp++)
        free(prc_cmd.str_args[temp]);
      var_name[0] = '\0';
    }
  }

  // global clean up 
  for (temp=0; temp<history_size; temp++)
    free(history[temp]);

  return 0;
}

/*
 * Assume whitespace around each environment variable.
 * (Don't have to handle the $HOME$HOME or $HOME/path corner cases.
 *
 */
char * EvalToken(const char * token)
{
  char env[MAX_ENV];  // what is the max length of the env variable name?
  strcpy(env, &token[1]);

  return (getenv(env) == NULL) ? "" : getenv(env);
}

/*
 * Append command and corresponding arguments to history and increase its size.  
 * If size is at MAX_HISTORY, expire oldest element before appending.
 *
 */
int UpdateHistory(const char *mycmd, char **history, int history_size)
{
  if (history_size < MAX_HISTORY)
  {
    // allocate memory for a single line.
    history[history_size] = (char*) malloc(sizeof(char) * (strlen(mycmd)+1));
    if (history[history_size] == NULL) 
    {
      perror("[ERROR]");
      exit(EXIT_FAILURE);
    }
    strcpy(history[history_size], mycmd);
    history_size++;
  }
  // we've reached the maximum history size, so do some maintenance (i.e. expire
  // the oldest entry).
  else
  {
    free(history[0]);

    int temp;
    for (temp=0; temp<(history_size-1); temp++)
      history[temp] = history[temp+1];

    history[MAX_HISTORY-1] = (char *) malloc(sizeof(char) * (strlen(mycmd)+1));
    if (history[history_size-1] == NULL) 
    {
      perror("[ERROR]");
      exit(EXIT_FAILURE);
    }
    strcpy(history[history_size-1], mycmd);
  }

  return history_size;
}

/*
 * return a pointer to the full path of the first match for executable in $PATH.
 * if a match isn't found from the stat call, just return the executable name.
 *
 */
char * GetFullPath(struct cmd_struct * prc_cmd)
{
  // not sure what the character limit of environment variable is, so be 
  // conservative.
  char path_env[MAX_PATH];
  char full_path[MAX_ARG_SIZE] = "\0";
  const char * delimit = ":";

  struct stat exec_stat;

  char * token;

  strcpy(path_env, getenv("PATH")); 
  token = strtok(path_env, delimit);

  while (token != NULL)
  {
    strcpy(full_path, token);
    strcat(full_path, "/");
    strcat(full_path, prc_cmd->cmd);

    // expected behavior is to take first match.
    if (stat(full_path, &exec_stat) == 0)
    {
      // realloc() because this memory was already allocated for the command, 
      // but not the full path (presumably this will require more memory).
      prc_cmd->str_args[0] = (char *) realloc((void *) prc_cmd->str_args[0], (strlen(full_path)+1) * sizeof(char));
      strcpy(prc_cmd->str_args[0], full_path);
      prc_cmd->cmd = prc_cmd->str_args[0];
      break;
    }
    token = strtok(NULL, delimit);
  }
  return prc_cmd->cmd;
}

/*
 * initialize all elements of the command structure.
 *
 */
void InitCommand(struct cmd_struct * prc_cmd)
{
  // reset structure for each new command.
  prc_cmd->args = 0;
  prc_cmd->cmd = NULL;
  prc_cmd->infile[0] = '\0';
  prc_cmd->outfile[0] = '\0';

  int loop = 0;

  for(loop=0; loop<MAX_ARGS; loop++)
    prc_cmd->str_args[loop] = NULL;
}

/*
 * wrap the exec syscall so we can appropriately handle file redirection (where necessary).
 *
 */
void ExecvWrapper(const struct cmd_struct * prc_cmd)
{
  int infile, outfile;

  if (prc_cmd->infile[0] != '\0')
  {
    infile = open(prc_cmd->infile, O_RDONLY);
    dup2(infile, 0);
    close(infile); 
  }
  if (prc_cmd->outfile[0] != '\0')
  {
    outfile = open(prc_cmd->outfile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    dup2(outfile, 1);
    close(outfile); 
  }
  execv(prc_cmd->cmd, prc_cmd->str_args);
}

/*
 * build and print the command prompt using the necessary syscalls.
 *
 */
void ShowPrompt()
{
  char * user, * cwd;

  cwd = (char*) malloc(sizeof(char) * MAXPATHLEN);
  if (cwd == NULL) 
  {
    perror("[ERROR]");
    exit(EXIT_FAILURE);
  }

  // if the environment variable isn't set, it won't be added to the prompt 
  // (this is how any other shell would behave).
  user = getenv("USER");
  cwd = getcwd(cwd, MAXPATHLEN);
  if(cwd == NULL)
    perror("[ERROR]");

  printf("\n%s@myshell%s> ", user, cwd);
  free(cwd);
}
