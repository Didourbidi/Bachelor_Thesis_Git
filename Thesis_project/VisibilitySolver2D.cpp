#include "VisibilitySolver2D.h"
#include "Renderer.h"
#include <assert.h>

namespace edt
{

	VisibilitySolver2D::VisibilitySolver2D() //constructor definition
	{

	}

	VisibilitySolver2D::~VisibilitySolver2D() {} //destructor definition

	void VisibilitySolver2D::init()
	{

	}

	void VisibilitySolver2D::runVisibilityTest(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines,
		const Vector& camera_position)
	{
	
	}

	bool VisibilitySolver2D::testMachineVisibility(MeshInstance* machine)
	{
		return false;
	}

}