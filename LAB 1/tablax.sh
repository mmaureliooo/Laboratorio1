#!/bin/bash

num_tabla="$1"
lim_tabla="$2"
desc="$3"

imprimir_tabla(){
    if [ "$desc" == "desc" ]; then
        for ((i=lim_tabla; i>=0; i--))
        do
            mult=$((num_tabla * i))
            echo -e "${num_tabla} X ${i} = ${mult}\n"
        done
    else
        for ((i=0; i<=lim_tabla; i++))
        do
            mult=$((num_tabla * i))
            echo -e "${num_tabla} X ${i} = ${mult}\n"
        done
    fi
}

imprimir_tabla
