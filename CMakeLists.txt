# Versión mínima de CMake requerida.
cmake_minimum_required(VERSION 3.10)

# Nombre de nuestro proyecto.
project(tex_paint)

# Queremos usar el estándar C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Agregamos nuestras fuentes ---
# Esto ya estaba correcto, incluimos glad.c directamente.
add_executable(tex_paint
        src/main.cpp
        common/third/glad/glad.c
        common/third/imgui/imgui.cpp
        common/third/imgui/imgui_draw.cpp
        common/third/imgui/imgui_tables.cpp
        common/third/imgui/imgui_widgets.cpp
        common/third/imgui/backends/imgui_impl_glfw.cpp
        common/third/imgui/backends/imgui_impl_opengl3.cpp
        common/utils/BezierRenderer.cpp
        common/utils/Callbacks.cpp
        common/utils/CEVersion.cpp
        common/utils/DrawBuffers.cpp
        common/utils/FramebufferTexture.cpp
        common/utils/Geometry.cpp
        common/utils/Image.cpp
        common/utils/Misc.cpp
        common/utils/Model.cpp
        common/utils/ObjMesh.cpp
        common/utils/Shaders.cpp
        common/utils/Texture.cpp
        common/utils/Window.cpp
        src/main.cpp
)

# --- Incluimos los directorios ---
target_include_directories(tex_paint PRIVATE
        common/third/glad
        common/third/imgui
        common/third/imgui/backends
        # common/third/stb   <-- OJO: No tienes STB en esta ruta según tu 'tree'. Lo comento por si acaso.
        # Si lo necesitas, asegúrate que la ruta es correcta.
        common/utils
        libs/glfw/include # <--- Eliminamos la línea de GLEW
        libs/glm
        libs/stb # <-- Agrego la ruta correcta a STB que se ve en tu 'tree'
)

# --- Enlazamos las librerías ---
# Le decimos a CMake que busque los archivos .lib en esta carpeta.
link_directories(
        libs/glfw/lib-vc2022 # <--- Eliminamos la línea de GLEW
)

# Enlazamos nuestro programa con los archivos de librería específicos.
target_link_libraries(tex_paint PRIVATE
        opengl32
        glfw3 # <--- Eliminamos 'glew32s' de aquí
)

# --- ELIMINADO ---
# La siguiente línea era para GLEW, ya no es necesaria.
# target_compile_definitions(tex_paint PRIVATE GLEW_STATIC)

# --- Copiamos recursos necesarios (DLLs y Shaders) ---
# Copia los shaders al directorio de compilación para que el programa los encuentre.
add_custom_command(TARGET tex_paint POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_CURRENT_BINARY_DIR}/shaders
)

# Copia los archivos .dll necesarios para ejecutar el programa en Windows.
if(WIN32)
    # --- ELIMINADO ---
    # Ya no necesitamos copiar glew32.dll
    add_custom_command(TARGET tex_paint POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw/lib-vc2022/glfw3.dll
            ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()