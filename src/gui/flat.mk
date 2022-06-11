GUI_O=gui/gui.o\
      gui/render.o\
      gui/darkroom.o\
      gui/main.o\
      gui/view.o\
      ../ext/imgui/imgui.o\
      ../ext/imgui/imgui_draw.o\
      ../ext/imgui/imgui_widgets.o\
      ../ext/imgui/imgui_tables.o\
      ../ext/imgui/backends/imgui_impl_vulkan.o\
      ../ext/imgui/backends/imgui_impl_glfw.o
GUI_H=gui/gui.h\
      gui/render.h\
      gui/api.h\
      gui/api.hh\
      gui/hotkey.hh\
      gui/widget_filebrowser.hh\
      gui/widget_thumbnail.hh\
      gui/view.h\
      gui/darkroom.h\
      gui/darkroom-util.h\
      gui/lighttable.h\
      pipe/graph-traverse.inc
GUI_CFLAGS=$(VKDT_GLFW_CFLAGS) -I../ext/imgui -I../ext/imgui/backends/

ifneq ($(origin MSYSTEM), environment)
GUI_LDFLAGS=-ldl $(VKDT_GLFW_LDFLAGS) -lm -lstdc++
else
GUI_LDFLAGS=
GUI_CFLAGS+= -DIMGUI_DISABLE_WIN32_FUNCTIONS
endif
GUI_LDFLAGS+=$(VKDT_GLFW_LDFLAGS) -lm -lstdc++

ifeq ($(VKDT_USE_FREETYPE),1)
GUI_CFLAGS+=$(shell pkg-config --cflags freetype2) -DVKDT_USE_FREETYPE=1
GUI_LDFLAGS+=$(shell pkg-config --libs freetype2)
endif
ifeq ($(VKDT_USE_PENTABLET),1)
GUI_CFLAGS+=-DVKDT_USE_PENTABLET=1
endif

gui/main.o:core/version.h core/signal.h
