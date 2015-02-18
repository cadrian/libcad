OBJ=$(shell ls -1 src/*.c | sed -r 's|^src/|target/out/|g;s|\.c|.o|g')
PIC_OBJ=$(shell ls -1 src/*.c | sed -r 's|^src/|target/out/|g;s|\.c|.po|g')
TST=$(shell ls -1 test/test*.c | sed -r 's|^test/|target/test/|g;s|\.c|.run|g')

PROJECT ?= $(shell awk '/^Source:/ {print $$2; exit}' debian/control)
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
else
SOBJ=cyg$(shell echo $(PROJECT) | sed 's/^lib//').dll
endif

all: run-test lib doc
	@echo

install: run-test lib doc target/version
	mkdir -p $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include
	mkdir -p $(DESTDIR)/usr/share/$(PROJECT)
	mkdir -p $(DESTDIR)/usr/share/doc/$(PROJECT)
	cp target/$(SOBJ) $(DESTDIR)/usr/lib/$(SOBJ).$(shell cat target/version)
	ln -sf $(SOBJ).$(shell cat target/version) $(DESTDIR)/usr/lib/$(SOBJ).0
	ln -sf $(SOBJ).$(shell cat target/version) $(DESTDIR)/usr/lib/$(SOBJ)
	cp target/$(AOBJ) $(DESTDIR)/usr/lib/$(AOBJ)
	cp include/*.h $(DESTDIR)/usr/include/
	cp -a target/*.pdf target/doc/html $(DESTDIR)/usr/share/doc/$(PROJECT)/
	-test -e gendoc.sh && cp Makefile release.sh gendoc.sh $(DESTDIR)/usr/share/$(PROJECT) # libcad specific

release: debuild target/version
	@echo "Releasing version $(shell cat target/version)"
	mkdir target/dpkg
	mv ../$(PROJECT)*_$(shell cat target/version)_*.deb    target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version).dsc	      target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version).tar.gz    target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version)_*.build   target/dpkg/
	mv ../$(PROJECT)_$(shell cat target/version)_*.changes target/dpkg/
	(cd target && tar cfz $(PROJECT)_$(shell cat target/version)_$(shell gcc -v 2>&1 | grep '^Target:' | sed 's/^Target: //').tgz $(SOBJ) $(PROJECT).pdf $(PROJECT)-htmldoc.tgz dpkg)

debuild: run-test lib doc
	debuild -us -uc

lib: target target/$(SOBJ) target/$(AOBJ)

doc: target target/$(PROJECT).pdf target/$(PROJECT)-htmldoc.tgz
	@echo

run-test: target target/$(SOBJ).0 $(TST)
	@echo

clean:
	@echo "Cleaning"
	rm -rf target
	grep ^Package: debian/control | awk -F: '{print $$2}' | while read pkg; do rm -rf debian/$$pkg; done
	rm -rf debian/tmp
	@echo "Done."

target/test/%.run: target/out/%.exe target/test
	@echo "	 Running test: $<"
	LD_LIBRARY_PATH=$(BUILD_DIR)/target:$(LD_LIBRARY_PATH) $< 2>&1 >$(@:.run=.log) && touch $@ || ( LD_LIBRARY_PATH=$(BUILD_DIR)/target:$(LD_LIBRARY_PATH) $(RUN) $<; exit 1 )

target:
	mkdir -p target/out/data

target/test: $(shell find test/data -type f)
	mkdir -p target/test
	cp -a test/data/* target/out/data/; done

ifeq "$(wildcard /etc/setup/setup.rc)" ""
target/$(SOBJ): $(PIC_OBJ)
	@echo "Linking shared library: $@"
	$(CC) -shared -fPIC -Wl,-z,defs,-soname=$(PROJECT).so.0 $(LDFLAGS) -o $@ $(PIC_OBJ) $(LINK_LIBS) \
	strip --strip-unneeded $@
	@echo
else
target/$(SOBJ): $(PIC_OBJ)
	@echo "Linking shared library: $@"
	$(CC) -shared -o $@ \
		-Wl,--out-implib=target/$(PROJECT).dll.a \
		-Wl,--export-all-symbols \
		-Wl,--enable-auto-import \
		-Wl,--whole-archive $(PIC_OBJ) \
		-Wl,--no-whole-archive $(LDFLAGS) $(LINK_LIBS)
#	strip --strip-unneeded $@
	@echo
endif

target/$(AOBJ): $(OBJ)
	@echo "Linking static library: $@"
	ar rcs $@ $(OBJ)
	@echo

target/$(SOBJ).0: target/$(SOBJ)
	(cd target && ln -sf $(PROJECT).so $(PROJECT).so.0)

target/$(PROJECT).pdf: target/doc/latex/refman.pdf
	@echo "	   Saving PDF"
	cp $< $@

target/doc/latex/refman.pdf: target/doc/latex/Makefile target/doc/latex/version.tex
	@echo "	 Building PDF"
#	remove the \batchmode on the first line:
	mv target/doc/latex/refman.tex target/doc/latex/refman.tex.orig
	tail -n +2 target/doc/latex/refman.tex.orig > target/doc/latex/refman.tex
	-yes x | $(MAKE) -C target/doc/latex #> target/doc/make.log 2>&1
	cat target/doc/latex/refman.log

target/$(PROJECT)-htmldoc.tgz: target/doc/html/index.html
	@echo "	 Building HTML archive"
	(cd target/doc/html; tar cfz - *) > $@

target/doc/latex/version.tex: target/version
	cp $< $@

target/version: debian/changelog
	head -n 1 debian/changelog | awk -F'[()]' '{print $$2}' > $@

debian/changelog: debian/changelog.raw
	sed "s/#DATE#/$(shell date -R)/;s/#SNAPSHOT#/$(shell date -u +'~%Y%m%d%H%M%S')/" < $< > $@

target/doc/latex/Makefile: target/doc/.doc
	sleep 1; touch $@

target/doc/html/index.html: target/doc/.doc
	sleep 1; touch $@

target/doc/.doc: Doxyfile target/gendoc.sh $(shell ls -1 src/*.c include/*.h doc/*)
	@echo "Generating documentation"
	target/gendoc.sh
	-doxygen -u $<
	doxygen $< && touch $@

target/gendoc.sh:
	if test -e gendoc.sh; then ln -sf '../gendoc.sh' $@; else ln -sf /usr/share/libcad/gendoc.sh $@; fi

target/out/%.o: src/%.c include/*.h
	@echo "Compiling library object: $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -fvisibility=hidden -I $(BUILD_DIR)/include -c $< -o $@

target/out/%.po: src/%.c include/*.h
	@echo "Compiling PIC library object: $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -fPIC -fvisibility=hidden -I $(BUILD_DIR)/include -c $< -o $@

target/out/%.exe: test/%.c test/*.h target/$(PROJECT).so
	@echo "Compiling test: $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wall -L $(BUILD_DIR)/target -I $(BUILD_DIR)/include -o $@ $< $(LINK_LIBS) $(PROJECT:lib%=-l%)

.PHONY: all lib doc clean run-test release debuild
#.SILENT:
