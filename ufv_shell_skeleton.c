#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>


/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Built-in command given as an example */
int cmd_exit(struct tokens *tokens);


/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

/* Table to include built-in commands */
fun_desc_t cmd_table[] = {
  {cmd_exit, "exit", "exit the command shell"},
};

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}



int run_program_thru_path(char *prog, char *args[]) {
  char *PATH = getenv("PATH");
  if (PATH == NULL) {
    return -1;
  }
  char prog_path[4096];
  char *path_dir = strtok(PATH, ":");
  // continue ...
  // ...
  // ...
}


int run_program(struct tokens *tokens) {
  int length = tokens_get_length(tokens);
  if (length == 0) {
    // user pressed return
    return 0;
  }
  int pid = fork();
  int status = 0;
  // continue ...
  // ...
  // ...
  // ...

    // execute new program in child process, searching thru path env var if needed
    if (execv(prog, args) == -1 && run_program_thru_path(prog, args) == -1) {
      printf("Error executing program %s\n", prog);
      exit(-1);
    }
  } 
  else {
    waitpid(pid, &status, 0);
  }
  return status;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}



int main(unused int argc, unused char *argv[]) {

  char line[4096];
  char *prompt= "ufv";

  /* Print shell prompt */
    fprintf(stdout, "%s: ", prompt);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);  // Note that all tokens are passed to the function that implements the built-in command
    } else {
      /* REPLACE this to run commands as programs. */
      // ...
    }

   
      /* shell prompts */
      fprintf(stdout, "%s: ", prompt);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}