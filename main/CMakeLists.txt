idf_component_register(
	SRCS
		src/main.cpp
		src/services/inkplate_static.cpp
		src/mqtt/topics/mqtt_topic_handler.cpp
		src/mqtt/topics/draw_bitmap_1bit/handler.cpp
	INCLUDE_DIRS src
	REQUIRES inkplate json esp_mqtt_cxx esp-idf-cxx
)
