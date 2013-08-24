EMU = ../sources/src
LIBRETRO = ../sources/libretro
WALLEGRO = ../sources/allegrowrapper

CORE_SRCS = $(EMU)/audio.o $(EMU)/cpu.o $(EMU)/cset.o $(EMU)/keyboard.o $(EMU)/main.o $(EMU)/table.o $(EMU)/vdc.o \
$(EMU)/vmachine.o $(EMU)/debug.o $(EMU)/timefunc.o $(EMU)/voice.o $(EMU)/crc32.o $(EMU)/vpp_cset.o $(EMU)/vpp.o $(EMU)/score.o 

BUILD_APP =  $(ZLIB_OBJECTS)  $(CORE_SRCS)

HINCLUDES := -I./$(EMU) -I$(LIBRETRO) -I$(WALLEGRO)

OBJECTS := $(BUILD_APP) $(WALLEGRO)/wrapalleg.o $(LIBRETRO)/libretro-o2em.o $(LIBRETRO)/o2em-mapper.o $(LIBRETRO)/vkbd.o \
	$(LIBRETRO)/graph.o $(LIBRETRO)/fontmsx.o \
	


