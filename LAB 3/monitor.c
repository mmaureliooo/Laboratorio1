#include "banco_comun.h"

/* Parámetros de detección */
static int   g_umbral_retiros        = 3;
static int   g_umbral_transferencias = 5;
static volatile sig_atomic_t g_salir = 0;

static mqd_t g_mq_monitor = (mqd_t)-1;
static mqd_t g_mq_alerta  = (mqd_t)-1;

/* Señal de terminación  */
static void manejador_sigterm(int s) { (void)s; g_salir = 1; }

/* Leer config */
static void leer_config_monitor(void) {
   //
   // Completar
   //
}

/*  Enviar alerta al padre  */
static void enviar_alerta(int cuenta_id, const char *tipo_alerta) {
    //
    // Completar
    //
}


#define MAX_CUENTAS_TRACK 256
#define MAX_TRAN_TRACK    64

typedef struct {
    int   cuenta_id;
    int   retiros_consecutivos;
    float ultimo_retiro;
} TrackRetiros;

typedef struct {
    int cuenta_origen;
    int cuenta_destino;
    int contador;
} TrackTransferencias;

static TrackRetiros        g_retiros[MAX_CUENTAS_TRACK];
static int                 g_n_retiros = 0;
static TrackTransferencias g_transf[MAX_TRAN_TRACK];
static int                 g_n_transf = 0;

/*  Analizar mensaje  */
static void analizar(const DatosMonitor *dm) {

    /*  Retiros consecutivos */
    if (dm->tipo_op == OP_RETIRO) {
    //
    // Completar
    //
    } else {
        for (int i = 0; i < g_n_retiros; i++)
            if (g_retiros[i].cuenta_id == dm->cuenta_origen) {
                g_retiros[i].retiros_consecutivos = 0; break;
            }
    }

    /*  Transferencias repetitivas */
    if (dm->tipo_op == OP_TRANSFERENCIA) {
        int idx = -1;
        for (int i = 0; i < g_n_transf; i++)
            if (g_transf[i].cuenta_origen  == dm->cuenta_origen &&
                g_transf[i].cuenta_destino == dm->cuenta_destino) { idx = i; break; }
        if (idx < 0 && g_n_transf < MAX_TRAN_TRACK) {
            idx = g_n_transf++;
            g_transf[idx].cuenta_origen  = dm->cuenta_origen;
            g_transf[idx].cuenta_destino = dm->cuenta_destino;
            g_transf[idx].contador       = 0;
        }
        if (idx >= 0) {
            g_transf[idx].contador++;
            if (g_transf[idx].contador >= g_umbral_transferencias) {
                enviar_alerta(dm->cuenta_origen, ALERTA_TRANSFERENCIAS);
                g_transf[idx].contador = 0;
            }
        }
    }
}


int main(void) {
    signal(SIGTERM, manejador_sigterm);
    signal(SIGINT,  SIG_IGN);

    leer_config_monitor();

    /* Abrir las colas ya creadas por banco.c */
    g_mq_monitor = mq_open(MQ_MONITOR, O_RDONLY);
    g_mq_alerta  = mq_open(MQ_ALERTA,  O_WRONLY);
    if (g_mq_monitor==(mqd_t)-1 || g_mq_alerta==(mqd_t)-1) {
        perror("[MONITOR] mq_open");
        return 1;
    }

    /* poll() sobre el descriptor de MQ_MONITOR: bloqueo eficiente */
    struct pollfd pfd;
    pfd.fd     = (int)g_mq_monitor;
    pfd.events = POLLIN;

    while (!g_salir) {
        //
        // Completar
        //
    }

    mq_close(g_mq_monitor);
    mq_close(g_mq_alerta);
    return 0;
}
