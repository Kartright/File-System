fs: fs-sim.o fs-main.o fs-validate.o
	gcc -Wall -Werror fs-sim.o fs-main.o fs-validate.o -o fs
compile: fs-sim.c fs-main.c fs-validate.c
	gcc -Wall -Werror -c fs-sim.c fs-main.c fs-validate.c
clean:
	rm -f fs-sim.o fs-main.o fs-validate.o fs
cleandisk:
	rm -f disk11 disk22 disk00
	./create_fs disk11
	./create_fs disk22
	./create_fs disk00