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

#komendy zewnętrzne
build: $(OUTFILE)
	@$(ECHO) Build Finished

clean:
	@$(ECHO) Cleanup...
	@$(ECHO) [RM] $(FILECOBJ_) $(FILECOBJ_TESTHLL) $(FILECOBJ_TESTQUEUE) $(FILECOBJ_TESTBST)
	@rm -f $(FILECOBJ_) $(FILECOBJ_TESTHLL) $(FILECOBJ_TESTQUEUE) $(FILECOBJ_TESTBST)
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
	
testhll.out: $(FILECOBJ_TESTHLL)
	@$(ECHO) [COMPILE] testhll.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTHLL)
	
testdoublylinkedlist.out: $(FILECOBJ_TESTDOUBLYLINKEDLIST)
	@$(ECHO) [COMPILE] testdoublylinkedlist.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTDOUBLYLINKEDLIST)
	
testqueue.out: $(FILECOBJ_TESTQUEUE)
	@$(ECHO) [COMPILE] testqueue.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTQUEUE)

testbst.out: $(FILECOBJ_TESTBST)
	@$(ECHO) [COMPILE] testbst.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTBST)
