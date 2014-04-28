all: packdude/packdude dudepack/dudepack

libpackdude/libpackdude.a:
	cd libpackdude; $(MAKE)

packdude/packdude: libpackdude/libpackdude.a
	cd packdude; $(MAKE)

dudepack/dudepack:
	cd dudepack; $(MAKE)

clean:
	cd libpackdude; $(MAKE) clean
	cd packdude; $(MAKE) clean
	cd dudepack; $(MAKE) clean
