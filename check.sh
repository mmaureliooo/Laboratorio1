#!bin/bash
function check(){
	if [ -e "$1" ]
		then
		return 0
	else
		return 1
	fi
}
