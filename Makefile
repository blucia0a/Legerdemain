all: applier scaley ldm 
applier:
	gcc Applier.c -o libapplier.so -fPIC -shared -g -O3 -ldl
ldm:
	gcc Applier.c legerdemain.c -o libldm.so -fPIC -shared -g -O3 -ldl
scaley:
	gcc scaley_LDM.c -o scaley_LDM.so -L. -lapplier -fPIC -shared -g -O3 -ldl
test:
	gcc scaleytest.c -o scaleytest -g -O3 -L. -lldm 
clean:
	rm *.so 
