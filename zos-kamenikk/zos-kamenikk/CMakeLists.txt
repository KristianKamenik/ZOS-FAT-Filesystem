add_executable (zos-kamenikk
  CommandHandle.cpp
  Filesystem.cpp
  FilesystemFAT.cpp
  System.cpp
  zos-kamenikk.cpp
)

target_include_directories(zos-kamenikk PRIVATE
 ./
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET vymazat-temp-projekt PROPERTY CXX_STANDARD 20)
endif()