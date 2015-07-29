lmao: bin/lmao
	cp bin/lmao .

all: lmao doc

doc: Doxyfile src/*.c src/*.h
	doxygen Doxyfile

bin/lmao: src/*.c src/*.h src/lmao.y src/lmao.l
	make -C src/

install: lmao
	install -Dm755 ./lmao $(DESTDIR)/usr/bin/lmao
	install -Dm644 ./example_cat_halt_on_eof.hell $(DESTDIR)/usr/share/lmao/example_cat_halt_on_eof.hell
	install -Dm644 ./example_hello_world.hell $(DESTDIR)/usr/share/lmao/example_hello_world.hell
	install -Dm644 ./example_simple_hello_world.hell $(DESTDIR)/usr/share/lmao/example_simple_hello_world.hell
	install -Dm644 ./example_simple_cat.hell $(DESTDIR)/usr/share/lmao/example_simple_cat.hell

uninstall:
	rm -f $(DESTDIR)/usr/bin/lmao
	rm -f $(DESTDIR)/usr/share/lmao/example_cat_halt_on_eof.hell
	rm -f $(DESTDIR)/usr/share/lmao/example_hello_world.hell
	rm -f $(DESTDIR)/usr/share/lmao/example_simple_hello_world.hell
	rm -f $(DESTDIR)/usr/share/lmao/example_simple_cat.hell
	rmdir $(DESTDIR)/usr/share/lmao/

clean:
	make -C src/ clean
	rm -f ./lmao
	rm -rf doc/

