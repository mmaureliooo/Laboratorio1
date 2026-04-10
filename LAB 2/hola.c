// Importa definiciones para la gestión de errores del sistema (variable errno)
#include <errno.h>
// Importa funciones de entrada/salida estándar (printf, fprintf, fgets, perror)
#include <stdio.h>
// Importa funciones de la biblioteca estándar de utilidades (exit, getenv)
#include <stdlib.h>
// Importa funciones para la manipulación de cadenas de caracteres (strcmp, strtok)
#include <string.h>
// Importa definiciones de tipos de datos del sistema (como pid_t)
#include <sys/types.h>
// Importa funciones para el manejo y captura de señales del sistema
#include <signal.h>
// Importa macros y funciones para el control de procesos (waitpid)
#include <sys/wait.h>
// Importa la API POSIX para llamadas al sistema (fork, execv, chdir, getcwd)
#include <unistd.h>
// Importa el archivo de cabecera local para el análisis léxico de la entrada (tokenización)
#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
// Define una macro 'unused' que aplica un atributo del compilador para suprimir advertencias de variables declaradas pero no utilizadas
#define unused __attribute__((unused))


/* Built-in command given as an example */
// Declara la firma de la función para el comando interno 'exit'
int cmd_exit(struct tokens *tokens);
// Declara la firma de la función para el comando interno 'pwd'
int cmd_pwd(struct tokens *tokens);
// Declara la firma de la función para el comando interno 'cd'
int cmd_cd(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
// Define 'cmd_fun_t' como un alias de tipo para un puntero a función que toma un 'struct tokens*' y devuelve un int
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
// Define una estructura y le asigna el alias 'fun_desc_t' para mapear comandos
typedef struct fun_desc {
  // Puntero a la función en C que implementa la lógica del comando
  cmd_fun_t *fun;
  // Cadena de caracteres que representa el identificador del comando (ej. "cd")
  char *cmd;
  // Cadena de caracteres con la documentación o descripción de la acción del comando
  char *doc;
} fun_desc_t;

/* Table to include built-in commands */
// Inicializa un arreglo global estático de estructuras 'fun_desc_t' que actúa como tabla de despacho
fun_desc_t cmd_table[] = {
  // Enlaza la función cmd_exit al string "exit"
  {cmd_exit, "exit", "exit the command shell"},
  // Enlaza la función cmd_pwd al string "pwd"
  {cmd_pwd, "pwd", "prints path to working directory"},
  // Enlaza la función cmd_cd al string "cd"
  {cmd_cd, "cd", "changes working directory to arg"},
};

/* Exits this shell */
// Define la implementación de 'cmd_exit'. Utiliza la macro 'unused' ya que no requiere leer los tokens
int cmd_exit(unused struct tokens *tokens) {
  // Llama a la función del sistema 'exit' con código 0 para finalizar el proceso actual (la shell)
  exit(0);
}


/* print working dir */
// Define la implementación de 'cmd_pwd'. Utiliza la macro 'unused'
int cmd_pwd(unused struct tokens *tokens){
  // Reserva estáticamente en la pila un arreglo de 4096 bytes para almacenar la ruta
  char cwd[4096];

  // Ejecuta la llamada al sistema 'getcwd' para escribir la ruta absoluta actual en 'cwd'
  // Si devuelve NULL, indica que la llamada al sistema falló
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
      // Imprime el string "pwd" seguido de la descripción del error reportado por 'errno'
      perror("pwd");
      // Retorna 1 al llamador indicando un estado de salida fallido
      return 1;
  // Si 'getcwd' fue exitoso
  } else {
    // Imprime el contenido del buffer 'cwd' seguido de un salto de línea en salida estándar
    printf("%s\n", cwd);
    // Retorna 0 indicando ejecución exitosa
    return 0;

  }

}

