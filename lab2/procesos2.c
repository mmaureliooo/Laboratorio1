#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {

    int tubo[2];


    if (fork() == 0) {
      pipe(tubo);
        if (fork() == 0) {
            dup2(tubo[0], STDIN_FILENO);
            close(tubo[1]);
            close(tubo[0]);
            execlp("wc", "wc", "-l", NULL);

        }
        else {
            dup2(tubo[1], STDOUT_FILENO);
            close(tubo[1]);
            close(tubo[0]);
            execlp("ls", "ls", NULL);
        }

    }
    else {
        wait(NULL);
        printf("Fin del proceso\n");
    }

    return 0;
}
