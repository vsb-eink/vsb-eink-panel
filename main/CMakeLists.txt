idf_component_register(
	SRCS
		src/main.cpp
		src/services/inkplate_static.cpp
	INCLUDE_DIRS src
	REQUIRES inkplate json esp_mqtt_cxx esp-idf-cxx
)
