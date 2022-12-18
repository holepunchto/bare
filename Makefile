pear: pear.c
	cc pear.c -I../libjs/include -I../libjs/vendor/libuv/include -L/opt/homebrew/lib ../libjs/build/libjs.a -luv -lv8 -lc++ -o pear
