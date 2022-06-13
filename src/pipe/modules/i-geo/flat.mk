MOD_C=pipe/modules/i-geo/corona/prims.c core/compat.c
MOD_LDFLAGS=-lm

ifeq ($(MINGW_BUILD), 1)
MOD_C += ../ext/mman-win32/mman.c
MOD_H += ../ext/mman-win32/mman.h
MOD_CFLAGS += -I../ext/mman-win32/
EXTRA_DEPS=../ext/mman-win32/mman.h ../ext/mman-win32/mman.c
endif

pipe/modules/i-geo/libi-geo.so: pipe/modules/i-geo/corona/geo.h pipe/modules/i-geo/corona/prims.h pipe/modules/i-geo/corona/prims.c ${EXTRA_DEPS}
