label = lsv1.1.0
target = bin/${label}
prefix = /usr/local/bin

all: ${target}


${target}: obj/${label}.o
	gcc obj/${label}.o -o bin/${label}


obj/${label}.o: src/${label}.c
	gcc -c src/${label}.c -o obj/${label}.o


clean: 
	rm -f obj/*.o ${target}

install: ${target}
	@echo "Installing ${label} to ${prefix}"
	mkdir -p ${prefix}
	cp ${target} ${prefix}/

uninstall:
	@echo "Uninstalling ${label} from ${prefix}"
	rm -f ${prefix}/${label}

