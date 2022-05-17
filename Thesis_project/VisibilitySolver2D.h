#pragma once
#include "common.h"
#include "algebra.h"

namespace edt
{
	struct MeshInstance;
	struct Vector;

	class VisibilitySolver2D
	{
	public:
		VisibilitySolver2D(); //constructor
		~VisibilitySolver2D(); //destructor -called when the object gets destroyed

		//functions
		void init();
		void runVisibilityTest(std::vector<MeshInstance*>& walls, std::vector<MeshInstance*>& machines,
			const Vector& camera_position);

	private:
		bool testMachineVisibility(MeshInstance* machine);
	};
}
#pragma once
