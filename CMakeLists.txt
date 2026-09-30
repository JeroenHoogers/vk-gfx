
cmake_minimum_required(VERSION 3.10)
project(vk-gfx
  VERSION 1.0.0
  LANGUAGES C CXX
)

option(VKGFX_BUILD_EXAMPLES "Build vk-gfx examples" OFF)

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
    ${SOURCE_DIR}/window.cpp
    ${SOURCE_DIR}/swapchain.cpp
    ${SOURCE_DIR}/pipeline.cpp
    ${SOURCE_DIR}/sync.cpp
    ${SOURCE_DIR}/command_buffer.cpp
    ${SOURCE_DIR}/render_pass.cpp
    ${SOURCE_DIR}/render_target.cpp
    ${SOURCE_DIR}/mesh.cpp
    ${SOURCE_DIR}/image.cpp
    ${SOURCE_DIR}/resource.cpp
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
if(VKGFX_BUILD_EXAMPLES)
  find_package(glfw3 3.3 REQUIRED)
  find_package(glm REQUIRED)

  find_program(SLANGC_EXECUTABLE
      NAMES slangc
      HINTS
          ${SLANGC_ROOT}
          ENV SLANGC_ROOT
      REQUIRED
  )

  set (CMAKE_CXX_STANDARD 20)

  set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
  set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

  set(VERTEX_SHADERS
      "${SHADER_SRC_DIR}/shader.slang"
      "${SHADER_SRC_DIR}/triangle.slang"
      "${SHADER_SRC_DIR}/textured.slang"
      "${SHADER_SRC_DIR}/text.slang" # will be compiled to spirv_1_3 instead of 1_0 since it uses SV_InstanceID
  )

  set(FRAGMENT_SHADERS
      "${SHADER_SRC_DIR}/shader.slang"
      "${SHADER_SRC_DIR}/triangle.slang"
      "${SHADER_SRC_DIR}/textured.slang"
      "${SHADER_SRC_DIR}/text.slang"
  )

  function(compile_slang_stage OUTPUTS_VAR STAGE ENTRY)
      set(COMPILED_OUTPUTS)

      foreach(SHADER IN LISTS ARGN)
          get_filename_component(SHADER_NAME "${SHADER}" NAME_WE)

          set(OUTPUT
              "${SHADER_BINARY_DIR}/${SHADER_NAME}.${STAGE}.spv"
          )

          add_custom_command(
              OUTPUT "${OUTPUT}"
              COMMAND ${CMAKE_COMMAND} -E make_directory
                      "${SHADER_BINARY_DIR}"
              COMMAND "${SLANGC_EXECUTABLE}"
                      -target spirv
                      -profile "spirv_1_0+GLSL_150" # spirv_1_3 (vulkan v1.1)
                      -emit-spirv-via-glsl # -emit-spirv-directly (>=v1.1)
                      -entry ${ENTRY}
                      -stage ${STAGE}
                      ${SHADER}
                      -o "${OUTPUT}"
              DEPENDS "${SHADER}"
              COMMENT "Compiling Vulkan shader ${SHADER_NAME} [${STAGE}]..."
              VERBATIM
          )
          list(APPEND COMPILED_OUTPUTS "${OUTPUT}")
      endforeach()

      set("${OUTPUTS_VAR}" "${COMPILED_OUTPUTS}" PARENT_SCOPE)
  endfunction()

  compile_slang_stage(
      COMPILED_VERTEX_SHADERS
      vertex
      vertexMain
      ${VERTEX_SHADERS}
  )

  compile_slang_stage(
      COMPILED_FRAGMENT_SHADERS
      fragment
      fragmentMain
      ${FRAGMENT_SHADERS}
  )

  set(COMPILED_SHADERS
      ${COMPILED_VERTEX_SHADERS}
      ${COMPILED_FRAGMENT_SHADERS}
  )

  add_custom_target(vkgfx_compile_shaders ALL
      DEPENDS ${COMPILED_SHADERS}
  )

  set(EXAMPLE_LIBS ${LIBRARIES} glfw glm::glm vk-gfx)

  # triangle
  include_directories(${INCLUDE_DIR} ${EXAMPLES_DIR}/thirdparty)
  add_executable(triangle-example ${SRC_EXAMPLES} ${EXAMPLES_DIR}/triangle.cpp)
  target_link_libraries(triangle-example PRIVATE ${EXAMPLE_LIBS})
  add_dependencies(triangle-example vkgfx_compile_shaders)

  # cube
  add_executable(cube-example ${SRC_EXAMPLES} ${EXAMPLES_DIR}/cube.cpp)
  target_link_libraries(cube-example PRIVATE ${EXAMPLE_LIBS})
  add_dependencies(cube-example vkgfx_compile_shaders)

  add_custom_command(TARGET cube-example POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              "${CMAKE_SOURCE_DIR}/assets"
              "$<TARGET_FILE_DIR:cube-example>/assets"
  )

  # text
  set(DRAFT_TYPE_DIR ${EXAMPLES_DIR}/thirdparty/draft-type)
  add_subdirectory(${DRAFT_TYPE_DIR})
  include_directories(${DRAFT_TYPE_DIR}/include)

  add_executable(text-example ${SRC_EXAMPLES} ${EXAMPLES_DIR}/text.cpp)
  target_link_libraries(text-example PRIVATE draft_type ${EXAMPLE_LIBS})
  add_dependencies(text-example vkgfx_compile_shaders)

  add_custom_command(TARGET text-example POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              "${CMAKE_SOURCE_DIR}/${DRAFT_TYPE_DIR}/assets/hershey-fonts"
              "$<TARGET_FILE_DIR:text-example>/assets/hershey-fonts"
  )

  # imgui
  set(IMGUI_DIR ${EXAMPLES_DIR}/thirdparty/imgui)
  include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)
  set(SRC_IMGUI
   ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
   ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
   ${IMGUI_DIR}/imgui.cpp
   ${IMGUI_DIR}/imgui_draw.cpp
   ${IMGUI_DIR}/imgui_demo.cpp
   ${IMGUI_DIR}/imgui_tables.cpp
   ${IMGUI_DIR}/imgui_widgets.cpp
   ${IMGUI_DIR}/imconfig.h
  )

  add_executable(imgui-example ${SRC_EXAMPLES} ${SRC_IMGUI} ${EXAMPLES_DIR}/imgui.cpp)
  target_link_libraries(imgui-example PRIVATE ${EXAMPLE_LIBS})
  add_dependencies(imgui-example vkgfx_compile_shaders)
endif(VKGFX_BUILD_EXAMPLES)
