OBJ=$(shell ls -1 src/*.c | sed -r 's|^src/|target/out/|g;s|\.c|.o|g')
PIC_OBJ=$(shell ls -1 src/*.c | sed -r 's|^src/|target/out/|g;s|\.c|.po|g')
TST=$(shell ls -1 test/test*.c | sed -r 's|^test/|target/test/|g;s|\.c|.run|g')

PROJECT ?= $(shell awk '/^Source:/ {print $$2; exit}' build/debian.main/control)
PROJECT_NAME ?= $(shell basename `pwd`)

CFLAGS ?= -g
LDFLAGS ?=
RUN ?=

LIBRARIES ?= libpthread
LINK_LIBS=$(LIBRARIES:lib%=-l%)

BUILD_DIR ?= $(shell pwd)

AOBJ=$(PROJECT).a
ifeq "$(wildcard /etc/setup/setup.rc)" ""
SOBJ=$(PROJECT).so
PICFLAG=-fPIC
else
SOBJ=cyg$(shell echo $(PROJECT) | sed 's/^lib//').dll
PICFLAG=
endif

ifeq "$(wildcard /usr/bin/doxygen)" ""
all: run-test lib
else
all: run-test lib doc
endif
	@echo

install: target/version lib doc
	mkdir -p debian/tmp/usr/lib
	mkdir -p debian/tmp/usr/include
	mkdir -p debian/tmp/usr/share/$(PROJECT)
	mkdir -p debian/tmp/usr/share/doc/$(PROJECT)-doc
	strip --strip-unneeded target/$(SOBJ)
	-cp target/$(SOBJ) debian/tmp/usr/lib/$(SOBJ).$(shell cat target/version)
	-ln -sf $(SOBJ).$(shell cat target/version) debian/tmp/usr/lib/$(SOBJ).0
	-ln -sf $(SOBJ).$(shell cat target/version) debian/tmp/usr/lib/$(SOBJ)
	-cp target/$(AOBJ) debian/tmp/usr/lib/$(AOBJ)
	-cp include/*.h debian/tmp/usr/include/
	-cp -a target/*.pdf target/doc/html debian/tmp/usr/share/doc/$(PROJECT)-doc/
	-test -e gendoc.sh && cp Makefile release.sh gendoc.sh debian/tmp/usr/share/$(PROJECT)/ # libcad specific
	env | sort

override_dh_strip:
	dh_strip --dbg-package=$(shell awk '/^Package: .*-dbg$/ {print $2}' build/debian.main/control)

release.main: target/dpkg/release.main release.doc

target/dpkg/release.main: run-test lib target/version
	@echo "Releasing main version $(shell cat target/version)"
	debuild -us -uc -nc -F
	mkdir -p target/dpkg
	mv ../$(PROJECT)*_$(shell cat target/version)_*.deb    target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version).dsc       target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version).tar.[gx]z target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version)_*.build   target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version)_*.changes target/dpkg/
	(cd target && tar cfz $(PROJECT)_$(shell cat target/version)_$(shell gcc -v 2>&1 | grep '^Target:' | sed 's/^Target: //').tgz $(SOBJ) $(PROJECT).pdf $(PROJECT)-htmldoc.tgz dpkg)
	touch target/dpkg/release.main

lib: target/$(SOBJ) target/$(AOBJ)

run-test: target/$(SOBJ).0 $(TST)
	@echo

clean:
	@echo "Cleaning"
	rm -rf target debian
	@echo "Done."

target/test/%.run: target/out/%.exe target/test
	@echo "	 Running test: $<"
	LD_LIBRARY_PATH=$(BUILD_DIR)/target:$(LD_LIBRARY_PATH) $< 2>&1 >$(@:.run=.log) && touch $@ || ( LD_LIBRARY_PATH=$(BUILD_DIR)/target:$(LD_LIBRARY_PATH) $(RUN) $<; exit 1 )

target/test: $(shell find test/data -type f)
	mkdir -p target/test target/out/data
	cp -a test/data/* target/out/data/; done

ifeq "$(wildcard /etc/setup/setup.rc)" ""
target/$(SOBJ): $(PIC_OBJ)
	@echo "Linking shared library: $@"
	$(CC) -shared $(PICFLAG) -Wl,-z,defs,-soname=$(PROJECT).so.0 $(LDFLAGS) -o $@ $(PIC_OBJ) -Wall -Werror $(LINK_LIBS)
	@echo
else
target/$(SOBJ): $(PIC_OBJ)
	@echo "Linking shared library: $@"
	$(CC) -shared -o $@ -Wall -Werror \
		-Wl,--out-implib=target/$(PROJECT).dll.a \
		-Wl,--export-all-symbols \
		-Wl,--enable-auto-import \
		-Wl,--whole-archive $(PIC_OBJ) \
		-Wl,--no-whole-archive $(LDFLAGS) $(LINK_LIBS)
	@echo
endif

target/$(AOBJ): $(OBJ)
	@echo "Linking static library: $@"
	ar rcs $@ $(OBJ)
	@echo

target/$(SOBJ).0: target/$(SOBJ)
	(cd target && ln -sf $(PROJECT).so $(PROJECT).so.0)

ifeq "$(wildcard /usr/bin/doxygen)" ""
release.doc:
else
release.doc: target/dpkg/release.doc

target/dpkg/release.doc: doc
	touch target/dpkg/release.doc

doc: target/$(PROJECT).pdf target/$(PROJECT)-htmldoc.tgz
	@echo

target/$(PROJECT).pdf: target/doc/latex/refman.pdf
	@echo "	   Saving PDF"
	cp $< $@

target/doc/latex/refman.pdf: target/doc/latex/Makefile target/doc/latex/version.tex
	@echo "	 Building PDF"
	-yes x | $(MAKE) -C target/doc/latex #> target/doc/make.log 2>&1
	cat target/doc/latex/refman.log

target/$(PROJECT)-htmldoc.tgz: target/doc/html/index.html
	@echo "	 Building HTML archive"
	(cd target/doc/html; tar cfz - *) > $@

target/doc/latex/version.tex: target/version
	cp $< $@

target/doc/latex/Makefile: target/doc/.doc
	sleep 1; touch $@

target/doc/html/index.html: target/doc/.doc
	sleep 1; touch $@

target/doc/.doc: Doxyfile target/gendoc.sh $(shell ls -1 src/*.c include/*.h doc/*)
	@echo "Generating documentation"
	target/gendoc.sh
	-doxygen -u $<
	doxygen $< && touch $@
#	remove the \batchmode on the first line:
	mv target/doc/latex/refman.tex target/doc/latex/refman.tex.orig
	tail -n +2 target/doc/latex/refman.tex.orig > target/doc/latex/refman.tex

target/gendoc.sh:
	if test -e gendoc.sh; then ln -sf '../gendoc.sh' $@; else ln -sf /usr/share/libcad/gendoc.sh $@; fi
endif

target/version: debian/changelog
	mkdir -p target
	head -n 1 debian/changelog | awk -F'[()]' '{print $$2}' > $@

debian/changelog: debian/changelog.raw
	sed "s/#DATE#/$(shell date -R)/;s/#SNAPSHOT#/$(shell date -u +'~%Y%m%d%H%M%S')/" < $< > $@

debian/changelog.raw:
	./build/build.sh

target/out/%.o: src/%.c include/*.h
	@echo "Compiling library object: $<"
	mkdir -p target/out
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -Werror -fvisibility=hidden -I $(BUILD_DIR)/include -c $< -o $@

target/out/%.po: src/%.c include/*.h
	@echo "Compiling PIC library object: $<"
	mkdir -p target/out
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -Werror $(PICFLAG) -fvisibility=hidden -I $(BUILD_DIR)/include -c $< -o $@

target/out/%.exe: test/%.c test/*.h target/$(SOBJ).0
	@echo "Compiling test: $<"
	mkdir -p target/out
	-cp -fp $(<:.c=.sh) target/out/
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -Werror -L $(BUILD_DIR)/target -I $(BUILD_DIR)/include $(LDFLAGS) -o $@ $< $(PROJECT:lib%=-l%) $(LINK_LIBS)

.PHONY: all lib doc clean run-test release.main release.doc
#.SILENT:
