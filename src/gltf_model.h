#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "common/box3.h"
#include "common/gltf_loader.h"
#include "common/matrix_stack.h"
#include "common/renderable.h"
#include "common/shaders.h"

struct GltfModel {
  std::vector<renderable> meshes;
  box3 bbox;
  float scale = 1.0f;
  glm::vec3 center{0.0f};
};

// Loads a glTF/glb file and computes uniform scale from target diagonal
inline GltfModel load_gltf_model(const std::string &path, float target_diagonal) {
  GltfModel model;
  gltf_loader loader;
  loader.load_to_renderable(path, model.meshes, model.bbox);
  model.scale = target_diagonal / model.bbox.diagonal();
  model.center = model.bbox.center();
  return model;
}

// Draws all meshes of a normalized glTF model grounded at y = 0
template <typename UpdateMatricesFn>
inline void draw_gltf_model(GltfModel &model, shader &sh,
                            matrix_stack &stack,
                            UpdateMatricesFn &&update_matrices,
                            bool rotate_y_180 = false,
                            int emissive_mesh_idx = -1) {
  stack.push();
  if (rotate_y_180)
    stack.mult(glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f)));
  stack.mult(glm::scale(glm::mat4(1.f), glm::vec3(model.scale)));
  stack.mult(glm::translate(glm::mat4(1.f),
                            -glm::vec3(model.center.x, model.bbox.min.y, model.center.z)));

  for (size_t i = 0; i < model.meshes.size(); ++i) {
    auto &mesh = model.meshes[i];
    if (static_cast<int>(i) == emissive_mesh_idx)
      glUniform3f(sh["uEmissive"], 1.0f, 0.85f, 0.45f);
    else
      glUniform3f(sh["uEmissive"], 0.0f, 0.0f, 0.0f);

    glUniform1i(sh["uUseVertexUV"], 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh.mater.base_color_texture);
    glUniform1i(sh["uTexture"], 0);
    glUniform1i(sh["uUseTexture"], 1);

    stack.push();
    stack.mult(mesh.transform);
    update_matrices(stack.m());
    mesh.bind();
    glDrawElements(mesh().mode, mesh().count, mesh().itype, 0);
    stack.pop();
  }
  glUniform3f(sh["uEmissive"], 0.0f, 0.0f, 0.0f);

  stack.pop();
}
