
target = bin/lsv1.0.0


all: ${target}


${target}: obj/lsv1.0.0.o
	gcc obj/lsv1.0.0.o -o bin/lsv1.0.0


obj/lsv1.0.0.o: src/lsv1.0.0.c
	gcc -c src/lsv1.0.0.c -o obj/lsv1.0.0.o


clean: 
	rm -f obj/*.o ${target}
