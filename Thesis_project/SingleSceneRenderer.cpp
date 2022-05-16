#include "SingleSceneRenderer.h"
#include "Renderer.h"
#include <assert.h>
#include "Camera.h"

namespace edt
{
	SingleSceneRenderer::SingleSceneRenderer() : //constructor definition
		renderer(nullptr),
		camera(nullptr)
	{
		back_buffer_resolution[0] = 0;
		back_buffer_resolution[1] = 0;
	}

	SingleSceneRenderer::~SingleSceneRenderer(){} //destructor definition

	void SingleSceneRenderer::init(Renderer* renderer, const int& resolution_x, const int& resolution_y, const Camera* camera)
	{
		back_buffer_resolution[0] = resolution_x;
		back_buffer_resolution[1] = resolution_y;
		this->camera = camera;
		this->renderer = renderer;
	}

	void SingleSceneRenderer::drawScene(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines, 
		std::vector<MeshInstance*>& machines_monochrome)
	{
		renderer->setViewMatrix(camera->get_view_matrix());
		renderer->setProjectionMatrix(camera->get_projection_matrix());

		glViewport(0, 0, back_buffer_resolution[0], back_buffer_resolution[1]);

		// Undo the settings of the visibility test.
		glDrawBuffer(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glDepthMask(GL_TRUE); //enable to write to depth buffer

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		//Z BUFFER
		//enable zbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		//Compare depth
		glDepthFunc(GL_LESS);

		if (!walls.empty())
		{
			renderer->setActiveShader(walls.front()->model->material.shader_id);		
			renderer->send_float3_to_shader("material.color", { 0.6f , 0.6f , 0.6f });
			
			for (MeshInstance* mesh_instance : walls)
				renderer->renderMesh(*mesh_instance);	
		}

		if (!machines.empty())
		{
			renderer->setActiveShader(machines.front()->model->material.shader_id);

			Vector visible_color = { 0.0f , 1.0f , 0.0f }, obscured_color = { 1.0f , 0.0f , 0.0f };
			for (int i = 0; i < (int)machines.size(); i++)
			{
				MeshInstance* mesh_instance = machines[i];
				renderer->send_float3_to_shader("material.color", machines_monochrome[i]->is_visible ? visible_color : obscured_color);
				renderer->renderMesh(*mesh_instance);
			}
		}

		glFlush();
	}
}