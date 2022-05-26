#include "PanoramicSceneRenderer.h"
#include "Renderer.h"
#include <assert.h>
#include "Camera.h"

namespace edt
{
	PanoramicSceneRenderer::PanoramicSceneRenderer() : //constructor definition
		renderer(nullptr),
		camera(nullptr)
	{
		back_buffer_resolution[0] = 0;
		back_buffer_resolution[1] = 0;
		viewport_resolution[0] = 0;
		viewport_resolution[1] = 0;
	}

	PanoramicSceneRenderer::~PanoramicSceneRenderer(){} //destructor definition

	void PanoramicSceneRenderer::init(Renderer* renderer, const int& resolution_x, const int& resolution_y, const Camera* camera)
	{
		back_buffer_resolution[0] = resolution_x;
		back_buffer_resolution[1] = resolution_y;
		this->camera = camera;
		this->renderer = renderer;
		createViewports();
	}

	void PanoramicSceneRenderer::drawScene(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines, 
		std::vector<MeshInstance*>& machines_monochrome)
	{
		Camera tmp_camera(*camera);
		tmp_camera.FoV = 90;
		renderer->setProjectionMatrix(tmp_camera.get_projection_matrix());

		// Undo the settings of the visibility test.
		glDrawBuffer(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glDepthMask(GL_TRUE); //enable to write to depth buffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		//Compare depth
		glDepthFunc(GL_LESS);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		for (const Viewport& vp : viewports)
		{
			if (&vp > viewports + 6)
				continue;

			tmp_camera.rotation = camera->rotation;
			tmp_camera.rotate(vp.rotation_adjustment);

			glViewport(vp.x, vp.y, viewport_resolution[0], viewport_resolution[1]);
			renderer->setViewMatrix(tmp_camera.get_view_matrix());

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
		}
		glFlush();
	}

	void PanoramicSceneRenderer::createViewports()
	{
		viewport_resolution[0] = back_buffer_resolution[0] / 4;
		viewport_resolution[1] = back_buffer_resolution[1] / 3;

		/*//top
		viewports[0].x = viewport_resolution[0];
		viewports[0].y = viewport_resolution[1] * 2;
		viewports[0].rotation_adjustment = { 90, 0, 0 };*/

		//left
		viewports[0].x = 0;
		viewports[0].y = viewport_resolution[1];
		viewports[0].rotation_adjustment = { 0, 90, 0 };

		//forward
		viewports[1].x = viewport_resolution[0];
		viewports[1].y = viewport_resolution[1];
		viewports[1].rotation_adjustment = { 0, 0, 0 };

		//right
		viewports[2].x = viewport_resolution[0] * 2;
		viewports[2].y = viewport_resolution[1];
		viewports[2].rotation_adjustment = { 0, -90, 0 };

		//back
		viewports[3].x = viewport_resolution[0] * 3;
		viewports[3].y = viewport_resolution[1];
		viewports[3].rotation_adjustment = { 0, 180, 0 };

		/*//bottom
		viewports[5].x = viewport_resolution[0];
		viewports[5].y = 0;
		viewports[5].rotation_adjustment = { -90, 0, 0 };*/
	}
}