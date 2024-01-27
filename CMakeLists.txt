# CMakeList.txt : CMake project for ElevenLabsTTS, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_TOOLCHAIN_FILE "C:/Users/16478/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")


project ("ElevenLabsTTS")

# Add source to this project's executable.
add_executable (ElevenLabsTTS "ElevenLabsTTS.cpp" "ElevenLabsTTS.h")

# Find packages
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)

# Link libraries to your executable
target_link_libraries(ElevenLabsTTS PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
target_link_libraries(ElevenLabsTTS PRIVATE CURL::libcurl)