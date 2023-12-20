# VSB-EInk - Firmware

This repository contains the firmware for the VSB-EInk project. The firmware is written in C++ and is based on the [esp-idf]() and the [ESP-IDF-Inkplate]() library.

## Getting started

### Prerequisites

* git
* esp-idf v5.0.*

### Project setup

1. Clone the repository and its submodules
    ```bash
    git clone --recurse-submodules git@github.com:vsb-eink/vsb-eink-panel.git
   ```
2. Load the esp-idf environment
    ```bash
    # if already set up get_idf alias
    get_idf
   
    # POSIX shell
    . "$IDF_PATH/export.sh"
   
    # Fish shell
    . "$IDF_PATH/export.fish"
    ```

## Signed images

The project has an automated firmware image build pipeline which builds the firmware and signs it with our own private key. Each time a project version is bumped in `version.txt`, pipeline build/signs and releases its build artifacts as a new GitHub release. These releases can then be used to either flash a new panel or update an existing panel over the air.

### Flashing a new panel

1. Download the latest release from the [releases page](https://github.com/vsb-eink/vsb-eink-panel/releases/latest). Specifically, you need the following files:
    * bootloader.bin
    * partition-table.bin
    * ota_data_initial.bin
    * vsb-eink-panel.bin
2. Flash the images using the following command (taken from the official idf.py cli)
   ```bash
   esptool.py -p (PORT) -b 460800 --before default_reset --after hard_reset --chip esp32  write_flash --flash_mode dio --flash_size 4MB --flash_freq 40m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0xd000 build/ota_data_initial.bin 0x10000 build/vsb-eink-panel.bin
   ```

## Panel provisioning

Firmware builds are deployment agnostic. Each panel has the same firmware image and differs only in its NVS partition.

To simplify the process of provisioning, the project contains a [python script](./scripts/provision_panel.py) which takes a CSV file as input and either generates an NVS partition image or flashes the data directly to the panel.

### Example CSV file
```csv
key,type,encoding,value
eeprom,namespace,,
eeprom,data,base64,VxYAAwMDAwMDAwAAAQIBAQICAQAAAgICAQICAQAAAAICAgICAQAAAwMCAQEBAgAAAwMCAgEBAgAAAgECAQIBAgAAAwMDAgICAgAU7g==
vsb_eink,namespace,,
panel_id,data,string,"ec4"
wifi_ssid_a,data,string,"tuonet-iot"
wifi_pass_a,data,string,"password"
wifi_ssid_b,data,string,"Home-Wifi"
wifi_pass_b,data,string,"password"
broker_url_a,data,string,"mqtt://192.168.1.164:1883"
broker_url_b,data,string,"mqtt://192.168.1.164:1883"
```

As you can see, each option has an A and B variant. The panel will try to use to the A variant first and if that fails, it will try the B variant.

### Provisioning a panel with a CSV file
```bash
python scripts/provision_panel.py --flash path/to/config.csv
```

## Panel status

Once the panel is correctly configured and booted up, it will periodically publish an MQTT message on the topic `vsb-eink/:panel_id/system`. You can either subcribe to this topic manually (`vsb-eink/+/status`) or use an MQTT client like [MQTT Explorer](https://mqtt-explorer.com/) to monitor all the project's messages.