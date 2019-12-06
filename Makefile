CC=gcc
LDFLAGS=`pkg-config --libs gtk+-3.0` -li2c
CFLAGS=`pkg-config --cflags gtk+-3.0`

all: emac_monitor_tool.c
	${CC} ${CFLAGS} emac_monitor_tool.c -o emt ${LDFLAGS}

clean:
	rm emt

install: emt
	install -m 755 emt /usr/sbin

