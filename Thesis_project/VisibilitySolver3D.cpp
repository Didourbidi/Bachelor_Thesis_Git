#include "VisibilitySolver3D.h"
#include "Renderer.h"
#include <assert.h>
#include <chrono>
#include <functional>

namespace edt
{
	
	VisibilitySolver3D::VisibilitySolver3D() : //constructor definition
		render_target(0),
		depth_buffer(0),
		frame_buffer(0),
		renderer(nullptr),
		nr_workers(4)
	{
		workers.resize( nr_workers );
		draw_buffers[0] = GL_COLOR_ATTACHMENT0;
		createProjectionMatrix();
		createViewMatrices();
		static_assert(sizeof(VisibilitySolver3D::Pixel) == sizeof(PIXEL_TYPE) && alignof(VisibilitySolver3D::Pixel) == alignof(PIXEL_TYPE));
	}

	VisibilitySolver3D::~VisibilitySolver3D(){} //destructor definition

	void VisibilitySolver3D::init(Renderer* renderer, int resolution)
	{
		this->resolution = resolution;
		this->renderer = renderer;

		createRenderTargets();
	}

	void VisibilitySolver3D::runVisibilityTest(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines, 
		const Vector& camera_position)
	{
		//-------------------------------
		//profile rendering - start
		auto profile_start = std::chrono::high_resolution_clock::now();
		//-------------------------------

		Matrix translation_matrix = create_Translation_mat(- camera_position);
		renderer->setProjectionMatrix(projection_matrix); //for each visibility test we have the same matrix
		glViewport(0, 0, resolution, resolution);

		//use custom render target (texture instead of back buffer)
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
		//use custom depth buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
		glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS); //when there are two surfaces at the same depth only the new one is drawn
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_target, 0);

		std::vector<Vector> vectors_to_machine_roots(machines.size());
		std::vector<Matrix> instance_transforms(machines.size()); //it is harcoded to 255 in the shader (OpenGL version limit)
		std::vector<size_t> frustum_collected(machines.size());

		for (size_t i=0; i<machines.size(); i++)
		{
			MeshInstance* machine = machines[i];
			vectors_to_machine_roots[i] = Normalize(machine->translation - camera_position);
			machine->is_visible = false;
		}

		texture_data.resize(resolution * resolution, { 0 });

		//-------------------------------
		//profile rendering - preparation
		auto profile_init = std::chrono::high_resolution_clock::now();
		//-------------------------------
		std::chrono::duration<double> profile_render_total(0); 
		std::chrono::duration<double> profile_visibility_test_total(0);
		//------------------------------

		for (View& view : round_view)
		{
			//-------------------------------
			//profile rendering - frustum start
			auto profile_frustum_start = std::chrono::high_resolution_clock::now();
			//-------------------------------

			Matrix view_mat = updateViewMatrix(view.mat, translation_matrix); //update matrices as we iterate them
			renderer->setViewMatrix(view_mat);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderer->setActiveShader(machines.front()->model->material.shader_id);

			size_t nr_frustum_collected = 0;
			for (size_t i = 0; i < machines.size(); i++)
				if (testFrustum(machines[i], view.fwd, vectors_to_machine_roots[i]))
				{
					frustum_collected[ nr_frustum_collected ] = i;
					::memcpy(instance_transforms.data() + nr_frustum_collected++, &machines[i]->transform, sizeof(Matrix));
				}

			const size_t instance_count = 255;
			for (size_t i = 0; i < nr_frustum_collected; i+= instance_count) //255 is the limit for opengl instancing
			{
				int nr_of_machines = instance_count;
				if (i + nr_of_machines > (int)nr_frustum_collected)
					nr_of_machines = nr_frustum_collected - i;

				renderer->send_int_to_shader("globals.instance_offset", (int)i);
				renderer->send_matrices_to_shader("model_matrix", instance_transforms.data() + i, nr_of_machines);
				renderer->renderMeshInstances(*machines.front()->model, nr_of_machines);
			}	
			
#if 0
			//for wall occlusion
			if( !walls.empty() )
				renderer->setActiveShader(walls.front()->model->material.shader_id);

			for (MeshInstance* wall : walls)
			{
				renderer->renderMesh(*wall);
			}
#endif
			//-------------------------------
			//profile rendering - depth buffer
			auto profile_render = std::chrono::high_resolution_clock::now();
			profile_render_total += (profile_render - profile_frustum_start);
			//-------------------------------

			fetchRenderTargetData( &view != &round_view[0]);
			testVisibilities(machines, frustum_collected);

			//-------------------------------
			//profile rendering - visibility test
			auto profile_visibility_test = std::chrono::high_resolution_clock::now();
			profile_visibility_test_total += (profile_visibility_test - profile_render);
			//-------------------------------
			//glFlush();
		}

		for (size_t i = 0; i < nr_workers; i++)
			if (workers[i].joinable())
				workers[i].join();

		//-------------------------------
		auto profile_end = std::chrono::high_resolution_clock::now();

		size_t nr_visible_machines = 0;
		for (MeshInstance* machine : machines)
			if (machine->is_visible)
				nr_visible_machines++;

		print("number of visible machines: " << nr_visible_machines << std::endl);
		print("profile_init: " << std::chrono::duration_cast<std::chrono::microseconds>(profile_init - profile_start).count() / 1000.0f
			<< std::endl);
		print("profile_render_total: " << std::chrono::duration_cast<std::chrono::microseconds>(profile_render_total).count()/1000.0f 
			<< std::endl);
		print("profile_visibility_test_total: " << std::chrono::duration_cast<std::chrono::microseconds>(profile_visibility_test_total).count() / 1000.0f
			<< std::endl << std::endl);
		print("test_total: " << std::chrono::duration_cast<std::chrono::microseconds>(profile_end - profile_start).count() / 1000.0f << std::endl << std::endl);

		//-------------------------------

	}

	void VisibilitySolver3D::fetchRenderTargetData( bool wait_workers )
	{
		glActiveTexture(GL_TEXTURE0);
		glGenerateMipmap(GL_TEXTURE_2D); //auto-generate mipmap
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, render_target);

		if(wait_workers)
			for (size_t i = 0; i < nr_workers; i++)
				if(workers[i].joinable() )
					workers[i].join();

		glGetTexImage(GL_TEXTURE_2D,
			0,
			RENDER_TARGET_PIXEL_FORMAT, // format
			RENDER_TARGET_PIXEL_DATA_TYPE, // type
			texture_data.data());
	}

	void VisibilitySolver3D::createRenderTargets()
	{
		glGenFramebuffers(1, &frame_buffer);

		{
			// The texture we're going to render to
			glGenTextures(1, &render_target);

			// "Bind" the newly created texture : all future texture functions will modify this texture
			glBindTexture(GL_TEXTURE_2D, render_target);

			// Give an empty image to OpenGL ( the last "0" )
			glTexImage2D(GL_TEXTURE_2D, 0, 
				RENDER_TARGET_INTERNAL_FORMAT, // internalformat
				resolution, resolution, 0, 
				RENDER_TARGET_PIXEL_FORMAT, // format
				RENDER_TARGET_PIXEL_DATA_TYPE, // type
				0);
			glGenerateMipmap(GL_TEXTURE_2D);

			// Poor filtering. Needed !
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}

		// The depth buffer
		glGenRenderbuffers(1, &depth_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, resolution, resolution);
	}

	void VisibilitySolver3D::createViewMatrices()
	{
		// all angles are negated to create view matrix.
		round_view[0] = { create_Scaling_mat({1.0f, 1.0f, 1.0f}), {0,0,-1} }; // forward. 0 rotation.
		round_view[1] = { create_Rotation_ymat(90), {1,0,0} }; // right. 90 around y.
		round_view[2] = { create_Rotation_ymat(180), {0,0,1} }; // back. 180 around y.
		round_view[3] = { create_Rotation_ymat(-90), {-1,0,0} }; // left. -90 around y.
		round_view[4] = { create_Rotation_xmat(-90), {0,1,0} }; // up. -90 around x.
		round_view[5] = { create_Rotation_xmat(90), {0,-1,0} }; // down. 90 around x.

		for (View& view : round_view)
		{
			view.mat.e[15] = 1.0f; //w has to always be 1
		}
	}

	Matrix VisibilitySolver3D::updateViewMatrix(const Matrix& mat, const Matrix& translation_matrix)
	{
		return MatMatMul(mat, translation_matrix);
	}

	void VisibilitySolver3D::createProjectionMatrix()
	{
		projection_matrix = construct_perspective_matrix(90.0f, 0.1f, 1000.0f, 1.0f);
	}

	bool VisibilitySolver3D::testFrustum(MeshInstance* machine, const Vector& cam_fwd, const Vector& dir_to_root)
	{
		static const float min_cos = 0.607106f;
		return DotProduct(dir_to_root, cam_fwd) > min_cos;
	}
	
	void VisibilitySolver3D::test_pixels_interleaved(const std::vector<MeshInstance*>& machines, const size_t& interleave, 
		const std::vector<size_t>& index_lookup)
	{
		//Sleep(1);
		size_t nr_pixels = texture_data.size();
		for (size_t i_pixel = interleave; i_pixel < nr_pixels; i_pixel += nr_workers)
			if (texture_data[i_pixel].r)
			{
				size_t instance_index = (size_t)(texture_data[i_pixel].r - 1);
				machines[index_lookup[instance_index]]->is_visible = true;
			}
	}

	void VisibilitySolver3D::testVisibilities(const std::vector<MeshInstance*>& machines, const std::vector<size_t>& index_lookup)
	{
		#if 0
			for (size_t i=0; i< nr_workers; i++)
				workers[i] = std::thread(std::bind(&VisibilitySolver3D::test_pixels_interleaved, this, machines, i, index_lookup) );
		#else

			for (const Pixel& pixel : texture_data)
				if( pixel.r )
				{
					size_t instance_index = (size_t)(pixel.r - 1);
					machines[ index_lookup[instance_index ] ]->is_visible = true;
				}

		#endif
	}
}