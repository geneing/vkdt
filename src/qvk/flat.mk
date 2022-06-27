QVK_O=qvk/qvk.o\
      qvk/qvk_util.o
QVK_H=qvk/qvk.h\
      qvk/qvk_util.h
QVK_CFLAGS=

ifneq ($(MINGW_BUILD), 1)
QVK_LDFLAGS=$(shell pkg-config --libs vulkan)
else
QVK_LDFLAGS=-LC:/VulkanSDK/1.3.216.0/Lib -l:vulkan-1.lib
endif