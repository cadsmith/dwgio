if (BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
  include(FetchContent)
  add_subdirectory(doctest)
endif()
