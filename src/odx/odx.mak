CPUDEFS += -DHAS_FAME=1 -D_GCW0_=1 -DHAS_CYCLONE=1 -DHAS_DRZ80=1
OBJDIRS += $(OBJ)/cpu/fame
CPUOBJS += $(OBJ)/cpu/fame/f68000.o $(OBJ)/cpu/fame/famec.o
OBJDIRS += $(OBJ)/cpu/m68000_cyclone $(OBJ)/cpu/z80_drz80
CPUOBJS += $(OBJ)/cpu/m68000_cyclone/cyclone.o $(OBJ)/cpu/m68000_cyclone/c68000.o
CPUOBJS += $(OBJ)/cpu/z80_drz80/drz80.o $(OBJ)/cpu/z80_drz80/drz80_z80.o

OSOBJS = $(OBJ)/odx/fastmem.o $(OBJ)/odx/minimal.o \
	$(OBJ)/odx/odx.o $(OBJ)/odx/video.o $(OBJ)/odx/blit.o \
	$(OBJ)/odx/sound.o $(OBJ)/odx/input.o $(OBJ)/odx/fileio.o \
	$(OBJ)/odx/config.o $(OBJ)/odx/fronthlp.o \
	$(OBJ)/odx/alsa_sound.o \
	$(OBJ)/odx/js_joystick.o \
	$(OBJ)/odx/x11_window.o \
	$(OBJ)/odx/col.o \
	$(OBJ)/odx/odx_frontend.o

