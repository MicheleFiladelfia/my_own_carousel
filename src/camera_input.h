#pragma once

#include <cstdio>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "common/trackball.h"

struct CameraState {
  trackball tb[2];
  int curr_tb = 0;
  glm::mat4 proj{1.0f};
  glm::mat4 view{1.0f};
};

inline CameraState &get_camera_state() {
  static CameraState state;
  return state;
}

inline void get_cursor_framebuffer_pos(GLFWwindow *window, double xpos,
                                       double ypos, double &out_x,
                                       double &out_y) {
  int win_w = 0, win_h = 0, fb_w = 0, fb_h = 0;
  glfwGetWindowSize(window, &win_w, &win_h);
  glfwGetFramebufferSize(window, &fb_w, &fb_h);
  out_x = (win_w > 0) ? xpos * (static_cast<double>(fb_w) / win_w) : xpos;
  out_y = (win_h > 0) ? ypos * (static_cast<double>(fb_h) / win_h) : ypos;
}

inline void framebuffer_size_callback(GLFWwindow * /*window*/, int width, int height) {
  glViewport(0, 0, width, height);
  if (height > 0) {
    get_camera_state().proj = glm::perspective(
        glm::radians(45.f), static_cast<float>(width) / static_cast<float>(height),
        0.05f, 10.f);
  }
}

inline void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  auto &cam = get_camera_state();
  double fx = 0.0, fy = 0.0;
  get_cursor_framebuffer_pos(window, xpos, ypos, fx, fy);
  cam.tb[cam.curr_tb].mouse_move(cam.proj, cam.view, fx, fy);
}

inline void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int /*mods*/) {
  auto &cam = get_camera_state();
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    double xpos = 0.0, ypos = 0.0, fx = 0.0, fy = 0.0;
    glfwGetCursorPos(window, &xpos, &ypos);
    get_cursor_framebuffer_pos(window, xpos, ypos, fx, fy);
    cam.tb[cam.curr_tb].mouse_press(cam.proj, cam.view, fx, fy);
  } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    cam.tb[cam.curr_tb].mouse_release();
  }
}

inline void scroll_callback(GLFWwindow * /*window*/, double xoffset, double yoffset) {
  auto &cam = get_camera_state();
  cam.tb[cam.curr_tb].mouse_scroll(xoffset, yoffset);
}

inline void key_callback(GLFWwindow * /*window*/, int /*key*/, int /*scancode*/,
                         int action, int /*mods*/) {
  // Toggle active trackball on any key press
  if (action == GLFW_PRESS) {
    auto &cam = get_camera_state();
    cam.curr_tb = 1 - cam.curr_tb;
  }
}

// Initializes GLFW window, OpenGL context, and camera matrices
inline GLFWwindow *init_window_and_camera(int width, int height, const char *title) {
  if (!glfwInit())
    return nullptr;

  glfwWindowHint(GLFW_SAMPLES, 4);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return nullptr;
  }

  if (glfwRawMouseMotionSupported())
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwMakeContextCurrent(window);

#if !defined(__APPLE__)
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::fprintf(stderr, "Failed to initialize GLAD\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return nullptr;
  }
#endif

  int fb_w = width, fb_h = height;
  glfwGetFramebufferSize(window, &fb_w, &fb_h);
  glViewport(0, 0, fb_w, fb_h);

  auto &cam = get_camera_state();
  cam.tb[0].reset();
  cam.tb[0].set_center_radius(glm::vec3(0.f), 1.f);
  cam.tb[1].reset();
  cam.tb[1].set_center_radius(glm::vec3(0.f), 1.f);
  cam.curr_tb = 0;
  cam.proj = glm::perspective(glm::radians(45.f),
                              static_cast<float>(fb_w) / static_cast<float>(fb_h),
                              0.05f, 10.f);
  cam.view = glm::lookAt(glm::vec3(0.f, 1.f, 1.5f), glm::vec3(0.f),
                         glm::vec3(0.f, 1.f, 0.f));

  return window;
}
