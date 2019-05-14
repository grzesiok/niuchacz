.PHONY: prepare clean_dependencies build_dependencies devbuild_dependencies build devbuild clean install

include Makefile.compile
include Makefile.config

#Dodatkowe polecenia
ECHO=echo

#reguły
%.o:%.asm
	@$(ECHO) [ASM] $@
	@$(ASM) $(ASMFLAGS) -o $(OBJDIR)/$@ $<

%.o:%.c
	@$(ECHO) [C] $@ CFLAGS=$(CFLAGS)
	@$(CC) $(CFLAGS) -o $(OBJDIR)/$@ $<

FILEASMOBJ=$(FILEASM64OBJ)
FILEASMOBJ_=$(FILEASMOBJ:%=$(OBJDIR)/%)
FILEASM64OBJ_=$(FILEASM64OBJ:%=$(OBJDIR)/%)
FILEASM32OBJ_=$(FILEASM32OBJ:%=$(OBJDIR)/%)
FILECOBJ_=$(FILECOBJ:%=$(OBJDIR)/%)
FILEOBJ=$(FILEASMOBJ_) $(FILECOBJ_)

#komendy zewnętrzne
prepare:
	@$(ECHO) DEBUG=$(DEBUG)
	@$(ECHO) ASM=$(ASM) ASMFLAGS=$(ASMFLAGS)
	@$(ECHO) CC=$(CC) CFLAGS=$(CFLAGS)
	@$(ECHO) LD=$(LD) LDFLAGS=$(LDFLAGS)

build: DEBUG=0
build: ASMFLAGS=-f elf_x86_64
build: LDFLAGS=-lc -m64 -Wall -lm -shared $(LIBS)
build: CFLAGS=-c -m64 -O3 -Wall -fPIC -msse4.2 $(INCLUDES) $(addprefix -D, $(DEFINES))
build: prepare $(OUTFILE)
	@$(ECHO) Build Finished

devbuild: DEBUG=1
devbuild: DEFINES+= DEBUG_MODE
devbuild: ASMFLAGS=-f elf_x86_64 -g
devbuild: LDFLAGS=-lc -m64 -Wall -g3 -lm -shared $(DEV_LIBS)
devbuild: CFLAGS=-c -m64 -Wall -g3 -fPIC -msse4.2 $(DEV_INCLUDES) $(addprefix -D, $(DEFINES))
devbuild: prepare $(OUTFILE)
	@$(ECHO) DEV Build Finished

clean:
	@$(ECHO) Cleanup...
	@$(ECHO) [RM] $(FILECOBJ_) $(FILECOBJ_TESTHLL) $(FILECOBJ_TESTQUEUE) $(FILECOBJ_TESTBST)
	@rm -f $(FILECOBJ_) $(FILECOBJ_TESTHLL) $(FILECOBJ_TESTQUEUE) $(FILECOBJ_TESTBST)
	@$(ECHO) [RM] $(FILEASM64OBJ_) $(FILEASM32OBJ_)
	@rm -f $(FILEASM32OBJ_) $(FILEASM64OBJ_)
	@$(ECHO) [RM] $(OUTFILE)
	@rm -f $(OUTFILE)

#kompilacja
$(OUTFILE): $(FILECOBJ) $(FILEASMOBJ)
	@$(ECHO) [COMPILE] $(OUTFILE)
	@$(LD) $(LDFLAGS) -o $@ $(FILEOBJ)
	
testhll.out: $(FILECOBJ_TESTHLL)
	@$(ECHO) [COMPILE] testhll.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTHLL)
	
testdoublylinkedlist.out: $(FILECOBJ_TESTDOUBLYLINKEDLIST)
	@$(ECHO) [COMPILE] testdoublylinkedlist.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTDOUBLYLINKEDLIST)
	
testqueue: DEBUG=1
testqueue: DEFINES+= DEBUG_MODE
testqueue: ASMFLAGS=-f elf_x86_64 -g
testqueue: LDFLAGS=-lc -m64 -Wall -g3 -lm $(DEV_LIBS)
testqueue: CFLAGS=-c -m64 -Wall -g3 -msse4.2 $(DEV_INCLUDES) $(addprefix -D, $(DEFINES))
testqueue: prepare testqueue.out
	@$(ECHO) TESTQUUE build Finished

testqueue.out: $(FILECOBJ_TESTQUEUE)
	@$(ECHO) [COMPILE] testqueue.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTQUEUE)

testbst.out: $(FILECOBJ_TESTBST)
	@$(ECHO) [COMPILE] testbst.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTBST)
