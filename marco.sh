#!/bin/bash
function marco(){
MARCO=$(pwd)
export MARCO
echo "variable exportada $MARCO"


}
function polo(){
if [ -z "$MARCO" ]
then
	echo "MARCO está vacía no hay cambio de directorio"
else
	echo "MARCO no está vacía, cambiando de directorio a MARCO..."
	cd "$MARCO"
	fi
}
