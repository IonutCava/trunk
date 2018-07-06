#ifndef _GL_TEXTURE_H_
#define _GL_TEXTURE_H_

#include "resource.h"
#include "Utility/Headers/BaseClasses.h"
#include "../Texture.h"
#include "TextureManager/ImageTools.h"

class glTexture : public Texture
{

public:
	glTexture(U32 type, bool flipped = false) : Texture(), _type(type), _flipped(flipped) {}
	~glTexture() {Destroy();}

	bool load(const std::string& name);
	bool unload() {Destroy(); return true;}

	void Bind(U32 slot) const;
	void Unbind(U32 slot) const;

	void SetMatrix(U32 slot, const mat4& transformMatrix);
	void RestoreMatrix(U32 slot);

	void LoadData(U32 target, U8* ptr, U32& w, U32& h, U32 d);

private:

	void Bind() const;
	void Unbind() const;
	void Destroy();
	void Gen();

private:
	bool _flipped;
	U32 _type;
};

#endif