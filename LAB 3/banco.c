#include "banco_comun.h"
#include <sys/stat.h>

/* Variables globales */
static mqd_t g_mq_log     = -1;
static mqd_t g_mq_alerta  = -1;
static mqd_t g_mq_monitor = -1;

static pid_t g_monitor_pid = -1;

#define MAX_HIJOS 64
typedef struct {
    pid_t pid;
    int   cuenta_id;
    int   pipe_wr;
} InfoHijo;
static InfoHijo g_hijos[MAX_HIJOS];
static int      g_num_hijos = 0;

static Config g_cfg;
static volatile sig_atomic_t g_salir = 0;

/* Señales */
static void manejador_sigterm(int s) { (void)s; g_salir = 1; }

/* Leer config.txt  */
static int leer_config(const char *ruta, Config *cfg) {
    FILE *f = fopen(ruta, "r");
    if (!f) { perror("fopen config"); return -1; }
    //
    // Completar
    //
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        /* Ignorar comentarios (#) y líneas vacías */
        if (line[0] == '#' || line[0] == '\n') continue;
        char key[128], val[128];
        /* %127[^=] captura todo hasta el '=' como clave; %127s captura el valor hasta el primer espacio o fin de línea */
        //tiene que ser 2 porque es clave y valor
        if (sscanf(line, "%127[^=]=%127s", key, val) != 2) continue;

        /* cada clave y valor a su correspondiente dato de la estructura Config */
        //usamos atoi/atof por simplicidad; si no es un valor int/float devuelve 0 y ya.
        if      (strcmp(key, "PROXIMO_ID")            == 0) cfg->proximo_id           = atoi(val);
        else if (strcmp(key, "LIM_RET_EUR")           == 0) cfg->lim_ret_eur          = atof(val);
        else if (strcmp(key, "LIM_RET_USD")           == 0) cfg->lim_ret_usd          = atof(val);
        else if (strcmp(key, "LIM_RET_GBP")           == 0) cfg->lim_ret_gbp          = atof(val);
        else if (strcmp(key, "LIM_TRF_EUR")           == 0) cfg->lim_trf_eur          = atof(val);
        else if (strcmp(key, "LIM_TRF_USD")           == 0) cfg->lim_trf_usd          = atof(val);
        else if (strcmp(key, "LIM_TRF_GBP")           == 0) cfg->lim_trf_gbp          = atof(val);
        else if (strcmp(key, "UMBRAL_RETIROS")        == 0) cfg->umbral_retiros        = atoi(val);
        else if (strcmp(key, "UMBRAL_TRANSFERENCIAS") == 0) cfg->umbral_transferencias = atoi(val);
        else if (strcmp(key, "NUM_HILOS")             == 0) cfg->num_hilos             = atoi(val);
        else if (strcmp(key, "ARCHIVO_CUENTAS")       == 0) strncpy(cfg->archivo_cuentas, val, MAX_PATH-1);
        else if (strcmp(key, "ARCHIVO_LOG")           == 0) strncpy(cfg->archivo_log,     val, MAX_PATH-1);
        else if (strcmp(key, "CAMBIO_USD")            == 0) cfg->cambio_usd            = atof(val);
        else if (strcmp(key, "CAMBIO_GBP")            == 0) cfg->cambio_gbp            = atof(val);
    }

    fclose(f);
    return 0;
}

/* Actualizar PROXIMO_ID en config.txt */
static void guardar_proximo_id(int nuevo_id) {
    FILE *f = fopen("config.txt", "r");
    if (!f) return;
    char contenido[4096];
    size_t n = fread(contenido, 1, sizeof(contenido)-1, f);
    contenido[n] = '\0';
    fclose(f);
    char *p = strstr(contenido, "PROXIMO_ID=");
    if (!p) return;
    size_t antes     = (size_t)(p - contenido);
    char  *fin_linea = strchr(p, '\n');

    char nuevo[4096];
    snprintf(nuevo, sizeof(nuevo), "%.*sPROXIMO_ID=%d%s",
             (int)antes, contenido, nuevo_id,
             fin_linea ? fin_linea : "");
    f = fopen("config.txt", "w");
    if (!f) return;
    fputs(nuevo, f);
    fclose(f);
}

/* Buscar cuenta */
static int buscar_cuenta(int numero, Cuenta *c) {
    FILE *f = fopen(g_cfg.archivo_cuentas, "rb");
    if (!f) return 0;
    int ok = 0;
    while (fread(c, sizeof(Cuenta), 1, f) == 1)
        if (c->numero_cuenta == numero) { ok = 1; break; }
    fclose(f);
    return ok;
}

