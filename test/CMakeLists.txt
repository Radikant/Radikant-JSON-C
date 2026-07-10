
# Add executable
add_executable(TST-JSON-ENCODING test_encoding.c)
add_executable(TST-JSON-DECODING test_decoding.c)
add_executable(TST-JSON-PERF test_perf.c)
add_executable(TST-JSON-VECTORS test_vectors.c)

# Link executable
target_link_libraries(TST-JSON-ENCODING      PRIVATE Radikant-Json-C Radikant-Probe-C)
target_link_libraries(TST-JSON-DECODING      PRIVATE Radikant-Json-C Radikant-Probe-C)
target_link_libraries(TST-JSON-PERF          PRIVATE Radikant-Json-C Radikant-Probe-C yyjson)
target_link_libraries(TST-JSON-VECTORS       PRIVATE Radikant-Json-C Radikant-Probe-C)