#
# download and extract required packages
#

all: llvm-link


# for LLVM plugin (re-route DBrew output into LLVM)
# Ubuntu-14.04 version also works on 15.10/16.04
LLVM_VERSION=3.8.1
LLVM_PKG=clang+llvm-$(LLVM_VERSION)-x86_64-linux-gnu-ubuntu-14.04
LLVM_PKGFILE=$(LLVM_PKG).tar.xz

# force llvm link to be recreated every time
llvm-link: llvm-$(LLVM_VERSION)
	rm -f llvm
	ln -s llvm-$(LLVM_VERSION) llvm

llvm-$(LLVM_VERSION): pkgs/$(LLVM_PKGFILE)
	cd pkgs && tar xvf $(LLVM_PKGFILE) $(LLVM_PKG)/lib $(LLVM_PKG)/include $(LLVM_PKG)/bin/llvm-config
	ln -s pkgs/$(LLVM_PKG) llvm-$(LLVM_VERSION)

pkgs/$(LLVM_PKGFILE):
	mkdir -p pkgs
	#cd pkgs && cp ../p/$(LLVM_PKGFILE) .
	cd pkgs && wget http://llvm.org/releases/$(LLVM_VERSION)/$(LLVM_PKGFILE)


clean:
	rm -f llvm llvm-$(LLVM_VERSION)
	rm -rf pkgs/$(LLVM_PKG)

clean-all: clean
	rm -rf pkgs

