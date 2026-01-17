add_library(lib_onewire INTERFACE)

target_sources(lib_onewire INTERFACE
    onewire.c
    )

target_include_directories(lib_onewire INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    )

pico_generate_pio_header(lib_onewire 
    ${CMAKE_CURRENT_LIST_DIR}/onewire_driver.pio
    )

target_link_libraries(lib_onewire INTERFACE 
    pico_stdlib
    hardware_pio
    hardware_clocks
    )