all:
	+make -C build
run: all
	exec ./bin/pecs