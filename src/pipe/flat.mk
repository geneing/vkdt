PIPE_O=\
pipe/alloc.o\
pipe/connector.o\
pipe/global.o\
pipe/graph.o\
pipe/graph-io.o\
pipe/graph-export.o\
pipe/module.o\
pipe/raytrace.o
PIPE_H=\
pipe/alloc.h\
pipe/connector.h\
pipe/connector.inc\
pipe/dlist.h\
pipe/draw.h\
pipe/global.h\
pipe/graph.h\
pipe/graph-io.h\
pipe/graph-print.h\
pipe/graph-export.h\
pipe/graph-traverse.inc\
pipe/modules/api.h\
pipe/dt-io.h\
pipe/module.h\
pipe/node.h\
pipe/params.h\
pipe/pipe.h\
pipe/raytrace.h\
pipe/token.h
PIPE_CFLAGS =
PIPE_LDFLAGS = -ldl

ifeq ($(MINGW_BUILD), 1)
PIPE_O += ../ext/dlfcn-win32/src/dlfcn.o
PIPE_H += ../ext/dlfcn-win32/src/dlfcn.h
PIPE_CFLAGS += -I../ext/dlfcn-win32/src
PIPE_LDFLAGS = -lshlwapi
endif
