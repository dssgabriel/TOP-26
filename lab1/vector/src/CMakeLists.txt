add_library(vector INTERFACE)
target_sources(vector
  INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ../include
    FILES vector/vector.hpp
)

add_executable(main)
target_sources(main PRIVATE main.cpp)
target_compile_options(main PRIVATE -Wall -Wextra -Werror )
target_link_libraries(main PRIVATE vector fmt::fmt)
