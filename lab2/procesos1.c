//#include <stdio.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/wait.h>

//int main(int argc, char* argv[]) {
 //   int id= fork();
 //   int n;
 //   if (id == 0){
 //     n=1;
 //   }else{
 //     n=6;
 //   }
 //   if(id != 0){
//      wait(NULL);
 //   }
//    int i;
//    for (i=n; i<n+5; i++){
 //     printf("%d ",i);
 //     fflush(stdout);
//    }
//    if(id != 0){
//          printf("\n");
//        }
//    return 0;
//}
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
int main (void) {
pid_t id1;
pid_t id2;
printf("soy el padre:%d\n",getpid());
id1=fork();
if(id1==0){
printf("soy el hijo:%d y mi padre es:%d\n",getpid(),getppid());
}
if(id1==0){
id2=fork();
}

if(id2==0){
printf("soy el nieto:%d y mi padre es %d\n",getpid(),getppid());
}


if(id1!=0){
wait(NULL);
  printf("el hijo ha terminado, padre continua\n");
  exit(0);

}

if(id2!=0){
wait(NULL);
  printf("el nieto ha terminado,hijo continua\n");
  exit(0);

}
return 0;
}
gcc procesos1.c -o procesos1
