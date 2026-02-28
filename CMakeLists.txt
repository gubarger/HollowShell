cmake_minimum_required(VERSION 3.20)
project(HollowShell VERSION 1.0.0 LANGUAGES CXX)
add_subdirectory("HollowShell")
target_compile_features(hollow_shell PRIVATE cxx_std_20)
target_compile_options(hollow_shell PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)