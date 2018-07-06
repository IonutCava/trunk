#ifndef OBJ_IMP_H_
#define OBJ_IMP_H_

#include "resource.h"
#include "Rendering/common.h"


ImportedModel* ReadOBJ(string filename,vec3 position, F32 scale, vec3 rotation, bool vegetation);
ImportedModel* ReadOBJ(string filename,vec3 position, F32 scale, vec3 rotation, bool vegetation, mycallback *call);


#endif



