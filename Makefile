src = $(filter-out linked_list, $(wildcard *.c))
obj = $(src:.c=.o)

# LDFLAGS = -lGL -lglut -lpng -lz -lm

shell: $(obj)
	$(CC) -lreadline -Wall -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) shell