/* Crear nueva cuenta */
static int crear_cuenta(Cuenta *nueva) {
    sem_t *sc = sem_open(SEM_CONFIG, 0);
    //
    // Completar
    //

    // Sección crítica: leer PROXIMO_ID, incrementarlo y guardarlo en config.txt
    if (sc == SEM_FAILED) { perror("sem_open SEM_CONFIG"); return -1; }
    sem_wait(sc);//restar al sem
    // Re-leemos config dentro del semáforo para obtener el valor más reciente:
    // otro proceso pudo haber incrementado PROXIMO_ID entre nuestro arranque y ahora
    if (leer_config("config.txt", &g_cfg) < 0) {
        sem_post(sc); sem_close(sc); return -1; //cerrar semaforo local si falla
    }
    nueva->numero_cuenta = g_cfg.proximo_id;   // asignar ID actual
    g_cfg.proximo_id++;                        // preparar el siguiente
    guardar_proximo_id(g_cfg.proximo_id);      // persistir en disco antes de liberar
    sem_post(sc);
    sem_close(sc); //cerrar semaforo local

    sem_t *sa = sem_open(SEM_CUENTAS, 0);

    // Sección crítica: añadir la nueva cuenta al final del fichero binario
    if (sa == SEM_FAILED) { perror("sem_open SEM_CUENTAS"); return -1; } //salir si falla
    sem_wait(sa);//restamos al sem
    // en ab el puntero siempre va al final, seguro aunque otro proceso tenga el fichero abierto
    FILE *f = fopen(g_cfg.archivo_cuentas, "ab");
    if (!f) { sem_post(sa); sem_close(sa); return -1; } // si falla cerramos y retornamos
    fwrite(nueva, sizeof(Cuenta), 1, f); // reescribimos la nueva cuenta
    fclose(f);//cerramos fichero
    sem_post(sa);
    sem_close(sa);

    printf("Cuenta %d creada para '%s'.\n", nueva->numero_cuenta, nueva->titular);
    return nueva->numero_cuenta;
}

/* Escribir en el log */
static void escribir_log(const char *linea) {
    FILE *f = fopen(g_cfg.archivo_log, "a");
    //
    // Completar
    //

    // Abrir en modo append para no sobreescribir entradas anteriores
    if (!f) return;
    fprintf(f, "%s\n", linea); //escribir la nueva linea de log como apendice
    fclose(f);//cerrar
}

/* Buscar hijo por cuenta_id */
static InfoHijo *buscar_hijo(int cuenta_id) {
    for (int i = 0; i < g_num_hijos; i++)
        if (g_hijos[i].cuenta_id == cuenta_id) return &g_hijos[i];
    return NULL;
}

/* Procesar mensajes de log pendientes en MQ_LOG */
static void procesar_log(void) {
    DatosLog dl;
    while (mq_receive(g_mq_log, (char *)&dl, sizeof(dl), NULL) > 0) {
        //
        // Completar
        //

        // Convertir tipo_op a cadena legible para la línea de log
        const char *tipo_str;
        switch (dl.tipo_op) {
            case OP_DEPOSITO:      tipo_str = "Deposito";             break;
            case OP_RETIRO:        tipo_str = "Retiro";               break;
            case OP_TRANSFERENCIA: tipo_str = "Transferencia";        break;
            case OP_MOVER_DIVISA:  tipo_str = "Movimiento de divisa"; break;
            default:               tipo_str = "Operacion";            break;
        }
        // Formato final en linea: [YYYY-MM-DD HH:MM:SS] Operacion en cuenta ID: cantidad DIV - OK|FALLIDO
        char linea[512];
        snprintf(linea, sizeof(linea),
                 "[%s] %s en cuenta %d: %.2f %s - %s",
                 dl.timestamp, tipo_str, dl.cuenta_id,
                 dl.cantidad,  nombre_divisa(dl.divisa),
                 dl.estado == 0 ? "OK" : "FALLIDO");
        escribir_log(linea);
    }
}

/* Procesar alertas pendientes en MQ_ALERTA */
static void procesar_alertas(void) {
    DatosAlerta da;
    while (mq_receive(g_mq_alerta, (char *)&da, sizeof(da), NULL) > 0) {
        char ts[MAX_TS]; timestamp_ahora(ts, sizeof(ts));
        char linea[512];
        int bloquear = (strstr(da.mensaje, ALERTA_RETIROS)        != NULL ||
                        strstr(da.mensaje, ALERTA_TRANSFERENCIAS) != NULL);
        snprintf(linea, sizeof(linea), "[%s] ALERTA: %s - cuenta %s",
                 ts, da.mensaje, bloquear ? "BLOQUEADA" : "monitoreada");
        escribir_log(linea);

        if (bloquear) {
            InfoHijo *h = buscar_hijo(da.cuenta_id);
            if (h && h->pipe_wr != -1) {
                char aviso[512];
                snprintf(aviso, sizeof(aviso),
                         "BLOQUEO: %s en cuenta %d", da.mensaje, da.cuenta_id);
                write(h->pipe_wr, aviso, strlen(aviso)+1);
                close(h->pipe_wr);
                h->pipe_wr = -1;
            }
        }
    }
}

