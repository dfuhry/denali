include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR}
        )

add_executable(ctree ctree.cpp)

install(TARGETS ctree DESTINATION bin)
