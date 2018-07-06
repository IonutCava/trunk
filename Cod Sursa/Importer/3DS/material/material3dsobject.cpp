#include "material3dsobject.h"

using namespace std;

Material3DSObject::Material3DSObject(const string &nname)
: name(nname)
{
}
Material3DSObject::~Material3DSObject()
{
}

const std::string &Material3DSObject::Name()
{
	return name;
}
const std::string &Material3DSObject::Texture()
{
	return name;
}
