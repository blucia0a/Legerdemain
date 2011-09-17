DWARFDIR=../libdwarf/libdwarf

LDM_BLDDIR=./build
LDM_LIBDIR=./libs
LDM_TESTDIR=./tests

all: libs
libs: applier stack libdwarfclient a2l ldm scaley thread_constructor
progs: dwarfinfo
test: a2ltest scaley_test 

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

ldm: legerdemain.c legerdemain.h
	gcc -fPIC legerdemain.c -o $(LDM_LIBDIR)/libldm.so -fPIC -shared -g -O3 -L$(DWARFDIR) -ldwarf -lbfd -lelf -ldl -lreadline $(LDM_LIBDIR)/libaddr2line.a $(LDM_LIBDIR)/libapplier.a $(LDM_LIBDIR)/libdwarfclient.a $(LDM_LIBDIR)/libstack.a

thread_constructor:
	gcc thread_constructor.c -o $(LDM_LIBDIR)/thread_constructor.so  -fPIC -shared -g -O3 -ldl

scaley: $(LDM_LIBDIR)/libapplier.a
	gcc scaley_LDM.c -o $(LDM_LIBDIR)/scaley_LDM.so -L$(LDM_LIBDIR) -lapplier -fPIC -shared -g -O3 -ldl

scaley_test:
	gcc scaleytest.c -o $(LDM_TESTDIR)/scaleytest -g

a2ltest: testLibA2L.c $(LDM_LIBDIR)/libaddr2line.a
	gcc -g ./testLibA2L.c -o $(LDM_TESTDIR)/testLibA2L -L$(LDM_LIBDIR) -laddr2line -liberty 

dwarfinfo: dwarfclient.c $(LDM_LIBDIR)/libstack.a
	gcc -g -lelf -L$(DWARFDIR) -ldwarf -I$(DWARFDIR) ./dwarfclient.c  $(LDM_LIBDIR)/libstack.a  -o $(LDM_BLDDIR)/dwarfinfo


clean:
	rm $(LDM_LIBDIR)/*.so $(LDM_LIBDIR)/*.o
