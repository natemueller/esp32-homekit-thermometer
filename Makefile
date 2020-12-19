 PROJECT_NAME = esp32_homekit_thermometer

CFLAGS += -I$(abspath ../../..) -DHOMEKIT_SHORT_APPLE_UUIDS

EXTRA_COMPONENT_DIRS += \
  $(abspath ../../../components)

include $(IDF_PATH)/make/project.mk
