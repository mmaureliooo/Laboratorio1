#!/bin/bash

#Validar nombre (6-20 caracteres)
validar_nombre() {
    local nombre="$1"

    if (( ${#nombre} >= 6 && ${#nombre} <= 20 ))
    then
        return 0
    else
        return 1
    fi
}

# Validar tamaño (1-20 MB)
validar_tamano() {
    local tam="$1"

    if [[ "$tam" =~ ^[0-9]+$ ]] && (( tam >= 1 && tam <= 20 ))
    then
        return 0
    else
        return 1
    fi
}

# Validar tipo de contenido
validar_tipo() {
    local tipo="$1"

    if [ "$tipo" = "cero" ] || [ "$tipo" = "random" ]
    then
        return 0
    else
        return 1
    fi
}

#Comprobación de número de argumentos
if [ $# -ne 3 ]
then
    echo "Ejecutame así: $0 <nombre_fichero> <tam_MB> <cero|random>"
    exit 1
fi

nombre="$1"
tam="$2"
tipo="$3"

#Validaciones
if ! validar_nombre "$nombre"
then
    echo "Error: El nombre debe tener entre 6 y 20 caracteres."
    exit 1
fi

if ! validar_tamano "$tam"
then
    echo "Error: El tamaño debe estar entre 1 y 20 MB."
    exit 1
fi

if ! validar_tipo "$tipo"
then
    echo "Error: El tipo debe ser 'cero' o 'random'."
    exit 1
fi

# Crear fichero
if [ "$tipo" = "cero" ]
then
    dd if=/dev/zero of="$nombre" bs=1M count="$tam"
else
    dd if=/dev/urandom of="$nombre" bs=1M count="$tam"
fi

echo "Fichero '$nombre' creado correctamente con tamaño $tam MB y contenido '$tipo'."
