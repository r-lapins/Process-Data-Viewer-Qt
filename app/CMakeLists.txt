set(SOURCES
    src/main.cpp
    src/main_window.cpp
    src/analysis_tab.cpp
    src/file_loader_service.cpp

    src/csv_samples_table_model.cpp
    src/csv_analysis_tab.cpp
    src/csv_analysis_engine.cpp
    src/csv_plot_widget.cpp

    src/wav_analysis_tab.cpp
    src/wav_analysis_engine.cpp
    src/wav_plot_widget.cpp
)

set(HEADERS
    include/pdv/session_data.h
    include/pdv/main_window.h
    include/pdv/analysis_tab.h
    include/pdv/file_loader_service.h

    include/pdv/csv_samples_table_model.h
    include/pdv/csv_analysis_tab.h
    include/pdv/csv_analysis_engine.h
    include/pdv/csv_plot_widget.h

    include/pdv/wav_analysis_tab.h
    include/pdv/wav_analysis_engine.h
    include/pdv/wav_plot_widget.h
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
        Qt6::Concurrent
        pdt
)
