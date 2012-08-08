OBJ=$(shell ls -1 src/*.c | sed -r 's|^src/|target/out/|g;s|\.c|.o|g')
PIC_OBJ=$(shell ls -1 src/*.c | sed -r 's|^src/|target/out/|g;s|\.c|.po|g')
TST=$(shell ls -1 test/test*.c | sed -r 's|^test/|target/test/|g;s|\.c|.run|g')

CFLAGS ?= -g
RUN ?=

all: run-test lib doc
	echo

install: run-test lib doc
	echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	env | grep libcad
	echo
	if [ -d $(DESTDIR) ]; then find $(DESTDIR) -name libcad -exec echo {} : \; -exec ls {} \; ; fi
	echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	mkdir -p $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include
	mkdir -p $(DESTDIR)/usr/share/doc/libcad
	cp target/libcad.so $(DESTDIR)/usr/lib/libcad.so.$(shell cat target/version)
	ln -sf libcad.so.$(shell cat target/version) $(DESTDIR)/usr/lib/libcad.so.0
	ln -sf libcad.so.$(shell cat target/version) $(DESTDIR)/usr/lib/libcad.so
	cp target/libcad.a $(DESTDIR)/usr/lib/libcad.a
	cp include/*.h $(DESTDIR)/usr/include/
	cp -a target/*.pdf target/doc/html $(DESTDIR)/usr/share/doc/libcad/

release: debuild
	echo Releasing version $(shell cat target/version)
	mkdir target/dpkg
	mv ../libcad*_$(shell cat target/version)-1_*.deb    target/dpkg/
	mv ../libcad_$(shell cat target/version).orig.*      target/dpkg/
	mv ../libcad_$(shell cat target/version)-1.debian.*  target/dpkg/
	mv ../libcad_$(shell cat target/version)-1.dsc       target/dpkg/
	mv ../libcad_$(shell cat target/version)-1_*.build   target/dpkg/
	mv ../libcad_$(shell cat target/version)-1_*.changes target/dpkg/
	cd target && tar cfz libcad_$(shell cat target/version)_$(shell gcc -v 2>&1 | grep '^Target:' | sed 's/^Target: //').tgz libcad.so libcad.pdf libcad-htmldoc.tgz dpkg

debuild: run-test lib doc
	debuild -us -uc -v$(shell cat target/version)

lib: target/libcad.so target/libcad.a

doc: target/libcad.pdf target/libcad-htmldoc.tgz
	echo

run-test: target/libcad.so.0 $(TST)
	echo

clean:
	echo "Cleaning"
	rm -rf target
	grep ^Package: debian/control | awk -F: '{print $$2}' | while read pkg; do rm -rf debian/$$pkg; done
	rm -rf debian/tmp
	echo "Done."

target/test/%.run: target/out/%.exe target/test
	echo "  Running test: $<"
	LD_LIBRARY_PATH=target:$(LD_LIBRARY_PATH) $< 2>&1 >$(@:.run=.log) && touch $@ || ( LD_LIBRARY_PATH=target $(RUN) $<; exit 1 )

target:
	mkdir -p target/out/data

target/test: $(shell find test/data -type f)
	mkdir -p target/test
	cp -a test/data/* target/out/data/; done

target/libcad.so: target $(PIC_OBJ)
	echo "Linking shared library: $@"
	$(CC) -shared -fPIC -Wl,-z,defs,-soname=libcad.so.0 -o $@ $(PIC_OBJ)
	strip --strip-unneeded $@
	echo

target/libcad.so.0: target/libcad.so
	cd target && ln -sf libcad.so libcad.so.0

target/libcad.a: target $(OBJ)
	echo "Linking static library: $@"
	ar rcs $@ $(OBJ)
	echo

target/libcad.pdf: target/doc/latex/refman.pdf
	echo "    Saving PDF"
	cp $< $@

target/doc/latex/refman.pdf: target/doc/latex/Makefile target/doc/latex/version.tex
	echo "  Building PDF"
	find target/doc/latex -name \*.tex -exec sed 's!\\-\\_\\-\\-\\_\\-\\-P\\-U\\-B\\-L\\-I\\-C\\-\\_\\-\\-\\_\\- !!g' -i {} \;
	sed -r 's!^(\\fancyfoot\[(RE|LO)\]\{\\fancyplain\{\}\{).*$$!\1\\scriptsize \\url{http://www.github.com/cadrian/libcad}}}!' -i target/doc/latex/doxygen.sty
	sed -r 's!^(\\fancyfoot\[(LE|RO)\]\{\\fancyplain\{\}\{).*$$!\1\\scriptsize Yac\\-J\\-P '$(shell cat target/version)'}}!' -i target/doc/latex/doxygen.sty
	echo '\\renewcommand{\\footrulewidth}{0.4pt}' >> target/doc/latex/doxygen.sty
	make -C target/doc/latex > target/doc/make.log 2>&1

target/libcad-htmldoc.tgz: target/doc/html/index.html
	echo "  Building HTML archive"
	(cd target/doc/html; tar cfz - *) > $@

target/doc/latex/version.tex: target/version
	cp $< $@

target/version: Changelog
	head -n 1 $< | egrep -o '[0-9]+\.[0-9]+\.[0-9]+' > $@

target/doc/latex/Makefile: target/doc/.doc
	sleep 1; touch $@

target/doc/html/index.html: target/doc/.doc
	sleep 1; touch $@

target/doc/.doc: Doxyfile gendoc.sh target $(shell ls -1 src/*.c include/*.h doc/*)
	echo "Generating documentation"
	./gendoc.sh
	doxygen $< && touch $@

target/out/%.o: src/%.c include/*.h
	echo "Compiling library object: $<"
	$(CC) $(CFLAGS) -fvisibility=hidden -I include -c $< -o $@

target/out/%.po: src/%.c include/*.h
	echo "Compiling PIC library object: $<"
	$(CC) $(CFLAGS) -fPIC -fvisibility=hidden -I include -c $< -o $@

target/out/%.exe: test/%.c test/*.h target/libcad.so
	echo "Compiling test: $<"
	$(CC) $(CFLAGS) -I include -L target -lcad $< -o $@

.PHONY: all lib doc clean run-test release debuild
.SILENT:
