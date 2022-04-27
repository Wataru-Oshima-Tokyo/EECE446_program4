CC = gcc

all: p4_registry ex_registry

clean:
	rm -r p4_registry ex_registry

p4_registry: p4_registry.c
	$(CC) $@.c -o $@ -Wall
ex_registry: ex_registry.c
	$(CC) $@.c -o $@ -Wall