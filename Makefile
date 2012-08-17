OBJ=$(shell ls -1 src/*.c | sed -r 's|^src/|target/out/|g;s|\.c|.o|g')
PIC_OBJ=$(shell ls -1 src/*.c | sed -r 's|^src/|target/out/|g;s|\.c|.po|g')
TST=$(shell ls -1 test/test*.c | sed -r 's|^test/|target/test/|g;s|\.c|.run|g')

VERSION=$(shell head -n 1 Changelog | egrep -o '[0-9]+\.[0-9]+\.[0-9]+')
MAJOR=$(shell head -n 1 Changelog | egrep -o '[0-9]+')
PROJECT ?= $(shell awk '/^Source:/ {print $$2; exit}' debian/control)
PROJECT_NAME ?= $(shell basename `pwd`)

CFLAGS ?= -g
RUN ?=

LIBRARIES ?=
LINK_LIBS=$(LIBRARIES:lib%=-l%)
CFLAGS += $(LINK_LIBS)
LDFLAGS += $(LINK_LIBS)


all: run-test lib doc
	@echo

install: run-test lib doc
	mkdir -p $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include
	mkdir -p $(DESTDIR)/usr/share/$(PROJECT)
	mkdir -p $(DESTDIR)/usr/share/doc/$(PROJECT)
	cp target/$(PROJECT).so $(DESTDIR)/usr/lib/$(PROJECT).so.$(VERSION)
	ln -sf $(PROJECT).so.$(VERSION) $(DESTDIR)/usr/lib/$(PROJECT).so.0
	ln -sf $(PROJECT).so.$(VERSION) $(DESTDIR)/usr/lib/$(PROJECT).so
	cp target/$(PROJECT).a $(DESTDIR)/usr/lib/$(PROJECT).a
	cp include/*.h $(DESTDIR)/usr/include/
	cp -a target/*.pdf target/doc/html $(DESTDIR)/usr/share/doc/$(PROJECT)/
	-test -e gendoc.sh && cp Makefile release.sh gendoc.sh $(DESTDIR)/usr/share/$(PROJECT) # libcad specific

release: debuild
	@echo "Releasing version $(VERSION)"
	mkdir target/dpkg
	mv ../$(PROJECT)*_$(VERSION)-1_*.deb    target/dpkg/
	mv ../$(PROJECT)_$(VERSION).orig.*      target/dpkg/
	mv ../$(PROJECT)_$(VERSION)-1.debian.*  target/dpkg/
	mv ../$(PROJECT)_$(VERSION)-1.dsc       target/dpkg/
	mv ../$(PROJECT)_$(VERSION)-1_*.build   target/dpkg/
	mv ../$(PROJECT)_$(VERSION)-1_*.changes target/dpkg/
	cd target && tar cfz $(PROJECT)_$(VERSION)_$(shell gcc -v 2>&1 | grep '^Target:' | sed 's/^Target: //').tgz $(PROJECT).so $(PROJECT).pdf $(PROJECT)-htmldoc.tgz dpkg

debuild: run-test lib doc
	debuild -us -uc -v$(VERSION)

lib: target/$(PROJECT).so target/$(PROJECT).a

doc: target/$(PROJECT).pdf target/$(PROJECT)-htmldoc.tgz
	@echo

run-test: target/$(PROJECT).so.0 $(TST)
	@echo

clean:
	@echo "Cleaning"
	rm -rf target
	grep ^Package: debian/control | awk -F: '{print $$2}' | while read pkg; do rm -rf debian/$$pkg; done
	rm -rf debian/tmp
	@echo "Done."

target/test/%.run: target/out/%.exe target/test
	@echo "  Running test: $<"
	LD_LIBRARY_PATH=target:$(LD_LIBRARY_PATH) $< 2>&1 >$(@:.run=.log) && touch $@ || ( LD_LIBRARY_PATH=target $(RUN) $<; exit 1 )

target:
	mkdir -p target/out/data

target/test: $(shell find test/data -type f)
	mkdir -p target/test
	cp -a test/data/* target/out/data/; done

target/$(PROJECT).so: target $(PIC_OBJ)
	@echo "Linking shared library: $@"
	$(CC) -shared -fPIC -Wl,-z,defs,-soname=$(PROJECT).so.0 $(LDFLAGS) -o $@ $(PIC_OBJ)
	strip --strip-unneeded $@
	@echo

target/$(PROJECT).so.0: target/$(PROJECT).so
	cd target && ln -sf $(PROJECT).so $(PROJECT).so.0

target/$(PROJECT).a: target $(OBJ)
	@echo "Linking static library: $@"
	ar rcs $@ $(OBJ)
	@echo

target/$(PROJECT).pdf: target/doc/latex/refman.pdf
	@echo "    Saving PDF"
	cp $< $@

target/doc/latex/refman.pdf: target/doc/latex/Makefile target/doc/latex/version.tex
	@echo "  Building PDF"
	find target/doc/latex -name \*.tex -exec sed 's!\\-\\_\\-\\-\\_\\-\\-P\\-U\\-B\\-L\\-I\\-C\\-\\_\\-\\-\\_\\- !!g' -i {} \;
	sed -r 's!^(\\fancyfoot\[(RE|LO)\]\{\\fancyplain\{\}\{).*$$!\1\\scriptsize \\url{http://www.github.com/cadrian/$(PROJECT_NAME)}}}!' -i target/doc/latex/doxygen.sty
	sed -r 's!^(\\fancyfoot\[(LE|RO)\]\{\\fancyplain\{\}\{).*$$!\1\\scriptsize '$(PROJECT_NAME)' '$(VERSION)'}}!' -i target/doc/latex/doxygen.sty
	echo '\\renewcommand{\\footrulewidth}{0.4pt}' >> target/doc/latex/doxygen.sty
	make -C target/doc/latex > target/doc/make.log 2>&1

target/$(PROJECT)-htmldoc.tgz: target/doc/html/index.html
	@echo "  Building HTML archive"
	(cd target/doc/html; tar cfz - *) > $@

target/doc/latex/version.tex: target/version
	cp $< $@

target/version: Changelog
	echo $(VERSION) > $@

target/doc/latex/Makefile: target/doc/.doc
	sleep 1; touch $@

target/doc/html/index.html: target/doc/.doc
	sleep 1; touch $@

target/doc/.doc: Doxyfile target/gendoc.sh target $(shell ls -1 src/*.c include/*.h doc/*)
	@echo "Generating documentation"
	target/gendoc.sh
	doxygen $< && touch $@

target/gendoc.sh:
	if test -e gendoc.sh; then ln -sf '../gendoc.sh' $@; else ln -sf /usr/share/libcad/gendoc.sh $@; fi

target/out/%.o: src/%.c include/*.h
	@echo "Compiling library object: $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fvisibility=hidden -I include -c $< -o $@

target/out/%.po: src/%.c include/*.h
	@echo "Compiling PIC library object: $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -fvisibility=hidden -I include -c $< -o $@

target/out/%.exe: test/%.c test/*.h target/$(PROJECT).so
	@echo "Compiling test: $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -I include -L target $(PROJECT:lib%=-l%) $< -o $@

.PHONY: all lib doc clean run-test release debuild
#.SILENT:
