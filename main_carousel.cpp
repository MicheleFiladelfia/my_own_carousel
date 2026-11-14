#define NANOSVG_IMPLEMENTATION // Expands implementation
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

#include <algorithm>
#include <iostream>
#include <string>

#include "common/carousel/carousel.h"
#include "common/carousel/carousel_loader.h"
#include "common/carousel/carousel_to_renderable.h"
#include "common/debugging.h"
#include "common/intersection.h"
#include "common/matrix_stack.h"
#include "common/renderable.h"
#include "common/shaders.h"
#include "common/simple_shapes.h"
#include "common/trackball.h"
#include "common/texture.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "common/gltf_loader.h"

trackball tb[2];
int curr_tb;

/* projection matrix*/
glm::mat4 proj;

/* view matrix */
glm::mat4 view;

matrix_stack stack;
float scaling_factor = 1.0;

static void get_cursor_framebuffer_pos(GLFWwindow *window, double xpos,
                                       double ypos, double &out_x,
                                       double &out_y) {
  int win_w = 0, win_h = 0, fb_w = 0, fb_h = 0;
  glfwGetWindowSize(window, &win_w, &win_h);
  glfwGetFramebufferSize(window, &fb_w, &fb_h);
  out_x = (win_w > 0) ? xpos * ((double)fb_w / win_w) : xpos;
  out_y = (win_h > 0) ? ypos * ((double)fb_h / win_h) : ypos;
}

/* callback function called when the framebuffer size changes */
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
  if (height > 0) {
    proj = glm::perspective(glm::radians(45.f), (float)width / (float)height,
                            0.05f, 10.f);
  }
}

/* callback function called when the mouse is moving */
static void cursor_position_callback(GLFWwindow *window, double xpos,
                                     double ypos) {
  double fx, fy;
  get_cursor_framebuffer_pos(window, xpos, ypos, fx, fy);
  tb[curr_tb].mouse_move(proj, view, fx, fy);
}

/* callback function called when a mouse button is pressed */
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    double xpos, ypos, fx, fy;
    glfwGetCursorPos(window, &xpos, &ypos);
    get_cursor_framebuffer_pos(window, xpos, ypos, fx, fy);
    tb[curr_tb].mouse_press(proj, view, fx, fy);
  } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    tb[curr_tb].mouse_release();
  }
}

/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  tb[curr_tb].mouse_scroll(xoffset, yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  /* every time any key is presse it switch from controlling trackball tb[0] to
   * tb[1] and viceversa */
  if (action == GLFW_PRESS)
    curr_tb = 1 - curr_tb;
}

glm::vec3 get_sky_color(float sun_height){
	float day_factor = glm::clamp(sun_height, 0.0f, 1.0f); // 0 = notte, 1 = giorno

    glm::vec3 night_sky(0.02f, 0.02f, 0.08f);
    glm::vec3 day_sky(0.0f, 0.67f, 0.85f);

	return glm::mix(night_sky, day_sky, day_factor);
}

struct CarModel {
    std::vector<renderable> meshes;
    box3 bbox;
    float scale;
    glm::vec3 center;
};

