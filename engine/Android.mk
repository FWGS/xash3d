#Xash Engine Android port
#Copyright (c) nicknekit

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := xash

SDL_PATH := /home/nicknekit/SDL2 #your sdl2 source path
SDLIMAGE_PATH := /home/nicknekit/SDL2_image #your sdl2_image source path
SDLMIXER_PATH := /home/nicknekit/SDL2_mixer #your sdl2_mixer source path
NANOGL_PATH := /../../nanogl #your nanogl source path

APP_PLATFORM := android-12
LOCAL_CFLAGS += -O2
LOCAL_CONLYFLAGS += -std=c99

LOCAL_C_INCLUDES := $(SDL_PATH)/include \
	$(SDLIMAGE_PATH) \
	$(SDLMIXER_PATH) \
	$(NANOGL_PATH)/GL \
	$(LOCAL_PATH)/.			    \
	$(LOCAL_PATH)/common			    \
	$(LOCAL_PATH)/client			    \
	$(LOCAL_PATH)/client/vgui		    \
	$(LOCAL_PATH)/server			    \
	$(LOCAL_PATH)/common/imagelib		    \
	$(LOCAL_PATH)/common/sdl		    \
	$(LOCAL_PATH)/common/soundlib		    \
	$(LOCAL_PATH)/common/soundlib/libmpg   \
	$(LOCAL_PATH)/../common		    \
	$(LOCAL_PATH)/../pm_shared		    \
	$(LOCAL_PATH)/../			    \
	$(LOCAL_PATH)/../utils/vgui/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	client/cl_cmds.c \
           client/cl_demo.c \
           client/cl_events.c \
           client/cl_frame.c \
           client/cl_game.c \
           client/cl_main.c \
           client/cl_menu.c \
           client/cl_parse.c \
           client/cl_pmove.c \
           client/cl_remap.c \
           client/cl_scrn.c \
           client/cl_tent.c \
           client/cl_video.c \
           client/cl_view.c \
           client/gl_backend.c \
           client/gl_beams.c \
           client/gl_cull.c \
           client/gl_decals.c \
           client/gl_draw.c \
           client/gl_image.c \
           client/gl_mirror.c \
           client/gl_refrag.c \
           client/gl_rlight.c \
           client/gl_rmain.c \
           client/gl_rmath.c \
           client/gl_rmisc.c \
           client/gl_rpart.c \
           client/gl_rsurf.c \
           client/gl_sprite.c \
           client/gl_studio.c \
           client/gl_vidnt.c \
           client/gl_warp.c \
           client/s_backend.c \
           client/s_dsp.c \
           client/s_load.c \
           client/s_main.c \
           client/s_mix.c \
           client/s_mouth.c \
           client/s_stream.c \
           client/s_utils.c \
           client/s_vox.c \
           common/avikit.c \
           common/build.c \
           common/cmd.c \
           common/common.c \
           common/con_utils.c \
           common/console.c \
           common/crclib.c \
           common/crtlib.c \
           common/cvar.c \
           common/filesystem.c \
           common/gamma.c \
           common/host.c \
           common/hpak.c \
           common/infostring.c \
           common/input.c \
           common/keys.c \
           common/library.c \
           common/mathlib.c \
           common/matrixlib.c \
           common/mod_studio.c \
           common/model.c \
           common/net_buffer.c \
           common/net_chan.c \
           common/net_encode.c \
           common/net_huff.c \
           common/network.c \
           common/pm_surface.c \
           common/pm_trace.c \
           common/random.c \
           common/sys_con.c \
           common/sys_win.c \
           common/titles.c \
           common/world.c \
           common/zone.c \
           server/sv_client.c \
           server/sv_cmds.c \
           server/sv_custom.c \
           server/sv_frame.c \
           server/sv_game.c \
           server/sv_init.c \
           server/sv_main.c \
           server/sv_move.c \
           server/sv_phys.c \
           server/sv_pmove.c \
           server/sv_save.c \
           server/sv_world.c \
           client/vgui/vgui_draw.c \
           common/imagelib/img_bmp.c \
           common/imagelib/img_main.c \
           common/imagelib/img_quant.c \
           common/imagelib/img_tga.c \
           common/imagelib/img_utils.c \
           common/imagelib/img_wad.c \
           common/soundlib/snd_main.c \
           common/soundlib/snd_mp3.c \
           common/soundlib/snd_utils.c \
           common/soundlib/snd_wav.c \
	   android.cpp \
	   common/sdl/events.c \
	   common/soundlib/libmpg/dct64_i386.c \
	   common/soundlib/libmpg/decode_i386.c \
	   common/soundlib/libmpg/interface.c \
	   common/soundlib/libmpg/layer2.c \
	   common/soundlib/libmpg/layer3.c \
	   common/soundlib/libmpg/tabinit.c \
	   common/soundlib/libmpg/common.c

LOCAL_SHARED_LIBRARIES += SDL2
LOCAL_SHARED_LIBRARIES += SDL2_image
LOCAL_SHARED_LIBRARIES += SDL2_mixer

LOCAL_LDLIBS := -ldl -llog 

include $(BUILD_SHARED_LIBRARY)
