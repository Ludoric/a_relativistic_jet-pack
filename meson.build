# g++ -I./include src/4d_sdf.cpp src/shader_utils.cpp src/glad.c -lglfw -O3 -o 4d_sdf

project('a_relativistic_jetpack', 'cpp', 'c')
deps = [dependency('glfw3'), dependency('GL')]
src = ['src'/'4d_sdf.cpp',
       'src'/'shader_utils.cpp',
       'src'/'glad.c']
incdir = include_directories('include')

executable('a_relativistic_jetpack', sources: src, include_directories : incdir, dependencies: deps)


# install_data(
#     join_paths('src', 'lambda2RGB.png'),
#     install_dir: join_paths('resources')
# )
# install_data(
#     join_paths('src', 'shaders', '4d_sdfs.cs.glsl'),
#     install_dir: join_paths('resources')
# )
# install_data(
#     join_paths('src', 'shaders', 'vert_shader.vs.glsl'),
#     install_dir: join_paths('resources')
# )
# install_data(
#     join_paths('src', 'shaders', 'frag_shader.fs.glsl'),
#     install_dir: join_paths('resources')
# )

copy = find_program('copyforinstall.py')

run_command(copy, 'src'/'lambda2RGB.png', 'resources'/'lambda2RGB.png', check: true)
run_command(copy, 'src'/'shaders'/'4d_sdfs.cs.glsl', 'resources'/'4d_sdfs.cs.glsl', check: true)
run_command(copy, 'src'/'shaders'/'vert_shader.vs.glsl', 'resources'/'vert_shader.vs.glsl', check: true)
run_command(copy, 'src'/'shaders'/'frag_shader.fs.glsl', 'resources'/'frag_shader.fs.glsl', check: true)
