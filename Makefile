CC = gcc

all: registry

clean:
	rm -r registry 

registry: registry.c
	$(CC) $@.c -o $@ -Wall
# ex_registry: ex_registry.c
# 	$(CC) $@.c -o $@ -Wall