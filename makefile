CC=occ
_OBJ= homer.a
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))
ODIR=o
DEPS=

all: $(ODIR)/._Homer.r Homer

Homer: $(OBJ)
	$(CC) -o $@ $(OBJ)
	iix chtyp -t 0xBC -a 0x0001 $@

$(ODIR)/%.a: %.c $(DEPS)
	@mkdir -p o
	$(CC) --lint=-1 -F -O -1 -c -o $@ $< 

$(ODIR)/._Homer.r:  homer.rez homer.equ
	@mkdir -p o
	occ -o $(ODIR)/Homer.r Homer.rez
	cp $(ODIR)/._Homer.r ._Homer

clean:
	@rm -f $(ODIR)/*.a $(ODIR)/*.root Homer $(ODIR)/Homer.r $(ODIR)/._Homer.r
	@rm -f Homer
	@rm -f ._Homer