// Define la implementación de 'cmd_cd'. La macro 'unused' aquí es semánticamente incorrecta dado que 'tokens' sí se utiliza en el cuerpo
int cmd_cd(unused struct tokens *tokens) {
  // Extrae el puntero al segundo token (índice 1) asumiendo que contiene el directorio objetivo
  char *dir = tokens_get_token(tokens, 1);

  // Evalúa si la extracción resultó en un puntero nulo (el usuario no proporcionó ruta)
  if (dir == NULL) {
      // Imprime un mensaje indicando la ausencia del argumento requerido
      printf("cd: missing argument\n");
      // Retorna 1 indicando un estado de salida fallido
      return 1;
  }

  // Ejecuta la llamada al sistema 'chdir' para modificar el directorio de trabajo del proceso
  // Si retorna un valor distinto de 0, la operación de cambio de directorio falló
  if (chdir(dir) != 0) {
      // Imprime el string "cd" seguido de la descripción del error (ej. "No such file or directory")
      perror("cd");
      // Retorna 1 indicando fallo en la ejecución
      return 1;
  }

  // Retorna 0 si el cambio de directorio se completó satisfactoriamente
  return 0;
}


// Función encargada de iterar sobre las rutas del sistema para ejecutar un binario
int run_program_thru_path(char *prog, char *args[]) {
  // Extrae la cadena correspondiente a la variable de entorno "PATH"
  char *PATH = getenv("PATH");
  // Si la variable no está definida en el entorno, retorna -1
  if (PATH == NULL) {
    return -1;
  }
  // Reserva 4096 bytes en la pila para ensamblar las rutas absolutas candidatas
  char prog_path[4096];
  // Utiliza 'strtok' para extraer la primera subcadena de PATH delimitada por dos puntos (":")
  char *path_dir = strtok(PATH, ":");
  // continue ...
  // ...
  // ...
//para correr un comando sin poner la ruta antes iteramos por las rutas hasta llegar al :/bin/... o a null
  // Inicia un bucle que se ejecutará mientras 'strtok' continúe devolviendo punteros válidos
  while(path_dir != NULL){
//añadimos barra entre medias del PATH y el token[0], que es el comando, con sprintf
    // Concatena el directorio extraído, un carácter '/', y el nombre del ejecutable dentro de 'prog_path'
    sprintf(prog_path, "%s/%s", path_dir, prog);
//intentamos ejecutar el nuevo path de la línea anterior
    // Invoca 'execv' para reemplazar la imagen del proceso con el binario en 'prog_path' utilizando 'args'
    // Si la llamada tiene éxito, el proceso se sobrescribe y la función nunca retorna de esta línea
    execv(prog_path, args);
//pasa al siguiente porque le pasamos null (ya que recuerda dónde se quedó) /home/hola/bin -> /hola/bin, hasta llegar a null, que acabaría el while
    // Si 'execv' retorna, significa que falló. Se llama a 'strtok' con NULL para extraer la siguiente ruta del estado guardado internamente
    path_dir = strtok(NULL, ":");

  }
  // Omisión: La función en C carece de un retorno explícito al finalizar el bucle (debería retornar -1)
}


// Función principal para la creación de procesos y ejecución de binarios externos
int run_program(struct tokens *tokens) {
  // Obtiene la cantidad total de tokens generados a partir de la entrada del usuario
  int length = tokens_get_length(tokens);
  // Evalúa si el contador es 0 (línea vacía o solo espacios en blanco)
  if (length == 0) {
    // user pressed return
    // Retorna 0 para continuar el flujo sin ejecutar acciones
    return 0;
  }
//nueva parte, crear prog y args como token 0 el comanodo y el resto de args
  //el prog es el primer token
  // Asigna el primer token (índice 0) al puntero 'prog', representando el binario objetivo
  char *prog = tokens_get_token(tokens, 0);
//reservamos memoria para args +1 ya que necesitamos el espacio para el NULL
  // Declara un arreglo de longitud variable (VLA) de punteros a char para estructurar los argumentos, sumando 1 para el terminador nulo
  char *args[length + 1];
//copiamos tokens al array de argumentos
  // Itera secuencialmente sobre todos los tokens existentes
  for (int i = 0; i < length; i++) {
      // Copia el puntero de cada token a la posición correspondiente en el arreglo de argumentos
      args[i] = tokens_get_token(tokens, i);
  }
//el último dato del array lo ponemos a null para el exec
  // Asigna NULL al índice final del arreglo, cumpliendo el requisito estricto de finalización de argumentos para la familia exec()
  args[length] = NULL;
  // Ejecuta la llamada al sistema 'fork' para duplicar el proceso de la shell actual. Guarda el valor de retorno en 'pid'
  int pid = fork();
  // Inicializa a cero la variable que almacenará el código de salida del subproceso
  int status = 0;
  // continue ...
  // ...
  // ...
  // ...

//ponemos que lo hace solo el proceso hijo
// Evalúa si el código se está ejecutando en el contexto del proceso hijo (donde fork() devuelve 0)
if (pid == 0) {

    // execute new program in child process, searching thru path env var if needed
    // Evaluación de cortocircuito:
    // 1. execv intenta ejecutar la ruta exacta proporcionada por el usuario. Si falla (retorna -1), prosigue.
    // 2. run_program_thru_path intenta buscar y ejecutar en el PATH. Si falla (retorna -1), entra al bloque.
    if (execv(prog, args) == -1 && run_program_thru_path(prog, args) == -1) {
      // Imprime el error notificando que la ejecución de dicho binario fue imposible
      printf("Error executing program %s\n", prog);
      // Finaliza el proceso hijo inmediatamente devolviendo un estado de error (-1) al sistema operativo
      exit(-1);
    }
  }
  // Bloque que se ejecuta exclusivamente en el contexto del proceso padre (la shell base)
  else {
    // Bloquea la ejecución del proceso padre hasta que el proceso hijo identificado por 'pid' cambie de estado o termine, guardando el resultado en 'status'
    waitpid(pid, &status, 0);
  }
  // Retorna el valor de estado capturado del proceso hijo
  return status;
}

