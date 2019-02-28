# Comandi utili bash

Comandi:

- `free` - memoria libera
- `df` - partizioni
- gli altri comandi li conosci

Piping:

- `;` - esecuzione in sequenza
- `&&`, `||` - esecuzione in sequenza con corto circuito 
- `|` - piping classico
- `>` - redirect su file classico
- `>1` - redirect di `stdout`
- `>2` - redirect di `stderr`

Scripting (intestazione):

- `#! /bin/bash` - intestazione
- `#Script "Hello World"` - intestazione nominativa

Scripting:

- `var=VALORE` - assegnazione
- `${var}` - stampa (con eventuale esecuzione)
- `$@` - equivalente ad `argv`
- `$#` - equivalente ad `argc`
- `$1`, `$2`, `$3`, `$n` - i primi 9 parametri passati
- `shift` - cestina il primo argomento nella lista
