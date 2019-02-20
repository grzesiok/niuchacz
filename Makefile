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
	@$(ECHO) [C] $@
	@$(CC) $(CFLAGS) -o $(OBJDIR)/$@ $<

FILEASMOBJ=$(FILEASM64OBJ)
FILEASMOBJ_=$(FILEASMOBJ:%=$(OBJDIR)/%)
FILEASM64OBJ_=$(FILEASM64OBJ:%=$(OBJDIR)/%)
FILEASM32OBJ_=$(FILEASM32OBJ:%=$(OBJDIR)/%)
FILECOBJ_=$(FILECOBJ:%=$(OBJDIR)/%)
FILEOBJ=$(FILEASMOBJ_) $(FILECOBJ_)

prepare:
	@$(ECHO) DEBUG=$(DEBUG)
	@$(ECHO) ASM=$(ASM) ASMFLAGS=$(ASMFLAGS)
	@$(ECHO) CC=$(CC) CFLAGS=$(CFLAGS)
	@$(ECHO) LD=$(LD) LDFLAGS=$(LDFLAGS)

clean_dependencies:
	@$(MAKE) -C libalgorithms clean
	@$(MAKE) -C sqlite clean
	
build_dependencies:
	@$(MAKE) -C libalgorithms build
	@cd sqlite && ./configure
	@$(MAKE) -C sqlite all

devbuild_dependencies:
	@$(MAKE) -C libalgorithms devbuild
	@cd sqlite && ./configure CPPFLAGS=-DSQLITE_DEBUG
	@$(MAKE) -C sqlite all

#komendy zewnętrzne
build: DEBUG=0
build: ASMFLAGS=-f elf_x86_64
build: LDFLAGS=-lc -m64 -Wall $(LIBS)
build: CFLAGS=-c -m64 -O3 -Wall $(INCLUDES) $(addprefix -D, $(DEFINES))
build: prepare build_dependencies $(OUTFILE)
	@$(ECHO) Build Finished

devbuild: DEBUG=1
devbuild: DEFINES+= DEBUG_MODE
devbuild: ASMFLAGS=-f elf_x86_64 -g
devbuild: LDFLAGS=-lc -m64 -Wall -g $(DEV_LIBS)
devbuild: CFLAGS=-c -m64 -Wall -g -I$(DEV_INCLUDES) $(addprefix -D, $(DEFINES))
devbuild: prepare devbuild_dependencies $(OUTFILE)
	@$(ECHO) DEV Build Finished

clean: clean_dependencies
	@$(ECHO) Cleanup...
	@$(ECHO) [RM] $(FILECOBJ_)
	@rm -f $(FILECOBJ_)
	@$(ECHO) [RM] $(FILEASM64OBJ_) $(FILEASM32OBJ_)
	@rm -f $(FILEASM32OBJ_) $(FILEASM64OBJ_)
	@$(ECHO) [RM] $(OUTFILE)
	@rm -f $(OUTFILE)

install:
	systemctl stop niuchacz.service
	cp niuchacz.out /usr/bin/niuchacz
	cp libalgorithms/libalgorithms.so /lib64/libalgorithms.so
	cp sys/etc/systemd/system/niuchacz.service /etc/systemd/system/niuchacz.service
	cp sys/etc/niuchacz/* /etc/niuchacz
	systemctl daemon-reload
	systemctl start niuchacz.service

#kompilacja
$(OUTFILE): $(FILECOBJ) $(FILEASMOBJ)
	@$(ECHO) [COMPILE] $(OUTFILE)
	@$(LD) $(LDFLAGS) -o $@ $(FILEOBJ)
	
testhashperf.out: prepare $(FILECOBJ_TESTHASHPERF)
	@$(ECHO) [COMPILE] testhashperf.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTHASHPERF)
	
testpsmgr.out: prepare $(FILECOBJ_TESTPSMGR)
	@$(ECHO) [COMPILE] testpsmgr.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTPSMGR)
	
testcmdmgr.out: prepare $(FILECOBJ_TESTCMDMGR)
	@$(ECHO) [COMPILE] testcmdmgr.out
	@$(LD) $(LDFLAGS) -o $@ $(FILECOBJ_TESTCMDMGR)
