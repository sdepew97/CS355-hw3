src = $(wildcard *.c)
obj = $(src:.c=.o)

# LDFLAGS = -lGL -lglut -lpng -lz -lm

shell: $(obj)
	$(CC) -lreadline -ggdb -g -Wall -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) shell
