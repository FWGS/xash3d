#Xash3d mainui port for android
#Copyright (c) nicknekit

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libmenu_hardfp
endif



LOCAL_MODULE := menu

APP_PLATFORM := android-12

LOCAL_C_INCLUDES := $(SDL_PATH)/include \
		    $(LOCAL_PATH)/.			    \
		    $(LOCAL_PATH)/../common \
	            $(LOCAL_PATH)/../pm_shared \
	            $(LOCAL_PATH)/../engine \
		    $(LOCAL_PATH)/../engine/common \
		    $(LOCAL_PATH)/../utils/vgui/include \
		    $(HLSDK_PATH)/cl_dll/

LOCAL_SRC_FILES := basemenu.cpp \
           menu_advcontrols.cpp \
           menu_audio.cpp \
           menu_btns.cpp \
           menu_configuration.cpp \
           menu_controls.cpp \
           menu_creategame.cpp \
           menu_credits.cpp \
           menu_customgame.cpp \
           menu_gameoptions.cpp \
           menu_internetgames.cpp \
           menu_langame.cpp \
           menu_loadgame.cpp \
           menu_main.cpp \
           menu_multiplayer.cpp \
           menu_newgame.cpp \
           menu_playersetup.cpp \
           menu_savegame.cpp \
           menu_saveload.cpp \
           menu_strings.cpp \
           menu_video.cpp \
           menu_vidmodes.cpp \
           menu_vidoptions.cpp \
           udll_int.cpp \
           ui_title_anim.cpp \
	   utils.cpp
ifeq ($(XASH_SDL),1)
LOCAL_SHARED_LIBRARIES += SDL2
endif
include $(BUILD_SHARED_LIBRARY)
