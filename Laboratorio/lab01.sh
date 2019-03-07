#!/bin/bash

# Comandi utili: ls, free, ll
# Piping: standard |, output >, stdout 1>, stderr 2>


x=2+3
echo "x=$x"
z1=$(( x*2 ))
z2=$(( $x*2 ))
echo "z1=$z1"
echo "z2=$z2"

echo "$(echo '1.12 + 2.02' | bc)"

echo "res=$?"
x=3
test $x -eq 3
echo "res=$?"
[ $x -eq 0 ] 
echo "res=$?"

x=3

if [ $x -eq 3]; then
	echo "OK"
else
	echo "NO"
fi

read char
case $char in
	y) echo "Input: y" ;;
	n) echo "Input: n" ;;
	*) echo "Input: nè y nè n" ;;
esac

for (( i = 1 , j = 1 ; i <= 10 ; i += 1, j *= 2 )); do
	echo "i=$i, j=$j"
done

echo "Crea un file 'semaforo.txt'..."
# attende che un file sia creato
until [[ -e "semaforo.txt" ]] ; do sleep 3; done
echo "File creato, ora eliminalo..."
# attende che un file sia eliminato
while [[ -e "semaforo.txt" ]] ; do sleep 3; done
echo "File eliminato!"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "DIR = ${DIR}"