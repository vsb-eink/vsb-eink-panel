idf_component_register(
	SRCS
		src/main.cpp
		src/config.cpp
		src/eink_mqtt.cpp
		src/utils.cpp
		src/drivers/inkplate_button.cpp
		src/drivers/inkplate_static.cpp
		src/drivers/inkplate_touchpad.cpp
		src/drivers/inkplate_waveform.cpp
		src/tasks/panel/panel_task.cpp
		src/tasks/system/system_task.cpp
	INCLUDE_DIRS src
	REQUIRES inkplate json esp_mqtt_cxx esp-idf-cxx esp_https_ota
)
