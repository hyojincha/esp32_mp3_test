set(COMPONENT_SRCDIRS
	./
	./codec
	./mp3_decoder
	./mp3player
	./i2c
  )

set(COMPONENT_ADD_INCLUDEDIRS
  ./codec
  ./mp3_decoder
  ./mp3player
  ./i2c
)

#set(COMPONENT_REQUIRES driver)
set(COMPONENT_PRIV_REQUIRES freertos nvs_flash mdns sdmmc fatfs)

register_component()
