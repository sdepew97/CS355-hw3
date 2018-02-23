all: shell

shell: shell.o llist.o llist_node.o
    gcc -g -o $@ $^ -Wall -lcheck

check_llist_node: check_llist_node.o llist_node.o
    gcc -g -o $@ $^ -Wall -lcheck

check_llist: check_llist.o llist.o llist_node.o
    gcc -g -o $@ $^ -Wall -lcheck

shell.o: shell.c shell.h llist.h llist_node.h
    gcc -c -Wall shell.c

llist.o: llist.h llist_node.h
    gcc -c -Wall llist.c

clean:
    rm -rf *.o *.gch *.dSYM shell\ *

.PHONY: all

# this next line prevents `make` from deleting the .o files
.SECONDARY:
