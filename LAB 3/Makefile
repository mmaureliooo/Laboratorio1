CC      = gcc
CFLAGS  = -Wall -Wextra -pthread
LDFLAGS = -pthread -lrt

# Binarios a generar
TARGETS = banco usuario monitor

.PHONY: all clean

all: $(TARGETS)

banco: banco.c banco_comun.h
	$(CC) $(CFLAGS) -o banco banco.c $(LDFLAGS)

usuario: usuario.c banco_comun.h
	$(CC) $(CFLAGS) -o usuario usuario.c $(LDFLAGS)

monitor: monitor.c banco_comun.h
	$(CC) $(CFLAGS) -o monitor monitor.c $(LDFLAGS)

# Eliminar binarios y ficheros generados en ejecución
clean:
	rm -f $(TARGETS) cuentas.dat transacciones.log
