
# Add executable
add_executable(EXP-JSON-ENCODING encoding.c)
add_executable(EXP-JSON-DECODING decoding.c)

# Link executable
target_link_libraries(EXP-JSON-ENCODING PRIVATE Radikant-Json-C)
target_link_libraries(EXP-JSON-DECODING PRIVATE Radikant-Json-C)
