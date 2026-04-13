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
   /* Solo necesita los umbrales de detección de anomalías */
   FILE *f = fopen("config.txt", "r");
   if (!f) return;
   char line[256];
   while (fgets(line, sizeof(line), f)) {
       if (line[0] == '#' || line[0] == '\n') continue;
       char key[128], val[128];
       if (sscanf(line, "%127[^=]=%127s", key, val) != 2) continue;
       if      (strcmp(key, "UMBRAL_RETIROS")        == 0) g_umbral_retiros        = atoi(val);
       else if (strcmp(key, "UMBRAL_TRANSFERENCIAS") == 0) g_umbral_transferencias = atoi(val);
   }
   fclose(f);
}

/*  Enviar alerta al padre  */
static void enviar_alerta(int cuenta_id, const char *tipo_alerta) {
    //
    // Completar
    //
    DatosAlerta da;
    memset(&da, 0, sizeof(da));
    da.cuenta_id = cuenta_id;
    /* El padre usará strstr sobre da.mensaje para clasificar la alerta */
    snprintf(da.mensaje, sizeof(da.mensaje),
             "%s en cuenta %d", tipo_alerta, cuenta_id);
    mq_send(g_mq_alerta, (const char *)&da, sizeof(da), 0);
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
    /* Buscar o crear entrada de seguimiento para esta cuenta */
    int idx = -1;
    for (int i = 0; i < g_n_retiros; i++)
        if (g_retiros[i].cuenta_id == dm->cuenta_origen) { idx = i; break; }
    if (idx < 0 && g_n_retiros < MAX_CUENTAS_TRACK) {
        idx = g_n_retiros++;
        g_retiros[idx].cuenta_id            = dm->cuenta_origen;
        g_retiros[idx].retiros_consecutivos = 0;
        g_retiros[idx].ultimo_retiro        = 0.0f;
    }
    if (idx >= 0) {
        g_retiros[idx].retiros_consecutivos++;
        g_retiros[idx].ultimo_retiro = dm->cantidad;
        /* Disparar alerta si se supera el umbral y reiniciar contador */
        if (g_retiros[idx].retiros_consecutivos >= g_umbral_retiros) {
            enviar_alerta(dm->cuenta_origen, ALERTA_RETIROS);
            g_retiros[idx].retiros_consecutivos = 0;
        }
    }
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
        /* Esperar hasta 500 ms a que llegue un mensaje de MQ_MONITOR */
        int ret = poll(&pfd, 1, 500);
        if (ret < 0) {
            if (errno == EINTR) continue;  /* señal recibida, recomprobar g_salir */
            break;
        }
        if (ret == 0) continue;  /* timeout, reintentar */

        /* Hay datos disponibles: leer y analizar el mensaje */
        DatosMonitor dm;
        if (mq_receive(g_mq_monitor, (char *)&dm, sizeof(dm), NULL) > 0)
            analizar(&dm);
    }

    mq_close(g_mq_monitor);
    mq_close(g_mq_alerta);
    return 0;
}
