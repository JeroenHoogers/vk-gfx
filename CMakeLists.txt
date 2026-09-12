
cmake_minimum_required(VERSION 3.10)
project(vk-gfx
  VERSION 1.0.0
  LANGUAGES C CXX
)

option(BUILD_EXAMPLES "Build examples" ON)

# export compile commands for IDE
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

if ( CMAKE_COMPILER_IS_GNUCC )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wuninitialized -Wpedantic -fPIC")
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-variable -Wno-error=unused-function -Wno-error=unused-parameter")
  endif()
endif ( CMAKE_COMPILER_IS_GNUCC )
if ( MSVC )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /MP")
endif ( MSVC )

find_package(Vulkan REQUIRED)
set(LIBRARIES Vulkan::Vulkan)


set(SOURCE_DIR src)
set(INCLUDE_DIR include)
set(EXAMPLES_DIR examples)
set(ASSETS_DIR assets)

set(SRC_LIB
    ${SOURCE_DIR}/device.cpp
    ${SOURCE_DIR}/buffer.cpp
    ${SOURCE_DIR}/swapchain.cpp
    ${SOURCE_DIR}/pipeline.cpp
    ${SOURCE_DIR}/sync.cpp
    ${SOURCE_DIR}/command_buffer.cpp
    ${SOURCE_DIR}/render_pass.cpp
    ${SOURCE_DIR}/render_target.cpp
    ${SOURCE_DIR}/mesh.cpp
    ${SOURCE_DIR}/descriptor_set.cpp
    ${SOURCE_DIR}/uniform_buffer.cpp
)

set(SRC_EXAMPLES
  ${EXAMPLES_DIR}/glfw_window.cpp
)

add_library(vk-gfx STATIC ${SRC_INCLUDE} ${SRC_LIB})
target_link_libraries(vk-gfx PRIVATE ${LIBRARIES})

target_include_directories(vk-gfx
    PUBLIC
        ${INCLUDE_DIR}
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${SOURCE_DIR}
)

# Example executable
if(BUILD_EXAMPLES)
  find_package(glfw3 3.3 REQUIRED)
  find_package(glm REQUIRED)

  set (CMAKE_CXX_STANDARD 20)

  set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
  set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

  set(SRC_SHADERS
    ${SHADER_SRC_DIR}/triangle.vert
    ${SHADER_SRC_DIR}/triangle.frag
    ${SHADER_SRC_DIR}/shader.vert
    ${SHADER_SRC_DIR}/shader.frag
  )

  set(COMPILED_SHADERS)

  foreach(SHADER IN LISTS SRC_SHADERS)
    get_filename_component(SHADER_NAME "${SHADER}" NAME)
    set(OUTPUT "${SHADER_BINARY_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
          OUTPUT "${OUTPUT}"
          COMMAND ${CMAKE_COMMAND} -E make_directory
                  "${SHADER_BINARY_DIR}"
          COMMAND glslc
                  ${SHADER}
                  -o "${OUTPUT}"
          DEPENDS "${SHADER}"
          COMMENT "Compiling Vulkan shader ${SHADER_NAME}..."
          VERBATIM
      )

    list(APPEND COMPILED_SHADERS "${OUTPUT}")
  endforeach()

  add_custom_target(compile_shaders ALL
      DEPENDS ${COMPILED_SHADERS}
  )

  set(EXAMPLE_LIBS ${LIBRARIES} glfw glm::glm)

  include_directories(${INCLUDE_DIR})
  add_executable(triangle-example ${SRC_EXAMPLES} ${EXAMPLES_DIR}/triangle.cpp)
  target_link_libraries(triangle-example PRIVATE vk-gfx ${EXAMPLE_LIBS})
  add_dependencies(triangle-example compile_shaders)

  add_executable(cube-example ${SRC_EXAMPLES} ${EXAMPLES_DIR}/cube.cpp)
  target_link_libraries(cube-example PRIVATE vk-gfx ${EXAMPLE_LIBS})
  add_dependencies(cube-example compile_shaders)
endif(BUILD_EXAMPLES)
