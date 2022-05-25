CFLAGS = -Wall -std=c99 -iquote/home/given_files/
LDFLAGS = -pthread
WRAP = $(shell if [ -e wrap/ ]; then echo "wrap/os.c"; else echo "~/given_files/wrap/os.c"; fi )

.PHONY: dummy clean
dummy:
	@echo "Du måste specifiera vilken fil du vill kompilera, utan ändelsen .c"
	@echo "Just nu kan du kompilera:"
	@echo ""
	@for i in *.c; do echo "make "$$(basename $$i .c); done

clean:
	@echo "make clean är inte implementerat i denna makefil."
	@echo "Ta helt enkelt bort den körbara filen i stället."

foodN:
	@echo "Byt ut N mot den deluppgift du vill lösa!"

%:: %.c
	gcc $(CFLAGS) $(LDFLAGS) $< $(WRAP) -o $@

