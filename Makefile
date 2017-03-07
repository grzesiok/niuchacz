include Makefile.config
include Makefile.compile
#Dodatkowe polecenia
ECHO=echo

#reguły
%.o:%.asm
	@$(ECHO) [ASM] $@
	@$(ASM) $(ASMFLAGS) -o $(OBJDIR)/$@ $<

%.o:%.c
	@$(ECHO) [C] $@
	@$(CC) $(CFLAGS) -o $(OBJDIR)/$@ $<

FILEASMOBJ=$(FILEASM64OBJ)
FILEASMOBJ_=$(FILEASMOBJ:%=$(OBJDIR)/%)
FILEASM64OBJ_=$(FILEASM64OBJ:%=$(OBJDIR)/%)
FILEASM32OBJ_=$(FILEASM32OBJ:%=$(OBJDIR)/%)
FILECOBJ_=$(FILECOBJ:%=$(OBJDIR)/%)
FILEOBJ=$(FILEASMOBJ_) $(FILECOBJ_)

clean_dependencies:
	@$(MAKE) -C sqlite clean
	
build_dependencies:
	@cd sqlite && ./configure CPPFLAGS=-DSQLITE_DEBUG
	@$(MAKE) -C sqlite all

#komendy zewnętrzne
build: build_dependencies $(OUTFILE)
	@$(ECHO) Build Finished

clean: clean_dependencies
	@$(ECHO) Cleanup...
	@$(ECHO) [RM] $(FILECOBJ_)
	@rm -f $(FILECOBJ_)
	@$(ECHO) [RM] $(FILEASM64OBJ_) $(FILEASM32OBJ_)
	@rm -f $(FILEASM32OBJ_) $(FILEASM64OBJ_)
	@$(ECHO) [RM] $(OUTFILE)
	@rm -f $(OUTFILE)

all: clean build
	@$(ECHO) Rebuild Finished

run: all
	./$(OUTFILE)

prepare:
	@$(ECHO) ASM=$(ASM) ASMFLAGS=$(ASMFLAGS)
	@$(ECHO) CC=$(CC) CFLAGS=$(CFLAGS)
	@$(ECHO) LD=$(LD) LDFLAGS=$(LDFLAGS)

#kompilacja
$(OUTFILE): prepare $(FILECOBJ) $(FILEASMOBJ)
	@$(ECHO) [COMPILE] $(OUTFILE)
	@$(LD) $(LDFLAGS) -o $@ $(FILEOBJ)
	
testqueue.out: $(FILECOBJ_TESTQUEUE)
	@$(ECHO) [COMPILE] testqueue.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTQUEUE)
	
testhashperf.out: $(FILECOBJ_TESTHASHPERF)
	@$(ECHO) [COMPILE] testhashperf.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTHASHPERF)