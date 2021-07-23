cmake_minimum_required(VERSION 3.10)

project(mpp   VERSION 0.1
              DESCRIPTION "Simple macro expansion tool"
              LANGUAGES CXX)

include_directories(source)
#file(GLOB SOURCES "source/*.cpp")
add_executable(mpp source/main.cpp)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /O2 /utf-8 /J)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic-errors -Werror -O3 -fno-rtti -funsigned-char)
endif()

target_compile_features(mpp PUBLIC cxx_std_17)

set_target_properties(mpp PROPERTIES
    #CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    #CXX_RTTI OFF
    INTERPROCEDURAL_OPTIMIZATION ON
)
#set_property(TARGET mpp PROPERTY INTERPROCEDURAL_OPTIMIZATION True)


#install(TARGETS mpp DESTINATION /usr/lib)
