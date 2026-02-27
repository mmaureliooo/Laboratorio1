#!/bin/bash
source /home/marco/code/mmaureliooo/ssoo/check.sh
echo "introduzca el nombre del archivo" ;read x
if check $x
then 
echo "$x existe !"
else
echo "$x no existe"
fi
