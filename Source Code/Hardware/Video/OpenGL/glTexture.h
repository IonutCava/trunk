#ifndef _GL_TEXTURE_H_
#define _GL_TEXTURE_H_

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"
#include "../Texture.h"
#include "TextureManager/ImageTools.h"

class glTexture : public Texture
{

public:
	glTexture(U32 type, bool flipped = false) : Texture(flipped), _type(type) {}
	~glTexture() {}

	bool load(const std::string& name);
	bool unload() {Destroy(); return true;}

	void Bind(U16 slot) const;
	void Unbind(U16 slot) const;

	void SetMatrix(U16 slot, const mat4& transformMatrix);
	void RestoreMatrix(U16 slot);

	void LoadData(U32 target, U8* ptr, U16& w, U16& h, U8 d);

private:

	void Bind() const;
	void Unbind() const;
	void Destroy();
	void Gen();

private:
	U32 _type;
};

#endif