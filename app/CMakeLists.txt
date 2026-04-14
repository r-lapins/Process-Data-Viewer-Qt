set(SOURCES
    # ===== CORE =====
    src/core/main.cpp
    src/core/main_window.cpp
    src/core/analysis_tab.cpp
    src/core/file_loader_service.cpp

    # ===== CSV =====
    src/csv/csv_analysis_controller.cpp
    src/csv/csv_analysis_controls_widget.cpp
    src/csv/csv_analysis_engine.cpp
    src/csv/csv_analysis_tab.cpp
    src/csv/csv_analysis_plot_widget.cpp
    src/csv/csv_analysis_results_panel.cpp
    src/csv/csv_samples_table_model.cpp

    # ===== WAV =====
    src/wav/wav_analysis_controller.cpp
    src/wav/wav_analysis_controls_widget.cpp
    src/wav/wav_analysis_plot_widget.cpp
    src/wav/wav_analysis_results_panel.cpp
    src/wav/wav_analysis_tab.cpp
)

set(HEADERS
    # ===== CORE =====
    include/pdv/core/session_data.h
    include/pdv/core/main_window.h
    include/pdv/core/analysis_tab.h
    include/pdv/core/file_loader_service.h

    # ===== CSV =====
    include/pdv/csv/csv_analysis_controller.h
    include/pdv/csv/csv_analysis_controls_widget.h
    include/pdv/csv/csv_analysis_engine.h
    include/pdv/csv/csv_analysis_tab.h
    include/pdv/csv/csv_analysis_plot_widget.h
    include/pdv/csv/csv_analysis_results_panel.h
    include/pdv/csv/csv_samples_table_model.h

    # ===== WAV =====
    include/pdv/wav/wav_analysis_controller.h
    include/pdv/wav/wav_analysis_controls_widget.h
    include/pdv/wav/wav_analysis_plot_widget.h
    include/pdv/wav/wav_analysis_results_panel.h
    include/pdv/wav/wav_analysis_tab.h
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
        pdt_csv
        pdt_dsp
        pdt_io_wav
)
