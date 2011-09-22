LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain

all: NullSampledWatcher 

GLIB_CFLAGS=`pkg-config --cflags glib`
GLIB_LIBS=`pkg-config --libs glib`

NullSampledWatcher:
	gcc NullSampledWatcher.c -o NullSampledWatcher.so -I$(LDMDIR) -L$(LDMDIR)/libs -lldm -lpthread -fPIC -shared -g -O3

clean:
	rm *.so
