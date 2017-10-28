ecobee: main.c json.c
	gcc -o ecobee main.c json.c src/utils.c src/api_methods.c -I. -lcurl -lm