/* Lanzar proceso usuario */
static pid_t lanzar_usuario(int cuenta_id, int *pipe_wr_out) {
    int pfd[2];
    if (pipe(pfd) < 0) { perror("pipe"); return -1; }

    pid_t pid = fork();
    //
    // Completar
    //
    if (pid < 0) { //por si acaso falla
        perror("fork usuario");
        close(pfd[0]); close(pfd[1]);
        return -1;
    }
    if (pid == 0) {
        // Proceso hijo: cerrar extremo de escritura del pipe
        close(pfd[1]);
        // Cerrar colas del padre (el hijo abrirá los suyos)
        if (g_mq_log     != (mqd_t)-1) mq_close(g_mq_log);
        if (g_mq_alerta  != (mqd_t)-1) mq_close(g_mq_alerta);
        if (g_mq_monitor != (mqd_t)-1) mq_close(g_mq_monitor);
        // Pasar cuenta_id y extremo de lectura del pipe como argumentos
        char s_cuenta[16], s_pipe[16];
        snprintf(s_cuenta, sizeof(s_cuenta), "%d", cuenta_id);
        snprintf(s_pipe,   sizeof(s_pipe),   "%d", pfd[0]);
        char *args[] = { "./usuario", s_cuenta, s_pipe, NULL };
        execv("./usuario", args);
        perror("execv usuario");
        _exit(1); //que no hace flush
    }
    // Proceso padre: conservar solo el extremo de escritura, si no hijo nunca eof
    close(pfd[0]);
    *pipe_wr_out = pfd[1]; //devolver el pipe de este usuario al padre por si se necesita bloquear
    return pid;  //devolver pid
}

/* Lanzar proceso monitor */
static void lanzar_monitor(void) {
    //
    // Completar
    //
    pid_t pid = fork();
    if (pid < 0) { perror("fork monitor"); return; }
    if (pid == 0) {
        // Proceso hijo (monitor): cerrar las colas del padre
        if (g_mq_log     != (mqd_t)-1) mq_close(g_mq_log);
        if (g_mq_alerta  != (mqd_t)-1) mq_close(g_mq_alerta);
        if (g_mq_monitor != (mqd_t)-1) mq_close(g_mq_monitor);
        char *args[] = { "./monitor", NULL };
        execv("./monitor", args);
        perror("execv monitor");
        _exit(1);// que no hace flush
    }
    // Guardar PID del monitor para enviarle SIGTERM al cierre
    g_monitor_pid = pid;
}


