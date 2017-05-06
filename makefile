ecobee: main.c json.c
	gcc -o ecobee main.c json.c -I. -lcurl -lm