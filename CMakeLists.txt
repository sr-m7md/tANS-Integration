cmake_minimum_required(VERSION 3.16)
project(Work1)

set(CMAKE_C_STANDARD 99)

set(CONTIKI_NG_PATH "/home/andezion/contiki-ng")

if(NOT EXISTS ${CONTIKI_NG_PATH})
    message(WARNING "Contiki-NG not found at ${CONTIKI_NG_PATH}. Please update CONTIKI_NG_PATH in CMakeLists.txt")
endif()

include_directories(
        ${CONTIKI_NG_PATH}
        ${CONTIKI_NG_PATH}/os
        ${CONTIKI_NG_PATH}/os/sys
        ${CONTIKI_NG_PATH}/os/lib
        ${CONTIKI_NG_PATH}/os/dev
        ${CONTIKI_NG_PATH}/os/net
        ${CONTIKI_NG_PATH}/os/net/ipv6
        ${CONTIKI_NG_PATH}/os/net/routing
        ${CONTIKI_NG_PATH}/arch/platform/native
        ${CONTIKI_NG_PATH}/arch/cpu/native
)

add_definitions(
        -DCONTIKI=1
        -DCONTIKI_NG=1
        -DNETSTACK_CONF_WITH_IPV6=1
        -DUIP_CONF_IPV6=1
        -DWITH_COAP=1
)

add_executable(sender
        sender/sender.c
        sender/tans_embedded.c
        sender/tans_embedded.h
        sender/project-conf.h
        sender/Makefile
        sender/native-config.h
)

add_executable(receiver
        receiver/receiver.c
        receiver/tans_embedded.c
        receiver/tans_embedded.h
        receiver/project-conf.h
        receiver/Makefile
        receiver/native-config.h
        tans_native/tans_native.c
        tans_native/project-conf.h
)

target_include_directories(sender PRIVATE
        sender/
        ${CONTIKI_NG_PATH}
        ${CONTIKI_NG_PATH}/os
        ${CONTIKI_NG_PATH}/os/sys
        ${CONTIKI_NG_PATH}/os/net
)

target_include_directories(receiver PRIVATE
        receiver/
        ${CONTIKI_NG_PATH}
        ${CONTIKI_NG_PATH}/os
        ${CONTIKI_NG_PATH}/os/sys
        ${CONTIKI_NG_PATH}/os/net
)
