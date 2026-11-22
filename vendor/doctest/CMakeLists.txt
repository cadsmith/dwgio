set(DOCTEST_WITH_TESTS OFF CACHE BOOL "")
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "")
set(DOCTEST_NO_INSTALL ON CACHE BOOL "")

FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG        2d0a9359a60c51affe2a9bebb1be1dca47868151
  SYSTEM
  FIND_PACKAGE_ARGS 2.5.3
)

FetchContent_MakeAvailable(doctest)
