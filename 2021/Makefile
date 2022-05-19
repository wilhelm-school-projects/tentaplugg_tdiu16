CFLAGS = -Wall -std=c99 -g
LDFLAGS = -pthread
WRAP = wrap/os.c

.PHONY: dummy clean
dummy:
	@echo "Du måste specifiera vilken fil du vill kompilera, utan ändelsen .c"
	@echo "Just nu kan du kompilera:"
	@echo ""
	@for i in *.c; do echo "make "$$(basename $$i .c); done

clean:
	@echo "make clean är inte implementerat i denna makefil."
	@echo "Ta helt enkelt bort den körbara filen i stället."
	@echo "Exempelvis:"
	@for i in *.c; do echo "rm "$$(basename $$i .c); done

%:: %.c
	gcc $(CFLAGS) $(LDFLAGS) $< $(WRAP) -o $@

