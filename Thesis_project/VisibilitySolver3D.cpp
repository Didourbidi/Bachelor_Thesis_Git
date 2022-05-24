#include "VisibilitySolver3D.h"
#include "Renderer.h"
#include <assert.h>

namespace edt
{
	
	VisibilitySolver3D::VisibilitySolver3D() : //constructor definition
		render_target(0),
		depth_buffer(0),
		frame_buffer(0),
		renderer(nullptr),
		mip_level(0),
		last_mip_resolution(LAST_MIP_RESOLUTION)
	{
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

		calculateLastMipLevel();
		createRenderTargets();
	}

	void VisibilitySolver3D::runVisibilityTest(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines, 
		const Vector& camera_position)
	{
		Matrix translation_matrix = create_Translation_mat(- camera_position);
		renderer->setProjectionMatrix(projection_matrix); //for each visibility test we have the same matrix

		//use custom render target (texture instead of back buffer)
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
		//use custom depth buffer
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
		glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers
		glDepthFunc(GL_LESS); //when there are two surfaces at the same depth only the new one is drawn

		int number_of_non_visible_machines = machines.size(); //there is no point to run the test if all machines are visible
		std::vector<Matrix> instance_transforms(machines.size()); //it is harcoded to 1000 in the shader
		//assert(machines.size() <= instance_transforms.size());

		for (size_t i=0; i<machines.size(); i++)
		{
			MeshInstance* machine = machines[i];
			::memcpy(instance_transforms.data() + i, &machine->transform, sizeof(Matrix));
			machine->is_visible = false;
		}

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_target, 0);
		texture_data.resize(last_mip_resolution * last_mip_resolution, { 0 });

		for (Matrix& rotation_mat : round_view)
		{
			Matrix view_mat = updateViewMatrix(rotation_mat, translation_matrix); //update matrices as we iterate them
			renderer->setViewMatrix(view_mat);

			glDepthMask(GL_TRUE);
			glClear(GL_DEPTH_BUFFER_BIT);
			//for wall occlusion
			if( !walls.empty() )
				renderer->setActiveShader(walls.front()->model->material.shader_id);

			for (MeshInstance* wall : walls)
			{
				renderer->renderMesh(*wall);
			}

			//for machine occlusion
			if (!machines.empty())
			{
				renderer->setActiveShader(machines.front()->model->material.shader_id);
				for(int i_machine = 0; i_machine < (int)machines.size(); i_machine += 255) //255 is the limit for opengl instancing
				{
					int nr_of_machines = 255;
					if (i_machine + nr_of_machines > (int)machines.size())
						nr_of_machines = machines.size() - i_machine;

					renderer->send_matrices_to_shader("model_matrix", instance_transforms.data() + i_machine, nr_of_machines);
					renderer->renderMeshInstances(*machines.front(), nr_of_machines);
				}
			}

			glClear(GL_COLOR_BUFFER_BIT); //clear only colors
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_FALSE);

			if (!machines.empty())
				renderer->setActiveShader(machines.front()->model->material.shader_id);

			for (MeshInstance* machine : machines)
			{
				if (!testMachineVisibility(machine))
					continue;
			
				if (!--number_of_non_visible_machines)
					break; //stop iterating machines
			}
			if (!number_of_non_visible_machines)
				break; //stop iterating view matrices
			//glFlush();
		}

		print("visible: " << (machines.size() - number_of_non_visible_machines) << std::endl);
	}

	bool VisibilitySolver3D::testMachineVisibility(MeshInstance* machine)
	{
		if (machine->is_visible)
			return false;
		
		renderer->renderMesh(*machine);
		glActiveTexture(GL_TEXTURE0);
		glGenerateMipmap(GL_TEXTURE_2D); //auto-generate mipmap
		/*
		glGetTextureLevelParameteriv(render_targets[0],
			mip_level,
			GLenum pname,
			GLint * params);*/

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, render_target);

		glGetTexImage(GL_TEXTURE_2D,
			mip_level,
			RENDER_TARGET_PIXEL_FORMAT, // format
			RENDER_TARGET_PIXEL_DATA_TYPE, // type
			texture_data.data());

		for (const Pixel& pixel : texture_data)
			if (pixel.r)
			{
				machine->is_visible = true;
				glClear(GL_COLOR_BUFFER_BIT);
				return true;
			}

		return false;
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
		round_view[0] = create_Scaling_mat({1.0f, 1.0f, 1.0f}); // forward. 0 rotation.
		round_view[1] = create_Rotation_ymat( 90 ); // right. 90 around y.
		round_view[2] = create_Rotation_ymat(180); // back. 180 around y.
		round_view[3] = create_Rotation_ymat(-90); // left. -90 around y.
		round_view[4] = create_Rotation_xmat(-90); // up. -90 around x.
		round_view[5] = create_Rotation_xmat(90); // down. 90 around x.

		for (Matrix& mat : round_view)
		{
			mat.e[15] = 1.0f; //w has to always be 1
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

	void VisibilitySolver3D::calculateLastMipLevel()
	{
		int last_mip_size = resolution;
		while (last_mip_size > last_mip_resolution)
		{
			last_mip_size /= 2;
			mip_level++;
		}
	}
}