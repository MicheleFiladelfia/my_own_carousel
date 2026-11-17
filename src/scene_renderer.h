#pragma once

#include <algorithm>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "common/carousel/carousel.h"
#include "common/matrix_stack.h"
#include "common/renderable.h"
#include "common/shaders.h"
#include "common/texture.h"
#include "src/gltf_model.h"

struct SunRay {
  GLuint vao = 0;
  GLuint vbo = 0;

  void create() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
  }

  void draw(const glm::vec3 &dir) const {
    float pts[6] = {0.f, 0.f, 0.f, dir.x, dir.y, dir.z};
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pts), pts);
    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, 2);
  }
};

inline glm::vec3 get_sky_color(float sun_height) {
  float day_factor = glm::clamp(sun_height, 0.0f, 1.0f);
  return glm::mix(glm::vec3(0.02f, 0.02f, 0.08f),
                  glm::vec3(0.0f, 0.67f, 0.85f), day_factor);
}

inline void update_model_and_normal(shader &sh, const glm::mat4 &view,
                                    const glm::mat4 &model) {
  glUniformMatrix4fv(sh["uModel"], 1, GL_FALSE, &model[0][0]);
  glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(view * model)));
  glUniformMatrix3fv(sh["uNormalMatrix"], 1, GL_FALSE, &normal_matrix[0][0]);
}

// Uploads lamp point lights in View Space (radius scaled with current stack)
inline void upload_lamp_lights(shader &sh, const race &r,
                               const GltfModel &lamp_model,
                               const glm::mat4 &view, const glm::mat4 &scene_model) {
  float lamp_h = (lamp_model.bbox.max.y - lamp_model.bbox.min.y) * lamp_model.scale;
  int num_lamps = std::min(static_cast<int>(r.lamps().size()), 32);
  std::vector<glm::vec3> lamp_pos_view(num_lamps);

  for (int i = 0; i < num_lamps; ++i) {
    glm::vec3 world_pos = r.lamps()[i].pos + glm::vec3(0.f, 0.85f * lamp_h, 0.f);
    lamp_pos_view[i] = glm::vec3(view * scene_model * glm::vec4(world_pos, 1.f));
  }

  glUniform1i(sh["uNumLamps"], num_lamps);
  glUniform1f(sh["uLampRadiusView"], 12.0f * glm::length(glm::vec3(scene_model[0])));
  if (num_lamps > 0)
    glUniform3fv(sh["uLampPosView"], num_lamps, &lamp_pos_view[0][0]);
}

inline void draw_terrain(shader &sh, const glm::mat4 &view,
                         const glm::mat4 &model, renderable &r_terrain,
                         const texture &grass_tex) {
  glUniform1i(sh["uUseVertexUV"], 0);
  glUniform1f(sh["uTexScale"], 0.1f);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(1.0f, 1.0f);
  update_model_and_normal(sh, view, model);
  glUniform3f(sh["uColor"], 0.2f, 0.6f, 0.2f);
  glUniform1i(sh["uLightingEnabled"], 1);
  glUniform1f(sh["uSpecularStrength"], 0.02f);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, grass_tex.id);
  glUniform1i(sh["uTexture"], 0);
  glUniform1i(sh["uUseTexture"], 1);

  r_terrain.bind();
  glDrawElements(r_terrain().mode, r_terrain().count, r_terrain().itype, 0);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glUniform1i(sh["uUseTexture"], 0);
}

inline void draw_track(shader &sh, const glm::mat4 &view,
                       const glm::mat4 &model, renderable &r_track,
                       const texture &asphalt_tex) {
  update_model_and_normal(sh, view, model);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, asphalt_tex.id);
  glUniform1i(sh["uTexture"], 1);
  glUniform1i(sh["uUseTexture"], 1);

  r_track.bind();
  glUniform1i(sh["uUseVertexUV"], 1);
  glUniform3f(sh["uColor"], 0.15f, 0.15f, 0.15f);
  glUniform1i(sh["uLightingEnabled"], 1);
  glUniform1f(sh["uSpecularStrength"], 0.01f);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, r_track.vn);
  glUniform1i(sh["uUseVertexUV"], 0);
  glUniform1i(sh["uUseTexture"], 0);
}

inline void draw_cars(shader &sh, const glm::mat4 &view,
                      matrix_stack &stack, const race &r,
                      std::vector<GltfModel> &car_models) {
  glUniform1i(sh["uLightingEnabled"], 1);
  glUniform1f(sh["uSpecularStrength"], 0.85f);

  auto update_fn = [&](const glm::mat4 &m) { update_model_and_normal(sh, view, m); };
  for (const auto &c : r.cars()) {
    GltfModel &cm = car_models[rand() % car_models.size()];
    stack.push();
    stack.mult(c.frame);
    draw_gltf_model(cm, sh, stack, update_fn, true);
    stack.pop();
  }

  glUniform1i(sh["uUseVertexUV"], 0);
  glUniform1i(sh["uUseTexture"], 0);
}

inline void draw_lamps(shader &sh, const glm::mat4 &view,
                       matrix_stack &stack, const race &r,
                       GltfModel &lamp_model) {
  glUniform1i(sh["uLightingEnabled"], 1);
  glUniform1f(sh["uSpecularStrength"], 0.2f);

  auto update_fn = [&](const glm::mat4 &m) { update_model_and_normal(sh, view, m); };
  for (const auto &lamp : r.lamps()) {
    stack.push();
    stack.mult(glm::translate(glm::mat4(1.f), lamp.pos));
    draw_gltf_model(lamp_model, sh, stack, update_fn, false, 1);
    stack.pop();
  }

  glUniform1i(sh["uUseVertexUV"], 0);
  glUniform1i(sh["uUseTexture"], 0);
}

inline void draw_cameramen(shader &sh, const glm::mat4 &view,
                           matrix_stack &stack, const race &r, renderable &fram) {
  glUniform1i(sh["uLightingEnabled"], 0);
  glUniform1f(sh["uSpecularStrength"], 0.2f);
  fram.bind();

  for (const auto &cm : r.cameramen()) {
    stack.push();
    stack.mult(cm.frame);
    stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(4.f)));
    update_model_and_normal(sh, view, stack.m());
    glUniform3f(sh["uColor"], -1.f, 0.6f, 0.f);
    glDrawArrays(GL_LINES, 0, 6);
    stack.pop();
  }
}

inline void draw_trees(shader &sh, const glm::mat4 &view, matrix_stack &stack,
                       const race &r, std::vector<GltfModel> &trees_models) {
  glUniform1i(sh["uLightingEnabled"], 1);
  glUniform1f(sh["uSpecularStrength"], 0.15f);

  auto update_fn = [&](const glm::mat4 &m) { update_model_and_normal(sh, view, m); };
  for (const auto &tree : r.trees()) {
    GltfModel &plant = trees_models[rand() % trees_models.size()];
    stack.push();
    stack.mult(glm::translate(glm::mat4(1.f), tree.pos));
    draw_gltf_model(plant, sh, stack, update_fn, false);
    stack.pop();
  }

  glUniform1i(sh["uUseVertexUV"], 0);
  glUniform1i(sh["uUseTexture"], 0);
}

