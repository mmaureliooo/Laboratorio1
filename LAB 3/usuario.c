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

    //lo mismo que en el resto de lecturas
    if (!f) { perror("fopen config.txt"); return; }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char key[128], val[128];
        if (sscanf(line, "%127[^=]=%127s", key, val) != 2) continue;
        // Solo lo necesario
        if      (strcmp(key, "LIM_RET_EUR")     == 0) g_cfg.lim_ret_eur          = atof(val);
        else if (strcmp(key, "LIM_RET_USD")     == 0) g_cfg.lim_ret_usd          = atof(val);
        else if (strcmp(key, "LIM_RET_GBP")     == 0) g_cfg.lim_ret_gbp          = atof(val);
        else if (strcmp(key, "LIM_TRF_EUR")     == 0) g_cfg.lim_trf_eur          = atof(val);
        else if (strcmp(key, "LIM_TRF_USD")     == 0) g_cfg.lim_trf_usd          = atof(val);
        else if (strcmp(key, "LIM_TRF_GBP")     == 0) g_cfg.lim_trf_gbp          = atof(val);
        else if (strcmp(key, "CAMBIO_USD")       == 0) g_cfg.cambio_usd           = atof(val);
        else if (strcmp(key, "CAMBIO_GBP")       == 0) g_cfg.cambio_gbp           = atof(val);
        else if (strcmp(key, "ARCHIVO_CUENTAS")  == 0) strncpy(g_cfg.archivo_cuentas, val, MAX_PATH-1);
    }
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
    // Rellenar la estructura con los datos de la operación
    dm.cuenta_origen  = cuenta_origen;
    dm.cuenta_destino = cuenta_destino;  // 0 si no es transferencia
    dm.tipo_op        = tipo_op;
    dm.cantidad       = cantidad;
    dm.divisa         = divisa;
    timestamp_ahora(dm.timestamp, sizeof(dm.timestamp));//funcion de banco comun
    mq_send(g_mq_monitor, (const char *)&dm, sizeof(dm), 0);// enviar al monitor
}

static void enviar_log(int tipo_op, float cantidad, int divisa, int estado) {
    DatosLog dl;
    memset(&dl, 0, sizeof(dl));
    //
    // Completar
    //
    // Rellenar y enviar el mensaje al padre (banco.c) para que lo registre
    dl.cuenta_id = g_cuenta_id;
    dl.pid_hijo  = getpid();
    dl.tipo_op   = tipo_op;
    dl.cantidad  = cantidad;
    dl.divisa    = divisa;
    dl.estado    = estado;   // 0 = exito, 1 = fallo
    timestamp_ahora(dl.timestamp, sizeof(dl.timestamp));
    mq_send(g_mq_log, (const char *)&dl, sizeof(dl), 0);//log al banco
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
    DatosOperacion *d = (DatosOperacion *)arg;
    if (!sem) { free(d); return NULL; }//por si falla

    // Sección crítica: leer cuenta, sumar importe y escribir de vuelta
    // Siempre leemos desde disco (no desde una variable local) para no trabajar con saldos obsoletos
    sem_wait(sem);//restamos
    Cuenta c; //objeto cuenta para copiarla con leer cuenta
    int estado = 1;  // 1 = fallo por defecto
    if (leer_cuenta(d->cuenta_id, &c) == 0) { //si va bien
        switch (d->divisa_origen) {//seleccionamos divisa elegida y sumamos cantidad
            case DIV_EUR: c.saldo_eur += d->cantidad; break;
            case DIV_USD: c.saldo_usd += d->cantidad; break;
            default:      c.saldo_gbp += d->cantidad; break; //hemos puesto el default en libras
        }
        escribir_cuenta(&c);//poner la nueva cuenta actualizada
        printf("Deposito OK: +%.2f %s\n", d->cantidad, nombre_divisa(d->divisa_origen));
        estado = 0;// ahora si que se ha completado
    }
    sem_post(sem);//liberar y cerrar
    sem_close(sem);

    // Notificar al padre y al monitor
    enviar_log(OP_DEPOSITO, d->cantidad, d->divisa_origen, estado);
    enviar_monitor(d->cuenta_id, 0, OP_DEPOSITO, d->cantidad, d->divisa_origen);
    free(d);//limpiamos
    return NULL;
}

