LOCAL_PATH := $(call my-dir)/../../..
PVRSDKDIR := $(realpath $(call my-dir)/../../../../../../..)

ASSETDIR := $(PVRSDKDIR)/Examples/Beginner/04_IntroducingPVRApi/Vulkan/Build/Android/assets


ifneq "$(MAKECMDGOALS)" "clean"
# Prebuilt module PVRUIRenderer
include $(CLEAR_VARS)
LOCAL_MODULE := PVRUIRenderer
LOCAL_SRC_FILES := $(PVRSDKDIR)/Framework/Bin/Android/local/$(TARGET_ARCH_ABI)/libPVRUIRenderer.a
include $(PREBUILT_STATIC_LIBRARY)
endif

ifneq "$(MAKECMDGOALS)" "clean"
# Prebuilt module PVRVulkan
include $(CLEAR_VARS)
LOCAL_MODULE := PVRVulkan
LOCAL_SRC_FILES := $(PVRSDKDIR)/Framework/Bin/Android/local/$(TARGET_ARCH_ABI)/libPVRVulkan.a
include $(PREBUILT_STATIC_LIBRARY)
endif

ifneq "$(MAKECMDGOALS)" "clean"
# Prebuilt module PVRNativeVulkan
include $(CLEAR_VARS)
LOCAL_MODULE := PVRNativeVulkan
LOCAL_SRC_FILES := $(PVRSDKDIR)/Framework/Bin/Android/local/$(TARGET_ARCH_ABI)/libPVRNativeVulkan.a
include $(PREBUILT_STATIC_LIBRARY)
endif

ifneq "$(MAKECMDGOALS)" "clean"
# Prebuilt module PVRVulkanGlue
include $(CLEAR_VARS)
LOCAL_MODULE := PVRVulkanGlue
LOCAL_SRC_FILES := $(PVRSDKDIR)/Framework/Bin/Android/local/$(TARGET_ARCH_ABI)/libPVRVulkanGlue.a
include $(PREBUILT_STATIC_LIBRARY)
endif

ifneq "$(MAKECMDGOALS)" "clean"
# Prebuilt module PVRAssets
include $(CLEAR_VARS)
LOCAL_MODULE := PVRAssets
LOCAL_SRC_FILES := $(PVRSDKDIR)/Framework/Bin/Android/local/$(TARGET_ARCH_ABI)/libPVRAssets.a
include $(PREBUILT_STATIC_LIBRARY)
endif

ifneq "$(MAKECMDGOALS)" "clean"
# Prebuilt module PVRShell
include $(CLEAR_VARS)
LOCAL_MODULE := PVRShell
LOCAL_SRC_FILES := $(PVRSDKDIR)/Framework/Bin/Android/local/$(TARGET_ARCH_ABI)/libPVRShell.a
include $(PREBUILT_STATIC_LIBRARY)
endif

ifneq "$(MAKECMDGOALS)" "clean"
# Prebuilt module PVRCore
include $(CLEAR_VARS)
LOCAL_MODULE := PVRCore
LOCAL_SRC_FILES := $(PVRSDKDIR)/Framework/Bin/Android/local/$(TARGET_ARCH_ABI)/libPVRCore.a
include $(PREBUILT_STATIC_LIBRARY)
endif


# Module VulkanIntroducingPVRApi
include $(CLEAR_VARS)

LOCAL_MODULE    := VulkanIntroducingPVRApi

### Add all source file names to be included in lib separated by a whitespace
LOCAL_SRC_FILES  := VulkanIntroducingPVRApi.cpp

LOCAL_C_INCLUDES := $(PVRSDKDIR)/Framework \
                    $(PVRSDKDIR)/Builds/Include



LOCAL_LDLIBS := -landroid \
                -llog

LOCAL_STATIC_LIBRARIES := PVRUIRenderer PVRVulkan PVRNativeVulkan PVRVulkanGlue PVRAssets PVRCore android_native_app_glue


LOCAL_CFLAGS += $(SDK_BUILD_FLAGS)

LOCAL_WHOLE_STATIC_LIBRARIES := PVRShell

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
