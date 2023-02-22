PROJECT_NAME := bts_pendent
EXTRA_COMPONENT_DIRS += ../libs
EXTRA_COMPONENT_DIRS += ../codec
EXTRA_COMPONENT_DIRS += ../mp3player
EXTRA_COMPONENT_DIRS += ../mp3_decoder
EXTRA_INCLUDE_DIRS += ../libs/include
include $(IDF_PATH)/make/project.mk
