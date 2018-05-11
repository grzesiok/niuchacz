.PHONY: all prepare clean_dependencies build_dependencies build clean run install

include Makefile.config
include Makefile.compile
#Dodatkowe polecenia
ECHO=echo

#reguły
%.o:%.asm
	@$(ECHO) [ASM] $@
	@$(ASM) $(ASMFLAGS) -o $(OBJDIR)/$@ $<

%.o:%.c
	@$(ECHO) [C] $(CC) $(CFLAGS) -o $(OBJDIR)/$@
	@$(CC) $(CFLAGS) -o $(OBJDIR)/$@ $<

FILEASMOBJ=$(FILEASM64OBJ)
FILEASMOBJ_=$(FILEASMOBJ:%=$(OBJDIR)/%)
FILEASM64OBJ_=$(FILEASM64OBJ:%=$(OBJDIR)/%)
FILEASM32OBJ_=$(FILEASM32OBJ:%=$(OBJDIR)/%)
FILECOBJ_=$(FILECOBJ:%=$(OBJDIR)/%)
FILEOBJ=$(FILEASMOBJ_) $(FILECOBJ_)
DEFINES=_GNU_SOURCE

prepare:
	@$(ECHO) ASM=$(ASM) ASMFLAGS=$(ASMFLAGS)
	@$(ECHO) CC=$(CC) CFLAGS=$(CFLAGS)
	@$(ECHO) LD=$(LD) LDFLAGS=$(LDFLAGS)

clean_dependencies:
	#@$(MAKE) -C libalgorithms clean
	#@$(MAKE) -C sqlite clean
	
build_dependencies: prepare
	#@$(MAKE) -C libalgorithms all
	#@cd sqlite && ./configure CPPFLAGS=-DSQLITE_DEBUG
	#@$(MAKE) -C sqlite all

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
	
install:
	systemctl stop niuchacz.service
	cp niuchacz.out /usr/bin/niuchacz
	cp libalgorithms/libalgorithms.so /lib64/libalgorithms.so
	cp sys/etc/systemd/system/niuchacz.service /etc/systemd/system/niuchacz.service
	cp sys/etc/niuchacz/* /etc/niuchacz
	systemctl daemon-reload
	systemctl start niuchacz.service

#kompilacja
$(OUTFILE): prepare $(FILECOBJ) $(FILEASMOBJ)
	@$(ECHO) [COMPILE] $(OUTFILE)
	@$(LD) $(LDFLAGS) -o $@ $(FILEOBJ)
	
testhashperf.out: enable_debug build_dependencies $(FILECOBJ_TESTHASHPERF)
	@$(ECHO) [COMPILE] testhashperf.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTHASHPERF)
	
testpsmgr.out: enable_debug build_dependencies $(FILECOBJ_TESTPSMGR)
	@$(ECHO) [COMPILE] testpsmgr.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTPSMGR)
	
testcmdmgr.out: build_dependencies $(FILECOBJ_TESTCMDMGR)
	@$(ECHO) [COMPILE] testcmdmgr.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTCMDMGR)