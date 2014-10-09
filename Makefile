git = git -c user.name="Auto" -c user.email="auto@auto.com" 

BUILDDIR = $(PWD)/build
SRCDIR = $(PWD)/src

all : release

.PHONY : release debug run clean patch

release : | build/buildr
	make -C $(BUILDDIR)/buildr
	cp -f $(BUILDDIR)/buildr/tara tara

debug : | build/buildd
	make -C $(BUILDDIR)/buildd
	cp -f $(BUILDDIR)/buildd/tara tara

build/buildr: build/z3/buildr/libz3.so
	mkdir -p $(BUILDDIR)/buildr
	cd $(BUILDDIR)/buildr; cmake -DCMAKE_BUILD_TYPE=Release $(SRCDIR)

build/buildd: build/z3/buildr/libz3.so
	mkdir -p $(BUILDDIR)/buildd
	cd $(BUILDDIR)/buildd; cmake -DCMAKE_BUILD_TYPE=Debug $(SRCDIR)

clean :
	rm -rf $(BUILDDIR)/buildr
	rm -rf $(BUILDDIR)/buildd
	rm -f tara

patch :
	mkdir -p z3-patch
	cd z3; $(git) diff > ../z3-patch/z3.patch

build/z3/patched : src/z3-patch/z3.patch src/z3-patch/smt_model_reporter.cpp | build/z3
	cd $(BUILDDIR)/z3; $(git) stash clear && $(git) stash save && $(git) apply --whitespace=fix $(SRCDIR)/z3-patch/z3.patch
	cd $(BUILDDIR)/z3/src/smt; ln -sf $(SRCDIR)/z3-patch/smt_model_reporter.cpp smt_model_reporter.cpp
	touch $(BUILDDIR)/z3/patched

build/z3 : 
	wget -O $(BUILDDIR)/z3.zip 'http://download-codeplex.sec.s-msft.com/Download/SourceControlFileDownload.ashx?ProjectName=z3&changeSetId=4732e03259487da4a45391a6acc45b6adb8a4a3e'
	cd $(BUILDDIR);$(git) init z3
	cd $(BUILDDIR);unzip z3.zip -d z3
	cd $(BUILDDIR)/z3; $(git) add -A; $(git) commit -m "clean z3 version"

build/z3/buildr/libz3.so : build/z3/patched
	rm -rf $(BUILDDIR)/z3/buildr
	cd $(BUILDDIR)/z3; python scripts/mk_make.py --staticlib -b buildr
	make -C $(BUILDDIR)/z3/buildr

build/z3/buildr/libz3.a : build/z3/buildr/libz3.so

build/z3/buildd/libz3.so : build/z3/patched
	rm -rf $(BUILDDIR)/z3/buildd
	cd $(BUILDDIR)/z3; python scripts/mk_make.py --staticlib -d -b buildd
	make -C $(BUILDDIR)/z3/buildd



