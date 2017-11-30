git = git -c user.name="Auto" -c user.email="auto@auto.com" 

BUILDDIR = $(PWD)/build
SRCDIR = $(PWD)/src
LLVM_VERSION=4.0.0

all : release

.PHONY : release debug run clean patch test

release : build/buildr/Makefile
	+make -C $(BUILDDIR)/buildr
	cp -f $(BUILDDIR)/buildr/tara tara

debug : build/buildd/Makefile
	+make -C $(BUILDDIR)/buildd
	cp -f $(BUILDDIR)/buildd/tara tara

build/buildr/Makefile: build/z3/buildr/libz3.so
	mkdir -p $(BUILDDIR)/buildr
	cd $(BUILDDIR)/buildr; cmake -DCMAKE_BUILD_TYPE=Release $(SRCDIR)

build/buildd/Makefile: build/z3/buildd/libz3.so build/llvm-$(LLVM_VERSION)/lib/libLLVMCore.a
	mkdir -p $(BUILDDIR)/buildd
	cd $(BUILDDIR)/buildd; cmake -DCMAKE_BUILD_TYPE=Debug -DLLVM_VERSION=$(LLVM_VERSION) -DZ3_DEBUG:BOOL=TRUE $(SRCDIR)

clean :
	rm -rf $(BUILDDIR)/buildr
	rm -rf $(BUILDDIR)/buildd
	rm -f tara
	find -name "*~"| xargs rm -rf


#-----------------------------------------------------------------------------
# Z3 fetch and patch generation

patch :
	mkdir -p src/z3-patch
	cd $(BUILDDIR)/z3; $(git) diff > ../../src/z3-patch/z3.patch

build/z3/README.md : 
	mkdir -p $(BUILDDIR)
	# todo: switch to https; someone may not have ssh access configured
	cd $(BUILDDIR);$(git) clone git@github.com:Z3Prover/z3.git
	cd $(BUILDDIR)/z3;$(git) checkout b8716b333908273ad8e27e325a8bea9be0596be3


NEW_Z3_FILES =  src/z3-patch/smt_model_reporter.cpp \
		src/z3-patch/special_relations_decl_plugin.cpp \
		src/z3-patch/special_relations_decl_plugin.h \
		src/z3-patch/theory_special_relations.cpp \
		src/z3-patch/theory_special_relations.h \
		src/z3-patch/api_special_relations.cpp


build/z3/patched : src/z3-patch/z3.patch $(NEW_Z3_FILES) build/z3/README.md
	cd $(BUILDDIR)/z3; $(git) stash clear && $(git) stash save && $(git) apply --whitespace=fix $(SRCDIR)/z3-patch/z3.patch
	cd $(BUILDDIR)/z3/src/smt; ln -sf $(SRCDIR)/z3-patch/smt_model_reporter.cpp smt_model_reporter.cpp; ln -sf $(SRCDIR)/z3-patch/theory_special_relations.cpp theory_special_relations.cpp; ln -sf $(SRCDIR)/z3-patch/theory_special_relations.h theory_special_relations.h
	cd $(BUILDDIR)/z3/src/ast; ln -sf $(SRCDIR)/z3-patch/special_relations_decl_plugin.cpp special_relations_decl_plugin.cpp; ln -sf $(SRCDIR)/z3-patch/special_relations_decl_plugin.h special_relations_decl_plugin.h
	cd $(BUILDDIR)/z3/src/api; ln -sf $(SRCDIR)/z3-patch/api_special_relations.cpp api_special_relations.cpp
	touch $(BUILDDIR)/z3/patched

build/z3/buildr/libz3.so : build/z3/patched
	rm -rf $(BUILDDIR)/z3/buildr
	cd $(BUILDDIR)/z3; python scripts/mk_make.py --staticlib -b buildr
	+make -C $(BUILDDIR)/z3/buildr

build/z3/buildr/libz3.a : build/z3/buildr/libz3.so

build/z3/buildd/libz3.so : build/z3/patched
	rm -rf $(BUILDDIR)/z3/buildd
	cd $(BUILDDIR)/z3; python scripts/mk_make.py --staticlib -d -b buildd
	+make -C $(BUILDDIR)/z3/buildd

#---------------------------------------------------------------------------
# fetch and install local llvm with debugging enabled

build/llvm-$(LLVM_VERSION).src/README.txt:
	cd $(BUILDDIR);wget http://releases.llvm.org/$(LLVM_VERSION)/llvm-$(LLVM_VERSION).src.tar.xz
	cd $(BUILDDIR);tar -xvJf llvm-$(LLVM_VERSION).src.tar.xz; mkdir -p llvm-$(LLVM_VERSION).src/build; mkdir -p llvm-$(LLVM_VERSION)

build/llvm-$(LLVM_VERSION)/lib/libLLVMCore.a : build/llvm-$(LLVM_VERSION).src/README.txt
	cd $(BUILDDIR)/llvm-$(LLVM_VERSION).src/build;cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=DEBUG -DLLVM_ENABLE_RTTI:BOOL=TRUE -DCMAKE_INSTALL_PREFIX=../../llvm-$(LLVM_VERSION) ../
	+make -C $(BUILDDIR)/llvm-$(LLVM_VERSION).src/build
	+make -C $(BUILDDIR)/llvm-$(LLVM_VERSION).src/build install

#---------------------------------------------------------------------------

runtest:
	make -C examples/tester/

test: release runtest
