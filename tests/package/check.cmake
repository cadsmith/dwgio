set(prefix ${BUILD}/package/prefix)
set(consumer ${BUILD}/package/consumer)
file(REMOVE_RECURSE ${BUILD}/package)
execute_process(
  COMMAND ${CMAKE_COMMAND} --install ${BUILD} --prefix ${prefix}
    --config ${CONFIG}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${CMAKE_COMMAND} -S ${SOURCE} -B ${consumer} -G ${GENERATOR}
    -DCMAKE_CXX_COMPILER=${COMPILER}
    -DCMAKE_PREFIX_PATH=${prefix}
    -DCMAKE_BUILD_TYPE=${CONFIG}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${CMAKE_COMMAND} --build ${consumer} --config ${CONFIG}
  COMMAND_ERROR_IS_FATAL ANY
)
find_program(program dwgio_package
  PATHS ${consumer} ${consumer}/${CONFIG}
  NO_DEFAULT_PATH REQUIRED
)
execute_process(
  COMMAND ${program} ${FIXTURE}
  OUTPUT_VARIABLE printed
  COMMAND_ERROR_IS_FATAL ANY
)
if (NOT printed MATCHES "objects, [0-9]+ bytes")
  message(FATAL_ERROR "the installed package printed: ${printed}")
endif()
