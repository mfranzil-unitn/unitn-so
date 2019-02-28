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

- `var=VALORE` - assegnazione (sono sempre stringhe)
- `${var}` - stampa (con eventuale esecuzione)
- `$@` - equivalente ad `argv`
- `$#` - equivalente ad `argc`
- `$1`, `$2`, `$3`, `$n` - i primi 9 parametri passati
- `shift` - cestina il primo argomento nella lista
- `" ... "` - ??
- `$(( ))` - contengono espressioni aritmetiche: se all'interno uso una `$var`, viene sostituita come fosse una macro, se uso il singolo contenuto di `var` allora viene inserito il valore come avesse le parentesi.
- `bc` - comando che supporta il piping in entrata, per eseguire operazioni in float
- `# Commenti` - kinda self-explanatory
- `$?` - valore di ritorno globale, usato dagli script (tipo return 0 in C): ha significato booleano (**0** niente errori, arriva fino a **256**)

Booleani:

- `test ...` - si aspetta un espressione booleana, e internamente modifica il registro booleano visto prima:
- `-eq, -ne, -lt, -gt` - operandi booleani utilizzati
- `[ ... ]` - sintassi di testing alternativa (gli spazi sono importanti!); attenzione che le parentesi quadrate sono considerate come ultimo comando eseguito
- `[[ ... ]]` - raggruppamento di espressioni booleani per utilizzare operatori comuni (>, < ...)- 
- `-f (file) -d (directory)` - verificano l'esistenza di un dato file/cartella. 

Cicli:

- `if [ ... ]; then ... ; else ...; fi` - costrutto if standard
- `case $var in; a|b) ... ;; c) ... ;; esac` - costrutto switch standard
- `for (( init ; case; step )); do; ...; done` - costrutto for standard
- `until [[ ... ]] ; ...; done` - while negato
- `while [[ ... ]] ; ...; done` - while standard
