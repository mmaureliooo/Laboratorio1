#include "banco_comun.h"

/* Variables globales del proceso hijo */
static mqd_t  g_mq_monitor = (mqd_t)-1;
static mqd_t  g_mq_log     = (mqd_t)-1;
static int    g_pipe_rd    = -1;
static int    g_cuenta_id  =  0;
static Config g_cfg;

/* Leer config (solo lectura) */
static void leer_config_usuario(void) {
    FILE *f = fopen("config.txt", "r");
    //
    // Completar
    //
    fclose(f);
}

/* Semáforo: abrir con comprobación */
static sem_t *abrir_sem_cuentas(void) {
    sem_t *s = sem_open(SEM_CUENTAS, 0);
    if (s == SEM_FAILED) { perror("sem_open SEM_CUENTAS"); return NULL; }
    return s;
}

/*
E2C940E3C540D7C9C4C5D540D8E4C540C3D6D5E2C5D9E5C5E240C5D340C5E2D8E4C5D3C5E3D640C4C540C5E2E3C540D7D9D6C7D9C1D4C140C9D5E3C1C3E3D66B40D5D640C8C1C7C1E240C3C1E2D66B40D4D6C4C9C6C9C3C140E2C9C5D4D7D9C540C5D340E3C5E7E3D640C5D540C5C2C3C4C9C340E2E4D4C1D5C4D640F140C1D340C4C5C3C9D4D640C3D1D9C1C3E3C5D940C5D540C5C2C3C4C9C34BE2C940E3C540C4C9C3C5D540D8E4C540D7D6D940D8E4C540D3D640C8C1E240C3C1D4C2C9C1C4D640C4C940D8E4C540D5D640D3D640C8C1E240C3C1D4C2C9C1C4D64BD5D640C8C1C7C1E240D5C9D5C7D2D540C3D6C4C9C6C9C3C1C4D640C1D340C3D6C4C9C7D640D6C3E4D3E3D640C4C5D5E3D9D640C4C5D340C2D3D6D8E4C540C3D6C4C9C6C9C3C1C4D640C5D540C5C2C3C4C9C3
*/
/*  Leer cuenta por búsqueda lineal */
static int leer_cuenta(int id, Cuenta *c) {
    FILE *f = fopen(g_cfg.archivo_cuentas, "rb");
    if (!f) return -1;
    int ok = -1;
    while (fread(c, sizeof(Cuenta), 1, f) == 1)
        if (c->numero_cuenta == id) { ok = 0; break; }
    fclose(f);
    return ok;
}

/* Escribir cuenta por posición real en el fichero  */
static int escribir_cuenta(const Cuenta *c) {
    FILE *f = fopen(g_cfg.archivo_cuentas, "rb+");
    if (!f) return -1;
    Cuenta tmp;
    long pos = 0;
    int found = 0;
    while (fread(&tmp, sizeof(Cuenta), 1, f) == 1) {
        if (tmp.numero_cuenta == c->numero_cuenta) { found = 1; break; }
        pos += (long)sizeof(Cuenta);
    }
    if (!found) { fclose(f); return -1; }
    fseek(f, pos, SEEK_SET);
    int ok = (fwrite(c, sizeof(Cuenta), 1, f) == 1) ? 0 : -1;
    fflush(f);
    fclose(f);
    return ok;
}

/*
E2C940E3C540D7C9C4C5D540D8E4C540C3D6D5E2C5D9E5C5E240C5D340C5E2D8E4C5D3C5E3D640C4C540C5E2E3C540D7D9D6C7D9C1D4C140C9D5E3C1C3E3D66B40D5D640C8C1C7C1E240C3C1E2D66B40D4D6C4C9C6C9C3C140E2C9C5D4D7D9C540C5D340E3C5E7E3D640C5D540C5C2C3C4C9C340E2E4D4C1D5C4D640F140C1D340C4C5C3C9D4D640C3D1D9C1C3E3C5D940C5D540C5C2C3C4C9C34BE2C940E3C540C4C9C3C5D540D8E4C540D7D6D940D8E4C540D3D640C8C1E240C3C1D4C2C9C1C4D640C4C940D8E4C540D5D640D3D640C8C1E240C3C1D4C2C9C1C4D64BD5D640C8C1C7C1E240D5C9D5C7D2D540C3D6C4C9C6C9C3C1C4D640C1D340C3D6C4C9C7D640D6C3E4D3E3D640C4C5D5E3D9D640C4C5D340C2D3D6D8E4C540C3D6C4C9C6C9C3C1C4D640C5D540C5C2C3C4C9C3
*/
/* Enviar mensajes a las colas POSIX */
static void enviar_monitor(int cuenta_origen, int cuenta_destino,
                           int tipo_op, float cantidad, int divisa) {
    DatosMonitor dm;
    memset(&dm, 0, sizeof(dm));
    //
    // Completar
    //
    mq_send(g_mq_monitor, (const char *)&dm, sizeof(dm), 0);
}

static void enviar_log(int tipo_op, float cantidad, int divisa, int estado) {
    DatosLog dl;
    memset(&dl, 0, sizeof(dl));
    //
    // Completar
    //
}

