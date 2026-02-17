find_package(PkgConfig REQUIRED)
pkg_check_modules(GMIME REQUIRED gmime-3.0)

include_directories(${GMIME_INCLUDE_DIRS})
link_directories(${GMIME_LIBRARY_DIRS})
