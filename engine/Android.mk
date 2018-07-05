#Xash Engine Android port
#Copyright (c) nicknekit

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := xash

APP_PLATFORM := android-12

include $(XASH3D_CONFIG)

LOCAL_CFLAGS += -D__MULTITEXTURE_SUPPORT__ -DXASH_GLES -DXASH_NANOGL -DUSE_EVDEV -DXASH_DYNAMIC_DLADDR -DCRASHHANDLER -DXASH_OPENSL -DXASH_SKIPCRTLIB -DXASH_FORCEINLINE -DXASH_FASTSTR

XASH_COMMIT := $(firstword $(shell cd $(LOCAL_PATH)&&git rev-parse --short=6 HEAD) unknown)

LOCAL_CFLAGS += -DXASH_BUILD_COMMIT=\"$(XASH_COMMIT)\"

LOCAL_CONLYFLAGS += -std=c99

LOCAL_C_INCLUDES := \
	$(NANOGL_PATH)/GL			    \
	$(NANOGL_PATH)/				    \
	$(LOCAL_PATH)/.				    \
	$(LOCAL_PATH)/common			    \
	$(LOCAL_PATH)/client			    \
	$(LOCAL_PATH)/client/vgui		    \
	$(LOCAL_PATH)/server			    \
	$(LOCAL_PATH)/common/imagelib		    \
	$(LOCAL_PATH)/platform/android		    \
	$(LOCAL_PATH)/common/soundlib		    \
	$(LOCAL_PATH)/common/soundlib/libmpg        \
	$(LOCAL_PATH)/../common		            \
	$(LOCAL_PATH)/../pm_shared		    \
	$(LOCAL_PATH)/../			    \
	$(LOCAL_PATH)/../utils/vgui/include	    \
	$(HLSDK_PATH)/cl_dll/

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
	   platform/android/dlsym-weak.cpp \
	   client/cl_cmds.c \
           client/cl_demo.c \
           client/cl_events.c \
           client/cl_frame.c \
           client/cl_game.c \
           client/cl_main.c \
           client/cl_menu.c \
           client/cl_mobile.c \
           client/cl_parse.c \
           client/cl_pmove.c \
           client/cl_remap.c \
           client/cl_scrn.c \
           client/cl_tent.c \
           client/cl_video.c \
           client/cl_view.c \
           client/cl_netgraph.c \
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
           client/vid_common.c \
           client/gl_warp.c \
           client/joyinput.c \
           platform/android/snd_opensles.c \
           client/input.c \
           client/keys.c \
           client/input_evdevkey.c \
           client/console.c \
           client/touch.c \
           client/gamma.c \
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
           common/base_cmd.c \
           common/cfgscript.c \
           common/cmd.c \
           common/common.c \
           common/con_utils.c \
           common/crclib.c \
           common/crtlib.c \
           common/cvar.c \
           common/filesystem.c \
           common/host.c \
           common/hpak.c \
           common/infostring.c \
           common/identification.c \
           common/library.c \
           common/masterlist.c \
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
           common/system.c \
           common/titles.c \
           common/world.c \
           common/zone.c \
           common/crashhandler.c \
           server/sv_client.c \
           server/sv_cmds.c \
           server/sv_custom.c \
           server/sv_frame.c \
           server/sv_filter.c \
           server/sv_game.c \
           server/sv_init.c \
           server/sv_main.c \
           server/sv_log.c \
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
	   common/imagelib/img_dds.c \
           common/soundlib/snd_main.c \
           common/soundlib/snd_mp3.c \
           common/soundlib/snd_utils.c \
           common/soundlib/snd_wav.c \
	   common/soundlib/libmpg/dct36.c \
	   common/soundlib/libmpg/dct64.c \
	   common/soundlib/libmpg/format.c \
	   common/soundlib/libmpg/frame.c \
	   common/soundlib/libmpg/index.c \
	   common/soundlib/libmpg/layer3.c \
	   common/soundlib/libmpg/libmpg.c \
	   common/soundlib/libmpg/mpg123.c \
	   common/soundlib/libmpg/parse.c \
	   common/soundlib/libmpg/reader.c \
	   common/soundlib/libmpg/synth.c \
	   common/soundlib/libmpg/tabinit.c \
	   common/Sequence.c \
           platform/android/vid_android.c \
           platform/android/android_nosdl.c

LOCAL_STATIC_LIBRARIES := NanoGL

LOCAL_LDLIBS := -ldl -llog

include $(BUILD_SHARED_LIBRARY)
