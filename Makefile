src = $(wildcard *.c)
obj = $(src:.c=.o)

# LDFLAGS = -lGL -lglut -lm

shell: $(obj)
	$(CC) -lreadline -g -Wall -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) shell
