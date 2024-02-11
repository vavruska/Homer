CC=occ
_OBJ= homer.a
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))
ODIR=o
DEPS=
CC_FLAGS :=--lint=-1 -O -1
UNAME_S := $(shell uname -s)

ifeq ($(OS),Windows_NT)
all: $(ODIR)/Homer.r Homer
else
    ifeq ($(UNAME_S),Linux)
all: $(ODIR)/._Homer.r Homer
    endif
    ifeq ($(UNAME_S),Darwin)
all: $(ODIR)/Homer.r Homer
    endif
endif
Homer: $(OBJ)
	$(CC) -o $@ $(OBJ)
	iix chtyp -t 0xBC -a 0x0001 $@

$(ODIR)/%.a: %.c $(DEPS)
	@mkdir -p o
	$(CC) $(CC_FLAGS) -c -o $@ $<

$(ODIR)/._Homer.r:  homer.rez homer.equ
	@mkdir -p o
	occ -o $(ODIR)/Homer.r Homer.rez
	cp $(ODIR)/._Homer.r ._Homer
	
$(ODIR)/Homer.r: homer.rez homer.equ
	@mkdir -p o
	occ -o $(ODIR)/Homer.r homer.rez
	occ -o Homer homer.rez

clean:
	@rm -f $(ODIR)/*.a $(ODIR)/*.root Homer $(ODIR)/Homer.r $(ODIR)/._Homer.r
	@rm -f Homer
	@rm -f ._Homer
