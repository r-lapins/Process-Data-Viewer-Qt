set(SOURCES
    src/main.cpp
    src/main_window.cpp
)

set(HEADERS
    include/pdv/main_window.h
)

qt_add_executable(process_data_viewer
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(process_data_viewer
    PRIVATE
        include
)

target_link_libraries(process_data_viewer
    PRIVATE
        Qt6::Widgets
        Qt6::Charts
        pdt
)