/* Retiro */
static void *thread_retiro(void *arg) {
    //
    // Completar
    //
    DatosOperacion *d = (DatosOperacion *)arg;
    sem_t *sem = abrir_sem_cuentas();
    if (!sem) { free(d); return NULL; }

    // Obtener el límite de retiro según la divisa elegida
    float limite;
    switch (d->divisa_origen) {
        case DIV_EUR: limite = g_cfg.lim_ret_eur; break;
        case DIV_USD: limite = g_cfg.lim_ret_usd; break;
        default:      limite = g_cfg.lim_ret_gbp; break;
    }

    int estado = 1;
    sem_wait(sem);//coge semaforo por op. critica
    Cuenta c;//creamos objeto cuenta para modificarla
    if (leer_cuenta(d->cuenta_id, &c) == 0) { //leemos a la cuenta vacia
        // Puntero al saldo correcto para no repetir el switch
        float *saldo;
        switch (d->divisa_origen) {
            case DIV_EUR: saldo = &c.saldo_eur; break;
            case DIV_USD: saldo = &c.saldo_usd; break;
            default:      saldo = &c.saldo_gbp; break;
        }
        // fondos suficientes
        if (d->cantidad > *saldo) {
            printf("Saldo insuficiente (%.2f %s disponibles).\n",
                   *saldo, nombre_divisa(d->divisa_origen));
        // y si estan dentro del limite del config
        } else if (d->cantidad > limite) {
            printf("Supera el limite de retiro (%.2f %s).\n",
                   limite, nombre_divisa(d->divisa_origen));
        } else { //si todo va bien
            *saldo -= d->cantidad;
            escribir_cuenta(&c);//poner la cuenta actualizada
            printf("Retiro OK: -%.2f %s\n", d->cantidad, nombre_divisa(d->divisa_origen));
            estado = 0;
        }
    }
    sem_post(sem);//sumar y cerrar local
    sem_close(sem);

    enviar_log(OP_RETIRO, d->cantidad, d->divisa_origen, estado);//envia el log
    enviar_monitor(d->cuenta_id, 0, OP_RETIRO, d->cantidad, d->divisa_origen);//envia al monitor a analizar
    free(d);//limpiamos por si acaso
    return NULL;
}

/*
E2C940E3C540D7C9C4C5D540D8E4C540C3D6D5E2C5D9E5C5E240C5D340C5E2D8E4C5D3C5E3D640C4C540C5E2E3C540D7D9D6C7D9C1D4C140C9D5E3C1C3E3D66B40D5D640C8C1C7C1E240C3C1E2D66B40D4D6C4C9C6C9C3C140E2C9C5D4D7D9C540C5D340E3C5E7E3D640C5D540C5C2C3C4C9C340E2E4D4C1D5C4D640F140C1D340C4C5C3C9D4D640C3D1D9C1C3E3C5D940C5D540C5C2C3C4C9C34BE2C940E3C540C4C9C3C5D540D8E4C540D7D6D940D8E4C540D3D640C8C1E240C3C1D4C2C9C1C4D640C4C940D8E4C540D5D640D3D640C8C1E240C3C1D4C2C9C1C4D64BD5D640C8C1C7C1E240D5C9D5C7D2D540C3D6C4C9C6C9C3C1C4D640C1D340C3D6C4C9C7D640D6C3E4D3E3D640C4C5D5E3D9D640C4C5D340C2D3D6D8E4C540C3D6C4C9C6C9C3C1C4D640C5D540C5C2C3C4C9C3
*/
/* Transferencia */
static void *thread_transferencia(void *arg) {
    //
    // Completar
    //
    DatosOperacion *d = (DatosOperacion *)arg;
    sem_t *sem = abrir_sem_cuentas();
    if (!sem) { free(d); return NULL; }

    // Limite de transferencia segun divisa
    float limite;
    switch (d->divisa_origen) {
        case DIV_EUR: limite = g_cfg.lim_trf_eur; break;
        case DIV_USD: limite = g_cfg.lim_trf_usd; break;
        default:      limite = g_cfg.lim_trf_gbp; break;
    }

    int estado = 1;
    // leer y modificar en misma seccion critica
    // nadie puede ver el estado intermedio donde el dinero ya salio del origen pero aun no esta en destino
    sem_wait(sem);//restar
    Cuenta origen, destino;//crear objetos
    if (leer_cuenta(d->cuenta_id,      &origen)  != 0 || //copiar cuentas
        leer_cuenta(d->cuenta_destino, &destino) != 0) {
        printf("Cuenta destino %d no encontrada.\n", d->cuenta_destino);//falla
    } else {
        // Punteros al saldo afectado en cada cuenta (misma divisa)
        float *s_orig, *s_dest;
        switch (d->divisa_origen) {
            case DIV_EUR: s_orig = &origen.saldo_eur; s_dest = &destino.saldo_eur; break;
            case DIV_USD: s_orig = &origen.saldo_usd; s_dest = &destino.saldo_usd; break;
            default:      s_orig = &origen.saldo_gbp; s_dest = &destino.saldo_gbp; break;
        }
        if (d->cantidad > *s_orig) { //comprobar cantidad
            printf("Saldo insuficiente para la transferencia.\n");
        } else if (d->cantidad > limite) {
            printf("Supera el limite de transferencia (%.2f %s).\n",
                   limite, nombre_divisa(d->divisa_origen));
        } else {
            *s_orig -= d->cantidad;
            *s_dest += d->cantidad;
            /* Escribir ambas cuentas dentro de la sección crítica */
            escribir_cuenta(&origen);
            escribir_cuenta(&destino);
            printf("Transferencia OK: %.2f %s -> cuenta %d\n",
                   d->cantidad, nombre_divisa(d->divisa_origen), d->cuenta_destino);
            estado = 0;//transferencia completada, exito
        }
    }
    sem_post(sem);//sumamos y cerramos
    sem_close(sem);
    //mensajes para log y analisis
    enviar_log(OP_TRANSFERENCIA, d->cantidad, d->divisa_origen, estado);
    enviar_monitor(d->cuenta_id, d->cuenta_destino,
                   OP_TRANSFERENCIA, d->cantidad, d->divisa_origen);
    free(d);
    return NULL;
}

