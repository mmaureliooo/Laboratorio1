#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))


/* Built-in command given as an example */
int cmd_exit(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);

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
  {cmd_pwd, "pwd", "prints path to working directory"},
  {cmd_cd, "cd", "changes working directory to arg"},
};

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}


/* print working dir */
int cmd_pwd(unused struct tokens *tokens){
  char cwd[4096];

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
      perror("pwd");
      return 1;
  } else {
    printf("%s\n", cwd);
    return 0;

  }

}

int cmd_cd(unused struct tokens *tokens) {
 char *dir = tokens_get_token(tokens, 1);

  if (dir == NULL) {
      printf("cd: missing argument\n");
      return 1;
  }

  if (chdir(dir) != 0) {
      perror("cd");
      return 1;
  }

  return 0;
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
//para correr un comando sin poner la ruta antes iteramos por las rutas hasta llegar al :/bin/... o a null
  while(path_dir != NULL){
//añadimos barra entre medias del PATH y el token[0], que es el comando, con sprintf
    sprintf(prog_path, "%s/%s", path_dir, prog);
//intentamos ejecutar el nuevo path de la línea anterior
    execv(prog_path, args);
//pasa al siguiente porque le pasamos null (ya que recuerda dónde se quedó) /home/hola/bin -> /hola/bin, hasta llegar a null, que acabaría el while
    path_dir = strtok(NULL, ":");

  }
}


int run_program(struct tokens *tokens) {
  int length = tokens_get_length(tokens);
  if (length == 0) {
    // user pressed return
    return 0;
  }
//nueva parte, crear prog y args como token 0 el comanodo y el resto de args
  //el prog es el primer token
  char *prog = tokens_get_token(tokens, 0);
//reservamos memoria para args +1 ya que necesitamos el espacio para el NULL
  char *args[length + 1];
//copiamos tokens al array de argumentos
  for (int i = 0; i < length; i++) {
      args[i] = tokens_get_token(tokens, i);
  }
//el último dato del array lo ponemos a null para el exec
  args[length] = NULL;
  int pid = fork();
  int status = 0;
  // continue ...
  // ...
  // ...
  // ...

//ponemos que lo hace solo el proceso hijo
if (pid == 0) {

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
  char *prompt= "ufv-Edamantio2";

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
  //hacemos que corra el comando desde el execv del run_program
      run_program(tokens);
    }


      /* shell prompts */
      fprintf(stdout, "%s: ", prompt);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
