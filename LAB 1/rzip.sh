#!/bin/bash
read nombre_archivo
read compresion

if [ -f "$nombre_archivo" ]; then
    if [[ "$compresion" == "tar" ]]; then
        tar -czfv "${nombre_archivo}.tar" "$nombre_archivo"
        
    elif [[ "$compresion" == "zip" ]]; then   
        zip -r "${nombre_archivo}.zip" "$nombre_archivo"
        
    else
        echo "Opcion de compresion invalida"
        exit 0
    fi

    rm "$nombre_archivo"
    exit 1
    
else
    echo "No se ha encontrado $nombre_archivo"
    exit 0
fi