/* Mover divisas */
static void *thread_mover_divisa(void *arg) {
    //
    // Completar
    //
    DatosOperacion *d = (DatosOperacion *)arg;
    sem_t *sem = abrir_sem_cuentas();
    if (!sem) { free(d); return NULL; }

    int estado = 1;
    sem_wait(sem);
    Cuenta c;
    if (leer_cuenta(d->cuenta_id, &c) == 0) {
        //Puntero al saldo origen, saldo inicial
        float *s_orig;
        switch (d->divisa_origen) {
            case DIV_EUR: s_orig = &c.saldo_eur; break;
            case DIV_USD: s_orig = &c.saldo_usd; break;
            default:      s_orig = &c.saldo_gbp; break;
        }
        if (d->cantidad > *s_orig) { //compruebas que lo que sacas es menor al saldo
            printf("Saldo insuficiente para la conversion.\n");
        } else {
            // Conversion a eur primero y luego a la que sea
            // Division para ir a EUR, multiplicacion para salir de EUR
            float en_eur;
            switch (d->divisa_origen) { //a euro lo que quieres sacar
                case DIV_EUR: en_eur = d->cantidad;                        break;
                case DIV_USD: en_eur = d->cantidad / g_cfg.cambio_usd;    break;
                default:      en_eur = d->cantidad / g_cfg.cambio_gbp;    break;
            }
            float en_dest;
            switch (d->divisa_destino) {//multiplicar lo que quieres sacar la divisa a la que vas
                case DIV_EUR: en_dest = en_eur;                            break;
                case DIV_USD: en_dest = en_eur * g_cfg.cambio_usd;        break;
                default:      en_dest = en_eur * g_cfg.cambio_gbp;        break;
            }
            *s_orig -= d->cantidad; //restas cantidad al saldo original de la divisa original
            switch (d->divisa_destino) { // sumas a la divisa destino el saldo convertido
                case DIV_EUR: c.saldo_eur += en_dest; break;
                case DIV_USD: c.saldo_usd += en_dest; break;
                default:      c.saldo_gbp += en_dest; break;
            }
            escribir_cuenta(&c);
            printf("Conversion OK: %.2f %s -> %.2f %s\n",
                   d->cantidad, nombre_divisa(d->divisa_origen),
                   en_dest,     nombre_divisa(d->divisa_destino));
            estado = 0; //todo bien
        }
    }
    sem_post(sem);
    sem_close(sem);

    enviar_log(OP_MOVER_DIVISA, d->cantidad, d->divisa_origen, estado);
    free(d);
    return NULL;
}

/* Lanzar thread */
static void lanzar_operacion(void *(*fn)(void*), DatosOperacion *d) {
    pthread_t t;
    pthread_create(&t, NULL, fn, d);
    pthread_join(t, NULL);
}

/* Consultar saldos */
static void consultar_saldos(void) {
    //
    // Completar(puesto por nosotros)
    //
    sem_t *sem = abrir_sem_cuentas();
    if (!sem) return;
    // semaforo por si otro hilo escribe mientras leemos
    sem_wait(sem);//restamos
    Cuenta c;
    int ok = leer_cuenta(g_cuenta_id, &c);//copiamos cuenta
    sem_post(sem);//sumas y cierras
    sem_close(sem);

    if (ok != 0) { printf("Error al leer la cuenta.\n"); return; }//error

    // Total equivalente en EUR usando los tipos de cambio
    float total_eur = c.saldo_eur + c.saldo_usd / g_cfg.cambio_usd + c.saldo_gbp / g_cfg.cambio_gbp;

    printf("\n+--------------0--------------+\n");
    printf("|  Saldos cuenta %-12d|\n", g_cuenta_id);
    printf("|-----------------------------|\n");
    printf("| EUR: %22.2f |\n", c.saldo_eur);
    printf("| USD: %22.2f |\n", c.saldo_usd);
    printf("| GBP: %22.2f |\n", c.saldo_gbp);
    printf("|-----------------------------|\n");
    printf("| Total (EUR): %14.2f |\n", total_eur);
    printf("+-----------------------------+\n");
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
