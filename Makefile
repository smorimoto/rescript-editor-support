SHELL = /bin/zsh
MAKEFLAGS += --jobs 4
INCLUDES = -I src -I src/vendor/odoc_parser -I src/vendor/omd -I src/vendor/res_outcome_printer -I src/vendor

OCAMLOPT = ocamlopt.opt
OCAMLFLAGS = -g -w +26+27+32+33+39 -bin-annot -I +compiler-libs $(INCLUDES)
OCAMLDEP = ocamldep.opt

%.cmi : %.mli
	@echo Building $@
	@$(OCAMLOPT) $(OCAMLFLAGS) -c $<
%.cmx : %.ml
	@echo Building $@
	@$(OCAMLOPT) $(OCAMLFLAGS) -c $<

include .depend
depend:
	@$(OCAMLDEP) -native $(INCLUDES) src/**/*.(ml|mli) > .depend

SOURCE_FILES = $(shell $(OCAMLDEP) -sort src/**/*.ml | sed -E "s/\.ml/.cmx/g")

lib/rescript-editor-support.exe: $(SOURCE_FILES)
	@echo Linking...
	@$(OCAMLOPT) $(OCAMLFLAGS) -O2 -o ./lib/rescript-editor-support.exe \
		-I +compiler-libs unix.cmxa str.cmxa ocamlcommon.cmxa $(INCLUDES) $(SOURCE_FILES)
	@echo Done!

build-native: lib/rescript-editor-support.exe depend

clean:
	git clean -dfx src

.DEFAULT_GOAL := build-native

.PHONY: depend clean build-native
