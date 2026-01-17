#pragma once

#include "pico/types.h"
#include "onewire_rom_commands.h"

// Bit-packed fields of a 64bit onewire rom code
typedef union {
    struct {
        uint8_t family:8;   // LSB of raw value
        uint64_t serial:48;
        uint8_t crc:8;
    };
    uint8_t byte[8];
    uint64_t raw;
} onewire_id_t;

// Opaque reference to library ow
typedef struct onewire_instance_t *onewire_t;

// Public API
onewire_t onewire_init(uint pio_num, uint gpio);
void onewire_send(onewire_t ow, uint8_t data);
uint onewire_read(onewire_t ow);
bool onewire_reset(onewire_t ow);
bool onewire_check_crc(onewire_id_t id);
int onewire_bus_scan (onewire_t ow, onewire_id_t *id_list, int maxdevs, uint8_t search_command);