int main(void) {
    signal(SIGTERM, manejador_sigterm);
    signal(SIGINT,  manejador_sigterm);
    signal(SIGCHLD, SIG_DFL);

    if (leer_config("config.txt", &g_cfg) < 0) {
        fprintf(stderr, "No se pudo leer config.txt\n"); return 1;
    }

    sem_unlink(SEM_CUENTAS); sem_unlink(SEM_CONFIG);
    sem_t *sem_c = sem_open(SEM_CUENTAS, O_CREAT|O_EXCL, 0600, 1);
    sem_t *sem_g = sem_open(SEM_CONFIG,  O_CREAT|O_EXCL, 0600, 1);
    if (sem_c==SEM_FAILED || sem_g==SEM_FAILED) { perror("sem_open inicial"); return 1; }
    sem_close(sem_c); sem_close(sem_g);

    int fd = open(g_cfg.archivo_cuentas, O_CREAT|O_RDWR, 0644);
    if (fd >= 0) close(fd);

    /* Crear las tres colas POSIX en modo no bloqueante para poder
     * drenarlas con mq_receive en un bucle sin quedarse colgado    */
    struct mq_attr attr; //struct de atributos en modo no block
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_flags  = O_NONBLOCK;

    mq_unlink(MQ_MONITOR); mq_unlink(MQ_LOG); mq_unlink(MQ_ALERTA);

    attr.mq_msgsize = sizeof(DatosMonitor);
    g_mq_monitor = mq_open(MQ_MONITOR, O_CREAT|O_RDWR|O_NONBLOCK, 0666, &attr);

    attr.mq_msgsize = sizeof(DatosLog);
    g_mq_log     = mq_open(MQ_LOG,     O_CREAT|O_RDWR|O_NONBLOCK, 0666, &attr);

    attr.mq_msgsize = sizeof(DatosAlerta);
    g_mq_alerta  = mq_open(MQ_ALERTA,  O_CREAT|O_RDWR|O_NONBLOCK, 0666, &attr);

    if (g_mq_monitor==-1 || g_mq_log==-1 || g_mq_alerta==-1) {
        perror("mq_open"); return 1;
    }

    lanzar_monitor();

    printf("\n+==============================+\n");
    printf("|    SecureBank  --  Login     |\n");
    printf("+==============================+\n");

    /* poll sobre stdin, MQ_LOG y MQ_ALERTA directamente */
    struct pollfd pfds[3];
    pfds[0].fd = STDIN_FILENO; pfds[0].events = POLLIN;
    pfds[1].fd = g_mq_log;     pfds[1].events = POLLIN;
    pfds[2].fd = g_mq_alerta;  pfds[2].events = POLLIN;

    while (!g_salir) {

        printf("\nIntroduzca numero de cuenta (0=nueva, -1=salir): ");
        fflush(stdout);

        int ret = poll(pfds, 3, -1);
        if (ret < 0) { if (errno==EINTR) continue; break; }

        if (pfds[1].revents & POLLIN) procesar_log();
        if (pfds[2].revents & POLLIN) procesar_alertas();
        if (!(pfds[0].revents & POLLIN)) continue;

        int numero;
        if (scanf("%d", &numero) != 1) {
            int c; while ((c=getchar())!='\n'&&c!=EOF);
            continue;
        }
        if (numero == -1) { g_salir = 1; break; }

        Cuenta cuenta;
        memset(&cuenta, 0, sizeof(cuenta));

        if (numero == 0) {
            printf("Nombre del titular: "); fflush(stdout);
            if (scanf(" %49[^\n]", cuenta.titular) != 1)
                strcpy(cuenta.titular, "Desconocido");
            cuenta.saldo_eur = cuenta.saldo_usd = cuenta.saldo_gbp = 0.0f;
            int id = crear_cuenta(&cuenta);
            if (id < 0) { fprintf(stderr, "Error al crear cuenta.\n"); continue; }
            numero = id;
        } else {
            if (!buscar_cuenta(numero, &cuenta)) {
                printf("Cuenta %d no encontrada.\n", numero);
                continue;
            }
            printf("Bienvenido, %s (cuenta %d)\n", cuenta.titular, numero);
        }

        int pipe_wr;
        pid_t pid = lanzar_usuario(numero, &pipe_wr);
        if (pid < 0) continue;

        g_hijos[g_num_hijos].pid       = pid;
        g_hijos[g_num_hijos].cuenta_id = numero;
        g_hijos[g_num_hijos].pipe_wr   = pipe_wr;
        g_num_hijos++;

        /* Fase sesión: poll sobre las colas y waitpid mientras el hijo vive */
        struct pollfd pfd_ses[2];
        pfd_ses[0].fd = g_mq_log;    pfd_ses[0].events = POLLIN;
        pfd_ses[1].fd = g_mq_alerta; pfd_ses[1].events = POLLIN;

        while (!g_salir) {
            poll(pfd_ses, 2, 200); //timeout cada 200ms

            if (pfd_ses[0].revents & POLLIN) procesar_log();
            if (pfd_ses[1].revents & POLLIN) procesar_alertas();

            int wst;
            if (waitpid(pid, &wst, WNOHANG) == pid) {
                if (pipe_wr != -1) { close(pipe_wr); pipe_wr = -1; }
                for (int i = 0; i < g_num_hijos; i++) {
                    if (g_hijos[i].pid == pid) {
                        g_hijos[i] = g_hijos[--g_num_hijos];
                        break;
                    }
                }
                break;
            }
        }
    }

    /* Cierre */
    g_salir = 1;

    for (int i = 0; i < g_num_hijos; i++) {
        if (g_hijos[i].pipe_wr != -1) close(g_hijos[i].pipe_wr);
        waitpid(g_hijos[i].pid, NULL, 0);
    }
    if (g_monitor_pid > 0) {
        kill(g_monitor_pid, SIGTERM);
        waitpid(g_monitor_pid, NULL, 0);
    }

    mq_close(g_mq_monitor); mq_unlink(MQ_MONITOR);
    mq_close(g_mq_log);     mq_unlink(MQ_LOG);
    mq_close(g_mq_alerta);  mq_unlink(MQ_ALERTA);
    sem_unlink(SEM_CUENTAS);
    sem_unlink(SEM_CONFIG);
    return 0;
}
