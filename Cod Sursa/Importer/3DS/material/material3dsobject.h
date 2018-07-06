#ifndef MATERIAL3DSOBJECT_H
#define MATERIAL3DSOBJECT_H

#include <string>

/***/
class Material3DSObject
{
    public:
	/***/
	Material3DSObject(const std::string &nname);
	/***/
	~Material3DSObject();

	const std::string &Name();
	const std::string &Texture();
    private:
	std::string name;
};

#endif
