add_library(pico_lib_onewire INTERFACE)

target_sources(pico_lib_onewire INTERFACE
    onewire.c
    )

target_include_directories(pico_lib_onewire INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    )

pico_generate_pio_header(pico_lib_onewire 
    ${CMAKE_CURRENT_LIST_DIR}/onewire_driver.pio
    )

target_link_libraries(pico_lib_onewire INTERFACE 
    pico_stdlib
    hardware_pio
    hardware_clocks
    )