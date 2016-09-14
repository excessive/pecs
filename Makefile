all: project
	+make -C build

project:
	@genie gmake

run: all
	@exec ./bin/pecs

clean:
	+make -C build clean

.PHONY: all clean project run