int main(int argc, char **argv) {
  srand(time(NULL));
  race r;

  carousel_loader::load("small_test.svg", "terrain_256.png", r);

  // add 10 cars with semirandom position
  for (int i = 0; i < 10; ++i) {
    float var = ((rand() % 100) / 100.0f - 0.5f) * 0.04f;
    float position = (float(i) / 10.0f) + var;
    r.add_car(position);
  }

  GLFWwindow *window;

  /* Initialize the library */
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_SAMPLES, 4);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  /* Create a windowed mode window and its OpenGL context */
  window = glfwCreateWindow(800, 800, "CarOusel", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  /* declare the callback functions on mouse events */
  if (glfwRawMouseMotionSupported())
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  /* Make the window's context current */
  glfwMakeContextCurrent(window);

#if !defined(__APPLE__)
  // Load GL symbols *after* the context is current
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::fprintf(stderr, "Failed to initialize GLAD\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }
#endif

  glEnable(GL_MULTISAMPLE);
  printout_opengl_glsl_info();

  renderable fram = shape_maker::frame();

  renderable r_cube = shape_maker::cube();

  renderable r_track;
  r_track.create();
  game_to_renderable::to_track(r, r_track);

  renderable r_terrain;
  r_terrain.create();
  game_to_renderable::to_heightfield(r, r_terrain);

  renderable r_trees;
  r_trees.create();
  game_to_renderable::to_tree(r, r_trees);

  renderable r_lamps;
  r_lamps.create();
  game_to_renderable::to_lamps(r, r_lamps);

  // Setup VAO and VBO for sunlight direction line
  GLuint sun_vao = 0, sun_vbo = 0;
  glGenVertexArrays(1, &sun_vao);
  glGenBuffers(1, &sun_vbo);
  glBindVertexArray(sun_vao);
  glBindBuffer(GL_ARRAY_BUFFER, sun_vbo);
  glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glBindVertexArray(0);

  shader basic_shader;
  basic_shader.create_program("shaders/basic.vert", "shaders/basic.frag");

  /* use the program shader "program_shader" */
  glUseProgram(basic_shader.program);

  /* define the viewport using the actual framebuffer size (Retina display
   * support) */
  int fb_w = 800, fb_h = 800;
  glfwGetFramebufferSize(window, &fb_w, &fb_h);
  glViewport(0, 0, fb_w, fb_h);

  tb[0].reset();
  tb[0].set_center_radius(glm::vec3(0, 0, 0), 1.f);
  tb[1].reset();
  tb[1].set_center_radius(glm::vec3(0, 0, 0), 1.f);
  curr_tb = 0;

  proj = glm::perspective(glm::radians(45.f), (float)fb_w / (float)fb_h, 0.05f,
                          10.f);

  view = glm::lookAt(glm::vec3(0, 1.f, 1.5), glm::vec3(0.f, 0.f, 0.f),
                     glm::vec3(0.0, 1.f, 0.f));

  glUniformMatrix4fv(basic_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
  glUniformMatrix4fv(basic_shader["uView"], 1, GL_FALSE, &view[0][0]);

  //cars models
  std::vector<CarModel> car_models;
  std::vector<std::string> car_files = {
    "3dmodels/garbage-truck.glb",
    "3dmodels/ambulance.glb",
    "3dmodels/police.glb",
    "3dmodels/sedan.glb",
    "3dmodels/taxi.glb",
    "3dmodels/suv.glb",
    "3dmodels/firetruck.glb",
    "3dmodels/race.glb"
};

for (const auto& f : car_files) {
    CarModel cm;
    gltf_loader loader;
    loader.load_to_renderable(f, cm.meshes, cm.bbox);
    cm.scale = 3.5f / cm.bbox.diagonal();
    cm.center = cm.bbox.center();
    car_models.push_back(cm);
}

  //textures
  texture grass_tex, asphalt_tex;
  grass_tex.load("grass_texture.jpg", 0);
  asphalt_tex.load("asphalt_texture.png", 1);

  r.start(18, 30, 0, 900); //time settings
  r.update();

  matrix_stack stack;

  glEnable(GL_DEPTH_TEST);
  int seed = rand();
  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window)) {
    srand(seed);
    /* Render here */

    // sky color
    float sun_height = -r.sunlight_direction()
                            .y; // varia tra -1 (mezzanotte) e +1 (mezzogiorno)

    glm::vec3 current_sky = get_sky_color(sun_height);
    glClearColor(current_sky.r, current_sky.g, current_sky.b, 1.f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    check_gl_errors(__LINE__, __FILE__);

    r.update();
    glUniformMatrix4fv(basic_shader["uProj"], 1, GL_FALSE, &proj[0][0]);
    glUniformMatrix4fv(basic_shader["uView"], 1, GL_FALSE, &view[0][0]);

    // Direzione del Sole (verso la sorgente) espressa in View Space
    glm::vec3 rotated_sun =
        glm::mat3(tb[curr_tb].matrix()) * r.sunlight_direction();
    glm::vec3 sun_dir_view = glm::mat3(view) * (-rotated_sun);
    glUniform3fv(basic_shader["uSunDirView"], 1, &sun_dir_view[0]);

    float sun_intensity = glm::clamp(-r.sunlight_direction().y * 4.0f, 0.0f, 1.0f);
    glUniform1f(basic_shader["uSunIntensity"], sun_intensity);    

    auto update_model_and_normal = [&](const glm::mat4 &model_mat) {
      glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &model_mat[0][0]);
      glm::mat3 normal_matrix =
          glm::transpose(glm::inverse(glm::mat3(view * model_mat)));
      glUniformMatrix3fv(basic_shader["uNormalMatrix"], 1, GL_FALSE,
                         &normal_matrix[0][0]);
    };

    stack.load_identity();
    stack.push();
    stack.mult(tb[curr_tb].matrix());
    update_model_and_normal(stack.m());
    glUniform3f(basic_shader["uColor"], -1.f, 0.6f, 0.f);
    glUniform1i(basic_shader["uLightingEnabled"], 0); // Disabilita luce per assi
    fram.bind();
    glDrawArrays(GL_LINES, 0, 6);

    // Draw sunlight direction line
    float sun_pts[6] = {0.f,
                        0.f,
                        0.f,
                        r.sunlight_direction().x,
                        r.sunlight_direction().y,
                        r.sunlight_direction().z};
    glBindBuffer(GL_ARRAY_BUFFER, sun_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sun_pts), sun_pts);
    glBindVertexArray(sun_vao);
    update_model_and_normal(stack.m());
    glUniform3f(basic_shader["uColor"], 0.f, 0.f, 1.f);
    glUniform1i(basic_shader["uLightingEnabled"], 0); // Disabilita luce per raggio sole
    glDrawArrays(GL_LINES, 0, 2);

    float s = 1.f / r.bbox().diagonal();
    glm::vec3 c = r.bbox().center();

    stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(s)));
    stack.mult(glm::translate(glm::mat4(1.f), -c));

    // terrain (illuminato, opaco)
    glUniform1i(basic_shader["uUseVertexUV"], 0);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    update_model_and_normal(stack.m());
    glUniform3f(basic_shader["uColor"], 0.2f, 0.6f, 0.2f);
    glUniform1i(basic_shader["uLightingEnabled"], 1);
    glUniform1f(basic_shader["uSpecularStrength"], 0.02f);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grass_tex.id);
    glUniform1i(basic_shader["uTexture"], 0);
    glUniform1i(basic_shader["uUseTexture"], 1);

    r_terrain.bind();
    glDrawElements(r_terrain().mode, r_terrain().count, r_terrain().itype, 0);
    glDisable(GL_POLYGON_OFFSET_FILL);

    glUniform1i(basic_shader["uUseTexture"], 0);

    // cars (carrozzeria lucida)
    glUniform1i(basic_shader["uUseVertexUV"], 1);
    glUniform1i(basic_shader["uLightingEnabled"], 1);
    glUniform1f(basic_shader["uSpecularStrength"], 0.85f);
    for (unsigned int ic = 0; ic < r.cars().size(); ++ic) {
      CarModel& cm = car_models[rand() % car_models.size()];

      stack.push();
      stack.mult(r.cars()[ic].frame);
      stack.mult(glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f)));
      stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(cm.scale)));
      stack.mult(glm::translate(glm::mat4(1.f), -glm::vec3(cm.center.x, cm.bbox.min.y, cm.center.z)));
      
      for (unsigned int im = 0; im < cm.meshes.size(); ++im) {
          glUniform1i(basic_shader["uUseVertexUV"], 1);
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, cm.meshes[im].mater.base_color_texture);
          glUniform1i(basic_shader["uTexture"], 0);
          glUniform1i(basic_shader["uUseTexture"], 1);
          

          stack.push();
          stack.mult(cm.meshes[im].transform);
          update_model_and_normal(stack.m());
          cm.meshes[im].bind();
          glDrawElements(cm.meshes[im]().mode, cm.meshes[im]().count, cm.meshes[im]().itype, 0);
          stack.pop();
      }
      stack.pop();
    }
    glUniform1i(basic_shader["uUseVertexUV"], 0);
    glUniform1i(basic_shader["uUseTexture"], 0);

    // cameramens
    glUniform1i(basic_shader["uLightingEnabled"], 0);
    glUniform1f(basic_shader["uSpecularStrength"], 0.2f);
    fram.bind();
    for (unsigned int ic = 0; ic < r.cameramen().size(); ++ic) {
      stack.push();
      stack.mult(r.cameramen()[ic].frame);
      stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(4, 4, 4)));
      update_model_and_normal(stack.m());
      glUniform3f(basic_shader["uColor"], -1.f, 0.6f, 0.f);
      glDrawArrays(GL_LINES, 0, 6);
      stack.pop();
    }

    // track (illuminata, asfalto satinato)
    update_model_and_normal(stack.m());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, asphalt_tex.id);
    glUniform1i(basic_shader["uTexture"], 1);
    glUniform1i(basic_shader["uUseTexture"], 1);

    r_track.bind();
    glUniform3f(basic_shader["uColor"], 0.15f, 0.15f, 0.15f);
    glUniform1i(basic_shader["uLightingEnabled"], 1);
    glUniform1f(basic_shader["uSpecularStrength"], 0.01f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, r_track.vn);

    glUniform1i(basic_shader["uUseTexture"], 0);

    // trees (wireframe/linee)
    r_trees.bind();
    glUniform3f(basic_shader["uColor"], 0.f, 1.0f, 0.f);
    glUniform1i(basic_shader["uLightingEnabled"], 0);
    glDrawArrays(GL_LINES, 0, r_trees.vn);

    // lamps (wireframe/linee)
    r_lamps.bind();
    glUniform3f(basic_shader["uColor"], 1.f, 1.0f, 0.f);
    glUniform1i(basic_shader["uLightingEnabled"], 0);
    glDrawArrays(GL_LINES, 0, r_lamps.vn);

    stack.pop();
    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();
  }
  glUseProgram(0);
  glfwTerminate();
  return 0;
}