/* Looks up the built-in command, if it exists. */
// Función que verifica si una cadena corresponde a un comando interno definido
int lookup(char cmd[]) {
  // Itera según el cociente entre el tamaño total en bytes del arreglo de comandos y el tamaño de una de sus estructuras
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    // Condición combinada: comprueba que el puntero 'cmd' no sea nulo y que su contenido sea idéntico a una entrada en la tabla
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      // Retorna la posición (índice entero) de la coincidencia en el arreglo
      return i;
  // Retorna -1 si finaliza el bucle sin encontrar cadenas coincidentes
  return -1;
}



// Función principal de entrada al ejecutable. Los parámetros argc y argv se marcan como no utilizados
int main(unused int argc, unused char *argv[]) {

  // Reserva un buffer de 4096 bytes en la pila del frame actual para alojar la línea cruda de texto de entrada
  char line[4096];
  // Define un puntero estático con la cadena que actúa como identificador visual del prompt
  char *prompt= "ufv-Edamantio2";

  /* Print shell prompt */
    // Escribe la cadena del prompt formateada hacia el descriptor de archivo de salida estándar
    fprintf(stdout, "%s: ", prompt);

  // Inicia un bucle de bloqueo que lee datos de la entrada estándar hasta encontrar un carácter de nueva línea, EOF, o llenar el buffer
  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    // Llama al analizador léxico externo pasándole el buffer de lectura para que construya la estructura de tokens
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    // Extrae el token índice 0 y lo envía a la función de búsqueda para determinar si es un comando interno, guardando el índice de retorno
    int fundex = lookup(tokens_get_token(tokens, 0));

    // Evalúa si el índice devuelto es válido (perteneciente a la tabla de comandos integrados)
    if (fundex >= 0) {
      // Utiliza el índice para acceder al puntero de función dentro del arreglo y lo ejecuta pasándole la estructura de tokens
      cmd_table[fundex].fun(tokens);  // Note that all tokens are passed to the function that implements the built-in command
    // Bloque de control en caso de que la búsqueda arrojara -1 (comando no interno)
    } else {
      /* REPLACE this to run commands as programs. */
      // ...
  //hacemos que corra el comando desde el execv del run_program
      // Transfiere el flujo a la función encargada de inicializar la lógica de procesos y ejecución externa
      run_program(tokens);
    }


      /* shell prompts */
      // Imprime nuevamente el identificador visual de la shell para preparar la recepción de la siguiente orden
      fprintf(stdout, "%s: ", prompt);

    /* Clean up memory */
    // Invoca la función externa encargada de liberar toda la memoria heap reservada durante el proceso de tokenización
    tokens_destroy(tokens);
  }

  // Define la salida exitosa del proceso principal (alcanzada si el bucle while se rompe por EOF)
  return 0;
}