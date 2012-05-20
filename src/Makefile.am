## Process this file with automake to produce Makefile.in

if BUILD_GUI
ui_PROGRAMS = WhySynth_gtk
else
ui_PROGRAMS =
endif

uidir = $(libdir)/dssi/whysynth

plugindir = $(libdir)/dssi

plugin_LTLIBRARIES = whysynth.la

WhySynth_gtk_SOURCES = \
	gui_main.c \
	gui_main.h \
	agran_tables.c \
	bitmap_about.xpm \
	bitmap_logo.xpm \
	common_data.c \
	common_data.h \
	dssp_event.h \
	gtkknob.c \
	gtkknob.h \
	gui_callbacks.c \
	gui_callbacks.h \
	gui_data.c \
	gui_images.c \
	gui_images.h \
	gui_interface.c \
	gui_interface.h \
	patch_tables.c \
	wave_tables.c \
	wave_tables.h \
	whysynth.h \
	whysynth_ports.c \
	whysynth_ports.h \
	whysynth_types.h \
	whysynth_voice.h

WhySynth_gtk_CFLAGS = -DY_GUI @GTK_CFLAGS@ $(AM_CFLAGS) $(GUI_CFLAGS)

if DARWIN
WhySynth_gtk_LDADD = -lm -lmx @GTK_LIBS@ $(AM_LDFLAGS) $(GUI_LIBS)
else
WhySynth_gtk_LDADD = -lm @GTK_LIBS@ $(AM_LDFLAGS) $(GUI_LIBS)
endif

whysynth_la_SOURCES = \
	dssp_synth.c \
	dssp_synth.h \
	agran_oscillator.c \
	agran_oscillator.h \
	agran_tables.c \
	common_data.c \
	common_data.h \
	dssp_event.c \
	dssp_event.h \
	effects.c \
	effects.h \
	effect_reverb.c \
	effect_reverb.h \
	effect_screverb.c \
	minblep_oscillator.h \
	minblep_tables.c \
	padsynth.c \
	padsynth.h \
	patch_tables.c \
	sampleset.c \
	sampleset.h \
	wave_data.c \
	wave_tables.c \
	wave_tables.h \
	whysynth.h \
	whysynth_data.c \
	whysynth_ports.c \
	whysynth_ports.h \
	whysynth_types.h \
	whysynth_voice.c \
	whysynth_voice.h \
	whysynth_voice_inline.h \
	whysynth_voice_render.c

if DARWIN
whysynth_la_CFLAGS = -DY_PLUGIN $(AM_CFLAGS) $(PLUGIN_CFLAGS) -DY_BOGUS_MLOCKALL
whysynth_la_LIBADD = -lm -lmx $(PLUGIN_LIBS)
else
whysynth_la_CFLAGS = -DY_PLUGIN $(AM_CFLAGS) $(PLUGIN_CFLAGS)
whysynth_la_LIBADD = -lm $(PLUGIN_LIBS)
endif

whysynth_la_LDFLAGS = -module -avoid-version
