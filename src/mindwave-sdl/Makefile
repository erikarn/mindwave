
PROG=mindwave-sdl
SRCS=mindwave-sdl.c
NO_MAN=1
LDADD=-lmindwave -lSDL -lGL -lGLU -lfftw3 -lm
CFLAGS+=	-I${.CURDIR}/../../lib/libmindwave -I/usr/local/include/
LDFLAGS+=	-L${.CURDIR}/../../lib/libmindwave -L/usr/local/lib/

.include <bsd.prog.mk>

