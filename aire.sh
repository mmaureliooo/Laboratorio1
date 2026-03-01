#!/bin/bash

# Validar entero
validar_entero() {
    if [[ "$1" =~ ^[0-9]+$ ]]
    then
        return 0
    else
        return 1
    fi
}

# Validar rango
validar_rango() {
    if (( $1 >= 0 && $1 <= 500 ))
    then
        return 0
    else
        return 1
    fi
}

# Clasificación
clasificar() {
    if [ "$1" -le 50 ];
    then
        echo "Bueno (Verde)"
    elif [ "$1" -le 100 ];
    then
        echo "Moderado (Amarillo)"
    elif [ "$1" -le 150 ];
    then
        echo "Dañino para grupos sensibles (Naranja)"
    elif [ "$1" -le 200 ];
    then
        echo "Dañino para la salud (Rojo)"
    else
        echo "Peligroso (Púrpura)"
    fi
}

echo "Introduce un valor IQA (0-500):"
read valor

if validar_entero "$valor" && validar_rango "$valor"
then
    clasificar "$valor"
else
    echo "Valor inválido."
fi
