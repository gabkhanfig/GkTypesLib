#include "job_system.h"

#if GK_TYPES_LIB_TEST

using JobSystem = gk::JobSystem;

test_case("ConstructDestruct") {
	JobSystem* jobSystem = new JobSystem(2);
	delete jobSystem;
}

#endif