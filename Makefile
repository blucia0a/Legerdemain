DWARFDIR=../libdwarf/libdwarf

LDM_BLDDIR=./build
LDM_LIBDIR=./libs
LDM_TESTDIR=./tests

all: supportlibs plugins lib userplugins
supportlibs: applier stack libdwarfclient a2l
plugins: thread_constructor
userplugins: null_thd_constructor
lib: ldm
progs: dwarfinfo

#TODO: Make test cases
test: 


###################################################
#Rules to build the support libraries used by LDM
###################################################
stack: stack.c stack.h
	gcc -fPIC -c stack.c -o $(LDM_LIBDIR)/stack.o -g -O3
	ar rcs $(LDM_LIBDIR)/libstack.a $(LDM_LIBDIR)/stack.o

applier: Applier.c Applier.h
	gcc -fPIC -c Applier.c -o $(LDM_LIBDIR)/applier.o -g -O3
	ar rcs $(LDM_LIBDIR)/libapplier.a $(LDM_LIBDIR)/applier.o

a2l: addr2line.c addr2line.h
	gcc -fPIC -c -g addr2line.c -o $(LDM_LIBDIR)/addr2line.o
	ar rcs $(LDM_LIBDIR)/libaddr2line.a $(LDM_LIBDIR)/addr2line.o

libdwarfclient: dwarfclient.c dwarfclient.h $(LDM_LIBDIR)/libstack.a
	gcc -fPIC -c -g -DDWARF_CLIENT_LIB -I$(DWARFDIR) ./dwarfclient.c  -o $(LDM_LIBDIR)/dwarfclient.o
	ar rcs $(LDM_LIBDIR)/libdwarfclient.a $(LDM_LIBDIR)/dwarfclient.o


###################################################
#Rule to build the main LDM shared library
###################################################
ldm: legerdemain.c legerdemain.h sampled_thread_monitor.c sampled_thread_monitor.h
	gcc sampled_thread_monitor.c legerdemain.c -o $(LDM_LIBDIR)/libldm.so -fPIC -shared -g -O3 -L$(DWARFDIR) -ldwarf -lbfd -lelf -ldl -lreadline $(LDM_LIBDIR)/libaddr2line.a $(LDM_LIBDIR)/libapplier.a $(LDM_LIBDIR)/libdwarfclient.a $(LDM_LIBDIR)/libstack.a



###################################################
#Rule to build internally used LDM plugins 
###################################################
thread_constructor: $(LDM_LIBDIR)/libapplier.a
	gcc thread_constructor.c -o $(LDM_LIBDIR)/thread_constructor.so  -L$(LDM_LIBDIR) -lapplier -fPIC -shared -g -O3 -ldl -lldm

###################################################
#Rule to build user plugins (e.g., thread ctr lib) 
###################################################
null_thd_constructor: 
	gcc null_thd_constructor.c -o $(LDM_LIBDIR)/null_thd_constructor.so -fPIC -shared -g -O3

###################################################
#Rule to build the standalone dwarf dumping utility
###################################################
dwarfinfo: dwarfclient.c $(LDM_LIBDIR)/libstack.a
	gcc -g -lelf -L$(DWARFDIR) -ldwarf -I$(DWARFDIR) ./dwarfclient.c  $(LDM_LIBDIR)/libstack.a  -o $(LDM_BLDDIR)/dwarfinfo



clean:
	rm $(LDM_LIBDIR)/*.so $(LDM_LIBDIR)/*.o
