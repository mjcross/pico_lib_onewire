#include <stdio.h>
#include <malloc.h>
#include "pico/malloc.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "onewire.h"
#include "onewire_driver.pio.h"
#include "onewire_rom_commands.h"

// library instance (onewire_t is an opaque reference to this)
struct onewire_instance_t {
    PIO pio;
    uint sm;
    int offset;
    uint jmp_reset;
    uint gpio;
};

// whether the driver is installed on each PIO (and if so where)
static bool driver_is_installed[NUM_PIOS];
static int8_t driver_offset[NUM_PIOS];

// create and initialise a library instance
// returns: a new library instance, or NULL if the chosen PIO is out of state machines or program space
onewire_t onewire_init(uint pio_num, uint gpio) {
    onewire_t ow = NULL;
    if (pio_num < NUM_PIOS) {
        PIO pio = pio_get_instance(pio_num);
        uint sm = pio_claim_unused_sm(pio, false);
        if (sm >= 0) {
            if (!driver_is_installed[pio_num]) {
                driver_offset[pio_num] = pio_add_program(pio, &onewire_driver_program);
            }
            if (driver_offset[pio_num] >= 0) {
                driver_is_installed[pio_num] = true;
                ow = calloc(1, sizeof(struct onewire_instance_t));
                hard_assert(ow);    // out of memory
                ow->pio = pio;
                ow->sm = sm;
                ow->offset = driver_offset[pio_num];
                ow->jmp_reset = onewire_driver_reset_instr(ow->offset);
                ow->gpio = gpio;
                pio_gpio_init(pio, gpio);   // set gpio function to pio
                onewire_driver_init(pio, sm, driver_offset[pio_num], gpio, 8);  // start the driver in 8-bit mode
            }
        }
    }
    return ow;
}

// read 8 bits from the bus
// returns: the value read (lsb first)
uint onewire_read(onewire_t ow) {
    pio_sm_put_blocking(ow->pio, ow->sm, 0b11111111);               // generate 8 read slots
    return (uint8_t)(pio_sm_get_blocking (ow->pio, ow->sm) >> 24);  // read response and shift into bits 0..7
}

// write 8 bits to the bus (lsb first)
void onewire_send (onewire_t ow, uint8_t data) {
    pio_sm_put_blocking (ow->pio, ow->sm, (uint32_t)data);
    pio_sm_get_blocking (ow->pio, ow->sm);                          // discard the response
}

// assert a reset condition on the bus and see if any device(s) respond
bool onewire_reset (onewire_t ow) {
    pio_sm_exec_wait_blocking (ow->pio, ow->sm, ow->jmp_reset);
    if ((pio_sm_get_blocking (ow->pio, ow->sm) & 1) == 0) {         // listen for any device pulling the bus low
        return true;
    }
    return false;
}

// check the CRC of a onewire id
// returns: true if the CRC matched
bool onewire_check_crc(onewire_id_t id) {
    uint8_t crc = 0;
    const uint8_t id_crc = id.crc;
    for(int i = 0; i < 56; i+= 1) {
        if ((id.raw & 0x01) ^ (crc & 0x01)) {
            crc = (crc >> 1) ^ 0x8c;                                // CRC polynomial x^8 + x^5 + x^4 + 1
        } else {
            crc >>= 1;
        }
        id.raw >>= 1;
    }
    return (id_crc == crc);
}

// Find ROM codes (64-bit hardware addresses) of all connected devices, using my
// version of https://www.analog.com/en/app-notes/1wire-search-algorithm.html.
// Returns: the number of devices found (up to maxdevs) or -1 if an error occurrred.
// ow: pointer to an instance of the library
// device_id_list: location at which store the addresses (NULL means don't store)
// maxdevs: maximum number of devices to find (0 means no limit)
int onewire_bus_scan (onewire_t ow, onewire_id_t *device_id_list, int maxdevs, uint8_t search_command) {
    int index;
    onewire_id_t device_id = {.raw = 0ull};
    int branch_point;
    int next_branch_point = -1;
    int num_found = 0;
    bool finished = false;

    onewire_driver_init (ow->pio, ow->sm, ow->offset, ow->gpio, 1); // restart the driver in single bit mode

    while (finished == false && (maxdevs == 0 || num_found < maxdevs )) {
        finished = true;
        branch_point = next_branch_point;
        if (onewire_reset (ow) == false) {
            num_found = 0;                          // no devices present
            finished = true;
            break;
        }
        for (int i = 0; i < 8; i += 1) {            // send search command (as single bits)
            onewire_send (ow, search_command >> i);
        }
        for (index = 0; index < 64; index += 1) {   // determine device_id bits 0..63 (see ref)
            uint a = onewire_read (ow);             // read two consecutive address bits 'a' and 'b'
            uint b = onewire_read (ow);
            if (a == 0 && b == 0) {                 // handle the case where both bits are zero
                if (index == branch_point) {
                    onewire_send (ow, 1);
                    device_id.raw |= (1ull << index);
                } else {
                    if (index > branch_point || (device_id.raw & (1ull << index)) == 0) {
                        onewire_send(ow, 0);
                        finished = false;
                        device_id.raw &= ~(1ull << index);
                        next_branch_point = index;
                    } else {                        // index < branch_point or device_id[index] = 1
                        onewire_send (ow, 1);
                    }
                }
            } else if (a != 0 && b != 0) {          // handle the case where both bits are one (should not happen)
                num_found = -2;
                finished = true;
                break;                              // terminate 'for' loop
            } else {
                if (a == 0) {                       // handle the case where the two bits are different (0, 1) or (1, 0)
                    onewire_send (ow, 0);
                    device_id.raw &= ~(1ull << index);
                } else {
                    onewire_send (ow, 1);
                    device_id.raw |= (1ull << index);
                }
            }
        }                                           // end of 'for' loop

        if (device_id_list != NULL) {
            device_id_list[num_found] = device_id;  // add the device_id to the list
        }
        num_found += 1;
    }                                               // end of 'while' loop

    onewire_driver_init (ow->pio, ow->sm, ow->offset, ow->gpio, 8); // restart the driver in 8-bit mode
    return num_found;
}