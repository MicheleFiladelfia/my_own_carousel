#pragma once

#include "../renderable.h"
#include "carousel.h"



struct game_to_renderable {
	
	static void ct(float* dst, glm::vec3 src) {
		dst[0] = src.x;
		dst[1] = src.y;
		dst[2] = src.z;
	}
	static void to_track(const race & r, renderable& r_t)  {
		
		std::vector<float> buffer_pos;
		std::vector<float> buffer_norm;
		const size_t n = r.t().curbs[0].size();
		buffer_pos.resize((n + 1) * 2 * 3);
		buffer_norm.resize((n + 1) * 2 * 3);
		const float TRACK_Y_OFFSET = 0.05f;
		for (unsigned int i = 0; i <= n; ++i) {
			glm::vec3 c0 = r.t().curbs[0][i % n];
			glm::vec3 c1 = r.t().curbs[1][i % n];
			c0.y += TRACK_Y_OFFSET;
			c1.y += TRACK_Y_OFFSET;
			ct(&buffer_pos[(2 * i  ) * 3], c0);
			ct(&buffer_pos[(2 * i+1) * 3], c1);

			glm::vec3 c0_prev = r.t().curbs[0][(i + n - 1) % n];
			glm::vec3 c0_next = r.t().curbs[0][(i + 1) % n];
			glm::vec3 norm = glm::cross(c0_next - c0_prev, c1 - c0);
			if (norm.y < 0.0f) norm = -norm;
			norm = (glm::length(norm) > 0.0f) ? glm::normalize(norm) : glm::vec3(0.0f, 1.0f, 0.0f);
			ct(&buffer_norm[(2 * i  ) * 3], norm);
			ct(&buffer_norm[(2 * i+1) * 3], norm);
		}

		r_t.add_vertex_attribute<float>(&buffer_pos[0], static_cast<unsigned int>(buffer_pos.size()), 0, 3);
		r_t.add_vertex_attribute<float>(&buffer_norm[0], static_cast<unsigned int>(buffer_norm.size()), 2, 3);
	}

	static void to_stick_object(const std::vector<stick_object>& vec, renderable& r_t) {

		std::vector<float> buffer_pos;
		buffer_pos.resize((vec.size()*2) * 3 );
		for (unsigned int i = 0; i < vec.size();++i) {
			ct(&buffer_pos[(2 * i) * 3], vec[i].pos);
			ct(&buffer_pos[(2 * i+1) * 3], vec[i].pos+glm::vec3(0, vec[i].height,0));
		}

		r_t.add_vertex_attribute<float>(&buffer_pos[0], static_cast<unsigned int>(buffer_pos.size()), 0, 3);
	}

	static void to_tree(const race& r, renderable& r_t) {
		to_stick_object(r.trees(), r_t);
	}
	static void to_lamps(const race& r, renderable& r_t) {
		to_stick_object(r.lamps(), r_t);
	}




	static void to_heightfield(const race& r, renderable& r_hf) {
		std::vector<unsigned int > buffer_id;
		const unsigned int& Z =static_cast<unsigned int>(r.ter().size_pix[1]);
		const unsigned int& X =static_cast<unsigned int>(r.ter().size_pix[0]);

		terrain ter = r.ter();

		std::vector<float>   hf3d;
		for (unsigned int iz = 0; iz < Z; ++iz)
			for (unsigned int ix = 0; ix < X; ++ix) {
				hf3d.push_back(ter.rect_xz[0] + (ix / float(X)) * ter.rect_xz[2]);
				hf3d.push_back(r.ter().hf(ix, iz));
				hf3d.push_back(ter.rect_xz[1] + (iz / float(Z)) * ter.rect_xz[3]);
			}

		for (unsigned int iz = 0; iz < Z-1; ++iz)
			for (unsigned int ix = 0; ix < X-1; ++ix) {
				
				buffer_id.push_back((iz * X) + ix);
				buffer_id.push_back((iz * X) + ix + 1);
				buffer_id.push_back((iz + 1) * X + ix + 1);

				buffer_id.push_back((iz * X) + ix);
				buffer_id.push_back((iz + 1) * X + ix + 1);
				buffer_id.push_back((iz + 1) * X + ix);
			}

		std::vector<glm::vec3> normals_accum(X * Z, glm::vec3(0.0f));
		auto get_vertex = [&](unsigned int idx) {
			return glm::vec3(hf3d[3 * idx], hf3d[3 * idx + 1], hf3d[3 * idx + 2]);
		};

		for (size_t i = 0; i < buffer_id.size(); i += 3) {
			unsigned int i0 = buffer_id[i];
			unsigned int i1 = buffer_id[i + 1];
			unsigned int i2 = buffer_id[i + 2];

			glm::vec3 v0 = get_vertex(i0);
			glm::vec3 v1 = get_vertex(i1);
			glm::vec3 v2 = get_vertex(i2);

			glm::vec3 face_normal = glm::cross(v2 - v0, v1 - v0);
			normals_accum[i0] += face_normal;
			normals_accum[i1] += face_normal;
			normals_accum[i2] += face_normal;
		}

		std::vector<float> normals(X * Z * 3);
		for (unsigned int i = 0; i < X * Z; ++i) {
			glm::vec3 n = (glm::length(normals_accum[i]) > 0.0f)
				? glm::normalize(normals_accum[i])
				: glm::vec3(0.0f, 1.0f, 0.0f);
			normals[3 * i]     = n.x;
			normals[3 * i + 1] = n.y;
			normals[3 * i + 2] = n.z;
		}

		r_hf.add_vertex_attribute<float>(&hf3d[0], X * Z * 3, 0, 3);
		r_hf.add_vertex_attribute<float>(&normals[0], X * Z * 3, 2, 3);
		r_hf.add_indices<unsigned int>(&buffer_id[0], static_cast<unsigned int>(buffer_id.size()), GL_TRIANGLES);
	}

};