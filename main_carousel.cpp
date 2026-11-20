#define NANOSVG_IMPLEMENTATION
#include "3dparty/nanosvg/src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "3dparty/nanosvg/src/nanosvgrast.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif

#define GLFW_INCLUDE_GLCOREARB
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <glad/glad.h>
#endif
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "common/carousel/carousel.h"
#include "common/carousel/carousel_loader.h"
#include "common/carousel/carousel_to_renderable.h"
#include "common/debugging.h"
#include "common/matrix_stack.h"
#include "common/renderable.h"
#include "common/shaders.h"
#include "common/simple_shapes.h"
#include "common/texture.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "common/gltf_loader.h"

#include "src/camera_input.h"
#include "src/gltf_model.h"
#include "src/scene_renderer.h"

int main(int /*argc*/, char ** /*argv*/) {
  srand(static_cast<unsigned int>(time(nullptr)));

  // Load track and populate cars
  race r;
  carousel_loader::load("map/map.svg", "map/terrain_256.png", r);
  for (int i = 0; i < 10; ++i) {
    float var = ((rand() % 100) / 100.0f) * 0.04f;
    r.add_car((static_cast<float>(i) / 10.0f) + var);
  }

  // Init window, OpenGL, and camera
  GLFWwindow *window = init_window_and_camera(800, 800, "CarOusel");
  if (!window)
    return -1;

  glEnable(GL_MULTISAMPLE);
  glEnable(GL_DEPTH_TEST);
  printout_opengl_glsl_info();

  // Build scene geometry
  renderable fram = shape_maker::frame();
  renderable r_track, r_terrain;
  r_track.create();
  game_to_renderable::to_track(r, r_track);
  r_terrain.create();
  game_to_renderable::to_heightfield(r, r_terrain);

  SunRay sun_ray;
  sun_ray.create();

  // Load shader
  shader basic_shader;
  basic_shader.create_program("shaders/basic.vert", "shaders/basic.frag");
  glUseProgram(basic_shader.program);

  // Load 3D models
  std::vector<std::string> car_files = {
      "3dmodels/Cars/garbage-truck.glb", "3dmodels/Cars/ambulance.glb",
      "3dmodels/Cars/police.glb",        "3dmodels/Cars/sedan.glb",
      "3dmodels/Cars/taxi.glb",          "3dmodels/Cars/suv.glb",
      "3dmodels/Cars/firetruck.glb",     "3dmodels/Cars/race.glb"};
  std::vector<GltfModel> car_models;
  car_models.reserve(car_files.size());
  for (const auto &file : car_files)
    car_models.push_back(load_gltf_model(file, 3.5f));

  GltfModel lamp_model = load_gltf_model("3dmodels/Lamps/street_lamp_01_1k.gltf", 2.5f);

  std::vector<GltfModel> trees_models;
  std::vector<std::string> tree_files = {
      "3dmodels/Nature/CommonTree_1.gltf", "3dmodels/Nature/CommonTree_2.gltf",
      "3dmodels/Nature/CommonTree_3.gltf", "3dmodels/Nature/CommonTree_4.gltf",
      "3dmodels/Nature/Pine_1.gltf",       "3dmodels/Nature/Pine_2.gltf",
      "3dmodels/Nature/Pine_3.gltf",       "3dmodels/Nature/Pine_4.gltf",
      "3dmodels/Nature/TwistedTree_1.gltf","3dmodels/Nature/TwistedTree_2.gltf"};
  std::vector<std::string> plant_files = {
      "3dmodels/Nature/Bush_Common.gltf",
      "3dmodels/Nature/Bush_Common_Flowers.gltf",
      "3dmodels/Nature/Plant_1_Big.gltf",
      "3dmodels/Nature/Plant_7_Big.gltf",
      "3dmodels/Nature/Fern_1.gltf"};
  trees_models.reserve(tree_files.size() + plant_files.size());
  for (const auto &file : tree_files)
    trees_models.push_back(load_gltf_model(file, 8.0f));
  for (const auto &file : plant_files)
    trees_models.push_back(load_gltf_model(file, 3.5f));

  GltfModel cameraman_model = load_gltf_model("3dmodels/camera-drone/source/drone.glb", 3.5f);

  // Load terrain and road textures
  texture grass_tex, asphalt_tex;
  grass_tex.load("Textures/grass_texture.jpg", 0);
  asphalt_tex.load("Textures/road.jpg", 1);

  r.start(18, 30, 0, 900);
  r.update();

  matrix_stack stack;
  int seed = rand();

  while (!glfwWindowShouldClose(window)) {
    srand(seed);
    auto &cam = get_camera_state();
    r.update();

    glm::vec3 sky = get_sky_color(-r.sunlight_direction().y);
    glClearColor(sky.r, sky.g, sky.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniformMatrix4fv(basic_shader["uProj"], 1, GL_FALSE, &cam.proj[0][0]);
    glUniformMatrix4fv(basic_shader["uView"], 1, GL_FALSE, &cam.view[0][0]);

    // Sun direction and intensity in View Space
    glm::vec3 rotated_sun = glm::mat3(cam.tb[cam.curr_tb].matrix()) * r.sunlight_direction();
    glm::vec3 sun_dir_view = glm::mat3(cam.view) * (-rotated_sun);
    float sun_intensity = glm::clamp(-r.sunlight_direction().y * 4.0f, 0.0f, 1.0f);
    glUniform3fv(basic_shader["uSunDirView"], 1, &sun_dir_view[0]);
    glUniform1f(basic_shader["uSunIntensity"], sun_intensity);

    stack.load_identity();
    stack.push();
    stack.mult(cam.tb[cam.curr_tb].matrix());

    // Debug axes and sun ray
    update_model_and_normal(basic_shader, cam.view, stack.m());
    glUniform3f(basic_shader["uColor"], -1.f, 0.6f, 0.f);
    glUniform1i(basic_shader["uLightingEnabled"], 0);
    fram.bind();
    glDrawArrays(GL_LINES, 0, 6);

    glUniform3f(basic_shader["uColor"], 0.f, 0.f, 1.f);
    sun_ray.draw(r.sunlight_direction());

    glm::mat4 cm_norm = r.cameramen()[idx].frame;
    // Normalize world scale and center
    float s = 1.f / r.bbox().diagonal();
    stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(s)));
    stack.mult(glm::translate(glm::mat4(1.f), -r.bbox().center()));

    // Upload lamp lights and draw scene
    upload_lamp_lights(basic_shader, r, lamp_model, cam.view, stack.m());
    draw_terrain(basic_shader, cam.view, stack.m(), r_terrain, grass_tex);
    draw_cars(basic_shader, cam.view, stack, r, car_models);
    draw_cameramen(basic_shader, cam.view, stack, r, cameraman_model);
    draw_track(basic_shader, cam.view, stack.m(), r_track, asphalt_tex);
    draw_trees(basic_shader, cam.view, stack, r, trees_models);
    draw_lamps(basic_shader, cam.view, stack, r, lamp_model);

    stack.pop();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glUseProgram(0);
  glfwTerminate();
  return 0;
}