/*
E2C940E3C540D7C9C4C5D540D8E4C540C3D6D5E2C5D9E5C5E240C5D340C5E2D8E4C5D3C5E3D640C4C540C5E2E3C540D7D9D6C7D9C1D4C140C9D5E3C1C3E3D66B40D5D640C8C1C7C1E240C3C1E2D66B40D4D6C4C9C6C9C3C140E2C9C5D4D7D9C540C5D340E3C5E7E3D640C5D540C5C2C3C4C9C340E2E4D4C1D5C4D640F140C1D340C4C5C3C9D4D640C3D1D9C1C3E3C5D940C5D540C5C2C3C4C9C34BE2C940E3C540C4C9C3C5D540D8E4C540D7D6D940D8E4C540D3D640C8C1E240C3C1D4C2C9C1C4D640C4C940D8E4C540D5D640D3D640C8C1E240C3C1D4C2C9C1C4D64BD5D640C8C1C7C1E240D5C9D5C7D2D540C3D6C4C9C6C9C3C1C4D640C1D340C3D6C4C9C7D640D6C3E4D3E3D640C4C5D5E3D9D640C4C5D340C2D3D6D8E4C540C3D6C4C9C6C9C3C1C4D640C5D540C5C2C3C4C9C3
*/
/* ════════════════════════════════════════════════════════════
 * Threads de operación bancaria
 * ════════════════════════════════════════════════════════════ */
typedef struct {
    int   tipo_op;
    int   cuenta_id;
    int   cuenta_destino;
    float cantidad;
    int   divisa_origen;
    int   divisa_destino;
} DatosOperacion;

/* Depósito  */
static void *thread_deposito(void *arg) {
    sem_t *sem = abrir_sem_cuentas();
    //
    // Completar
    //
}

/* Retiro */
static void *thread_retiro(void *arg) {

    //
    // Completar
    //
}

/*
E2C940E3C540D7C9C4C5D540D8E4C540C3D6D5E2C5D9E5C5E240C5D340C5E2D8E4C5D3C5E3D640C4C540C5E2E3C540D7D9D6C7D9C1D4C140C9D5E3C1C3E3D66B40D5D640C8C1C7C1E240C3C1E2D66B40D4D6C4C9C6C9C3C140E2C9C5D4D7D9C540C5D340E3C5E7E3D640C5D540C5C2C3C4C9C340E2E4D4C1D5C4D640F140C1D340C4C5C3C9D4D640C3D1D9C1C3E3C5D940C5D540C5C2C3C4C9C34BE2C940E3C540C4C9C3C5D540D8E4C540D7D6D940D8E4C540D3D640C8C1E240C3C1D4C2C9C1C4D640C4C940D8E4C540D5D640D3D640C8C1E240C3C1D4C2C9C1C4D64BD5D640C8C1C7C1E240D5C9D5C7D2D540C3D6C4C9C6C9C3C1C4D640C1D340C3D6C4C9C7D640D6C3E4D3E3D640C4C5D5E3D9D640C4C5D340C2D3D6D8E4C540C3D6C4C9C6C9C3C1C4D640C5D540C5C2C3C4C9C3
*/
/* Transferencia */
static void *thread_transferencia(void *arg) {
    
   
}

/* Mover divisas */
static void *thread_mover_divisa(void *arg) {
    
}

/* Lanzar thread */
static void lanzar_operacion(void *(*fn)(void*), DatosOperacion *d) {
    pthread_t t;
    pthread_create(&t, NULL, fn, d);
    pthread_join(t, NULL);
}

/* Consultar saldos */
static void consultar_saldos(void) {
    
}

/* Pedir divisa */
static int pedir_divisa(const char *prompt) {
    int d = -1;
    while (d < 0 || d > 2) {
        printf("%s (0=EUR, 1=USD, 2=GBP): ", prompt);
        fflush(stdout);
        if (scanf("%d", &d) != 1) {
            int c; while ((c=getchar())!='\n'&&c!=EOF);
            d = -1;
        }
    }
    return d;
}

