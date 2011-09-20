LDMDIR=/sampa/home/blucia/cvsandbox/Legerdemain

all: MallocWatcher 

MallocWatcher:
	gcc MallocWatcher.c -o MallocWatcher.so -I$(LDMDIR) -fPIC -shared -g -O3 -ldl 

clean:
	rm *.so
