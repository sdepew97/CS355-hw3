src = $(wildcard *.c)
obj = $(src:.c=.o)

# LDFLAGS = -lGL -lglut -lpng -lz -lm -ggdb -g

shell: $(obj)
	$(CC) -lreadline -Wall -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) shell