/* Procesar opción del menú */
static void procesar_opcion(int opcion) {
    DatosOperacion *d = NULL;
    switch (opcion) {
    case 1:
        d = calloc(1, sizeof(DatosOperacion));
        d->tipo_op       = OP_DEPOSITO;
        d->cuenta_id     = g_cuenta_id;
        d->divisa_origen = pedir_divisa("Divisa");
        printf("Cantidad a depositar: "); fflush(stdout);
        scanf("%f", &d->cantidad);
        lanzar_operacion(thread_deposito, d);
        break;
    case 2:
        d = calloc(1, sizeof(DatosOperacion));
        d->tipo_op       = OP_RETIRO;
        d->cuenta_id     = g_cuenta_id;
        d->divisa_origen = pedir_divisa("Divisa");
        printf("Cantidad a retirar: "); fflush(stdout);
        scanf("%f", &d->cantidad);
        lanzar_operacion(thread_retiro, d);
        break;
    case 3:
        d = calloc(1, sizeof(DatosOperacion));
        d->tipo_op       = OP_TRANSFERENCIA;
        d->cuenta_id     = g_cuenta_id;
        printf("Cuenta destino: "); fflush(stdout);
        scanf("%d", &d->cuenta_destino);
        d->divisa_origen = pedir_divisa("Divisa");
        printf("Cantidad: "); fflush(stdout);
        scanf("%f", &d->cantidad);
        lanzar_operacion(thread_transferencia, d);
        break;
    case 4:
        consultar_saldos();
        break;
    case 5:
        d = calloc(1, sizeof(DatosOperacion));
        d->tipo_op        = OP_MOVER_DIVISA;
        d->cuenta_id      = g_cuenta_id;
        d->divisa_origen  = pedir_divisa("Divisa origen");
        d->divisa_destino = pedir_divisa("Divisa destino");
        printf("Cantidad a convertir: "); fflush(stdout);
        scanf("%f", &d->cantidad);
        lanzar_operacion(thread_mover_divisa, d);
        break;
    case 6:
        printf("Saliendo...\n");
        mq_close(g_mq_monitor);
        mq_close(g_mq_log);
        exit(0);
    default:
        printf("Opcion no valida.\n");
    }
}

/* Mostrar menú */
static void mostrar_menu(void) {
    printf("\n+==========================+\n");
    printf("|  Cuenta %-17d|\n", g_cuenta_id);
    printf("|==========================|\n");
    printf("| 1. Deposito              |\n");
    printf("| 2. Retiro                |\n");
    printf("| 3. Transferencia         |\n");
    printf("| 4. Consultar saldos      |\n");
    printf("| 5. Mover divisas         |\n");
    printf("| 6. Salir                 |\n");
    printf("+==========================+\n");
    printf("Opcion: ");
    fflush(stdout);
}

/*
E2C940E3C540D7C9C4C5D540D8E4C540C3D6D5E2C5D9E5C5E240C5D340C5E2D8E4C5D3C5E3D640C4C540C5E2E3C540D7D9D6C7D9C1D4C140C9D5E3C1C3E3D66B40D5D640C8C1C7C1E240C3C1E2D66B40D4D6C4C9C6C9C3C140E2C9C5D4D7D9C540C5D340E3C5E7E3D640C5D540C5C2C3C4C9C340E2E4D4C1D5C4D640F140C1D340C4C5C3C9D4D640C3D1D9C1C3E3C5D940C5D540C5C2C3C4C9C34BE2C940E3C540C4C9C3C5D540D8E4C540D7D6D940D8E4C540D3D640C8C1E240C3C1D4C2C9C1C4D640C4C940D8E4C540D5D640D3D640C8C1E240C3C1D4C2C9C1C4D64BD5D640C8C1C7C1E240D5C9D5C7D2D540C3D6C4C9C6C9C3C1C4D640C1D340C3D6C4C9C7D640D6C3E4D3E3D640C4C5D5E3D9D640C4C5D340C2D3D6D8E4C540C3D6C4C9C6C9C3C1C4D640C5D540C5C2C3C4C9C3
*/
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: usuario <cuenta_id> <pipe_rd>\n");
        return 1;
    }
    g_cuenta_id = atoi(argv[1]);
    g_pipe_rd   = atoi(argv[2]);

    leer_config_usuario();

    /* Abrir las colas POSIX ya creadas por banco.c */
    g_mq_monitor = mq_open(MQ_MONITOR, O_WRONLY);
    g_mq_log     = mq_open(MQ_LOG,     O_WRONLY);
    if (g_mq_monitor==(mqd_t)-1 || g_mq_log==(mqd_t)-1) {
        perror("mq_open en usuario");
        return 1;
    }

    sem_t *test = sem_open(SEM_CUENTAS, 0);
    if (test == SEM_FAILED) {
        fprintf(stderr, "[Cuenta %d] ERROR: semaforo %s no disponible: %s\n",
                g_cuenta_id, SEM_CUENTAS, strerror(errno));
        return 1;
    }
    sem_close(test);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; fds[0].events = POLLIN;
    fds[1].fd = g_pipe_rd;    fds[1].events = POLLIN;

    mostrar_menu();

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) { if (errno==EINTR) continue; break; }

        if (fds[1].revents & POLLIN) {
            char buf[256];
            ssize_t n = read(g_pipe_rd, buf, sizeof(buf)-1);
            if (n > 0) {
                buf[n] = '\0';
                printf("\n[ALERTA DEL BANCO]: %s\n", buf);
                fflush(stdout);
                if (strstr(buf, "BLOQUEO")) {
                    mq_close(g_mq_monitor);
                    mq_close(g_mq_log);
                    close(g_pipe_rd);
                    exit(0);
                }
            }
        }

        if (fds[0].revents & POLLIN) {
            int opcion;
            if (scanf("%d", &opcion) == 1) {
                procesar_opcion(opcion);
            } else {
                int c; while ((c=getchar())!='\n'&&c!=EOF);
            }
            mostrar_menu();
        }
    }

    mq_close(g_mq_monitor);
    mq_close(g_mq_log);
    close(g_pipe_rd);
    return 0;
}
