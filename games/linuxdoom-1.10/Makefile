################################################################
#
# $Id:$
#
# $Log:$
#
CC=  gcc  # gcc or g++

CFLAGS=-g -Wall -DNORMALUNIX -DLINUX -DNO_SOUND_RUNTIME -DTINY_E1M1_ONLY # -DUSEASM 
LDFLAGS=-L/usr/X11R6/lib
LIBS=-lXext -lX11 -lm

TINYWAD_SOURCE?=$(if $(wildcard linux/doom1.wad.bak),linux/doom1.wad.bak,linux/doom1.wad)
TINYWAD_OUTPUT?=linux/doom1.wad
TINYWAD_FLAGS?=--aggressive-ui-strip --strip-sprites-e1m1 --strip-menu-assets --strip-midi-assets --strip-endscreen --minimal-things --canonical-wall-texture DOOR3
TINYWAD_VARIANT_DIR?=wad_variants
TINY_STRIP_RUNTIME?=1

ifeq ($(TINY_STRIP_RUNTIME),1)
CFLAGS += -DTINY_NO_NET -DTINY_NO_SOUND -DTINY_NO_MENU
SOUND_OBJ=$(O)/i_sound_stub.o
NET_OBJ=$(O)/i_net_stub.o
else
SOUND_OBJ=$(O)/i_sound.o
NET_OBJ=$(O)/i_net.o
endif

# subdirectory for objects
O=linux

# not too sophisticated dependency
OBJS=				\
		$(O)/doomdef.o		\
		$(O)/doomstat.o		\
		$(O)/dstrings.o		\
		$(O)/i_system.o		\
		$(SOUND_OBJ)		\
		$(O)/i_video.o		\
		$(NET_OBJ)			\
		$(O)/tables.o			\
		$(O)/f_finale.o		\
		$(O)/f_wipe.o 		\
		$(O)/d_main.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o		\
		$(O)/g_game.o			\
		$(O)/m_menu.o			\
		$(O)/m_misc.o			\
		$(O)/m_argv.o  		\
		$(O)/m_bbox.o			\
		$(O)/m_fixed.o		\
		$(O)/m_swap.o			\
		$(O)/m_cheat.o		\
		$(O)/m_random.o		\
		$(O)/am_map.o			\
		$(O)/p_ceilng.o		\
		$(O)/p_doors.o		\
		$(O)/p_enemy.o		\
		$(O)/p_floor.o		\
		$(O)/p_inter.o		\
		$(O)/p_lights.o		\
		$(O)/p_map.o			\
		$(O)/p_maputl.o		\
		$(O)/p_plats.o		\
		$(O)/p_pspr.o			\
		$(O)/p_setup.o		\
		$(O)/p_sight.o		\
		$(O)/p_spec.o			\
		$(O)/p_switch.o		\
		$(O)/p_mobj.o			\
		$(O)/p_telept.o		\
		$(O)/p_tick.o			\
		$(O)/p_saveg.o		\
		$(O)/p_user.o			\
		$(O)/r_bsp.o			\
		$(O)/r_data.o			\
		$(O)/r_draw.o			\
		$(O)/r_main.o			\
		$(O)/r_plane.o		\
		$(O)/r_segs.o			\
		$(O)/r_sky.o			\
		$(O)/r_things.o		\
		$(O)/w_wad.o			\
		$(O)/wi_stuff.o		\
		$(O)/v_video.o		\
		$(O)/st_lib.o			\
		$(O)/st_stuff.o		\
		$(O)/hu_stuff.o		\
		$(O)/hu_lib.o			\
		$(O)/s_sound.o		\
		$(O)/z_zone.o			\
		$(O)/info.o				\
		$(O)/sounds.o

all:	 $(O)/linuxxdoom

tinywad:
	./tools/minify_wad.py $(TINYWAD_SOURCE) $(TINYWAD_OUTPUT) $(TINYWAD_FLAGS)

tinywad-tests:
	mkdir -p $(TINYWAD_VARIANT_DIR)
	./tools/minify_wad.py $(TINYWAD_SOURCE) $(TINYWAD_VARIANT_DIR)/doom1.test_a.wad --aggressive-ui-strip --strip-sprites-e1m1 --strip-menu-assets --strip-midi-assets --strip-endscreen
	./tools/minify_wad.py $(TINYWAD_SOURCE) $(TINYWAD_VARIANT_DIR)/doom1.test_b.wad --aggressive-ui-strip --strip-menu-assets --strip-midi-assets --strip-endscreen --canonical-wall-texture DOOR3
	./tools/minify_wad.py $(TINYWAD_SOURCE) $(TINYWAD_VARIANT_DIR)/doom1.test_c.wad --aggressive-ui-strip --strip-sprites-e1m1 --strip-menu-assets --strip-midi-assets --strip-endscreen --canonical-wall-texture DOOR3
	./tools/minify_wad.py $(TINYWAD_SOURCE) $(TINYWAD_VARIANT_DIR)/doom1.test_d.wad --aggressive-ui-strip --strip-menu-assets --strip-midi-assets --strip-endscreen --canonical-wall-texture DOOR3 --canonical-floor-flat FLOOR7_2 --canonical-ceil-flat F_SKY1
	./tools/minify_wad.py $(TINYWAD_SOURCE) $(TINYWAD_VARIANT_DIR)/doom1.test_e.wad --aggressive-ui-strip --strip-sprites-e1m1 --strip-menu-assets --strip-midi-assets --strip-endscreen --minimal-things --min-sector-light 192
	./tools/minify_wad.py $(TINYWAD_SOURCE) $(TINYWAD_VARIANT_DIR)/doom1.test_f.wad --aggressive-ui-strip --strip-sprites-e1m1 --strip-menu-assets --strip-midi-assets --strip-endscreen --minimal-things --canonical-wall-texture DOOR3

$(O):
	mkdir -p $(O)

clean:
	rm -f *.o *~ *.flc
	rm -f linux/*

$(O)/linuxxdoom:	$(O) $(OBJS) $(O)/i_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/i_main.o \
	-o $(O)/linuxxdoom $(LIBS)

$(O)/%.o:	%.c | $(O)
	$(CC) $(CFLAGS) -c $< -o $@

#############################################################
#
#############################################################