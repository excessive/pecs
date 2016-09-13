all: project
	+make -C build

project:
	@genie gmake

run: all
	@exec ./bin/pecs

.PHONY: all project run
