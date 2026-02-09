
# Add executable
add_executable(TST-JSON-ENCODING test_json_encoding.c)
add_executable(TST-JSON-DECODING test_json_decoding.c)
add_executable(TST-JSON-ENCODING-EDGE test_json_encoding_edge.c)
add_executable(TST-JSON-DECODING-EDGE test_json_decoding_edge.c)


# Link executable
target_link_libraries(TST-JSON-ENCODING PRIVATE Radikant-Json)
target_link_libraries(TST-JSON-DECODING PRIVATE Radikant-Json)
target_link_libraries(TST-JSON-ENCODING-EDGE PRIVATE Radikant-Json)
target_link_libraries(TST-JSON-DECODING-EDGE PRIVATE Radikant-Json)