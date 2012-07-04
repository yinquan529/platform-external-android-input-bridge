LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:=    \
    common.c         \
    aibd.c
LOCAL_MODULE := aibd
LOCAL_CFLAGS += -fno-strict-aliasing -DAPPNAME=\"aidb\"
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:=    \
    common.c         \
    aib.c
LOCAL_MODULE := aib
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
LOCAL_CFLAGS += -fno-strict-aliasing -DAPPNAME=\"aib\"
include $(BUILD_HOST_EXECUTABLE)
