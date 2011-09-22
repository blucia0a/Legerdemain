LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain

all: NullThreadWatcher 

GLIB_CFLAGS=`pkg-config --cflags glib`
GLIB_LIBS=`pkg-config --libs glib`

NullThreadWatcher:
	gcc NullThreadWatcher.c -o NullThreadWatcher.so -I$(LDMDIR) -L$(LDMDIR)/libs -lldm -lpthread -fPIC -shared -g -O3

clean:
	rm *.so
