#include "mesh3dsobject.h"

#include <iostream>
using namespace std;

Mesh3DSObject::Mesh3DSObject(const string &nname , Model3DSChunk c)
: name(nname)
{
	for(Model3DSChunk cc = c.Child() ; cc ; cc = cc.Sibling())
	{
		switch(cc.ID())
		{
			case(0x4160):
				ParseLocalCoordinateSystem(cc);
			break;

			case(0x4110):
				ParseVertices(cc);
			break;

			case(0x4140):
				ParseTextureCoords(cc);
			break;

			case(0x4120):
				ParseFaces(cc);
			break;

			default:
				cout << "\t\terror : unknown chunk " << cc.ID() << "\n";
			break;
		}
	}
}
Mesh3DSObject::Mesh3DSObject(const Mesh3DSObject &mesh)
{
	operator=(mesh);
}
Mesh3DSObject::~Mesh3DSObject()
{
}

void Mesh3DSObject::operator=(const Mesh3DSObject &mesh)
{
	name = mesh.name;
	//F32 matrix[4][4];

	vertices = mesh.vertices;
	texturecoords = mesh.texturecoords;
	faces = mesh.faces;
	material_faces = mesh.material_faces;
}

void Mesh3DSObject::ParseLocalCoordinateSystem(Model3DSChunk c)
{
	// bottom row should be (0 , 0 , 0 , 1)

	// populate matrix
	for(int i = 0 ; i < 4 ; i++)
	{
		for(int j = 0 ; j < 3 ; j++)
		{
			matrix.m[i][j] = c.Float();
		}
	}

	// output matrix to the screen
	for(int j = 0 ; j < 3 ; j++)
	{
		cout << "\t\t\t(";
		for(int i = 0 ; i < 4 ; i++)
		{
			cout << matrix.m[i][j];
			
			if(i < 3)
				cout << " , ";
		}
		cout << ")\n";
	}

	matrix.m[0][3] = 0;
	matrix.m[1][3] = 0;
	matrix.m[2][3] = 0;
	matrix.m[3][3] = 1;
	cout << "\t\t\t(0 , 0 , 0 , 1)\n";
}
void Mesh3DSObject::ParseVertices(Model3DSChunk c)
{
	int n_vertices = c.Short();

	Mesh3DSVertex v;

	cout << dec << "\t\t\tn_vertices = (" << n_vertices << ")\n";
	for(int i = 0 ; i < n_vertices ; i++)
	{
		v.x = c.Float();
		v.y = c.Float();
		v.z = c.Float();
		
		vertices.push_back(v);

		cout << "\t\t\tv[" << i << "] = ("
			<< v.x << "," << v.y << "," << v.z << ")\n";
	}
}
void Mesh3DSObject::ParseTextureCoords(Model3DSChunk c)
{
	int n_texcoords = c.Short();
	
	Mesh3DSTextureCoord texcoord;

	cout << dec << "\t\t\tn_texcoords = (" << n_texcoords << ")\n";
	for(int i = 0 ; i < n_texcoords ; i++)
	{
		texcoord.u = c.Float();
		texcoord.v = c.Float();
		
		texturecoords.push_back(texcoord);
		
		cout << "\t\t\ttexcoord[" << i << "] = ("
			<< texcoord.u << "," << texcoord.v << ")\n";
	}
}
void Mesh3DSObject::ParseFaces(Model3DSChunk c)
{
	int n_faces = c.Short();

	Mesh3DSFace face;

	cout << dec << "\t\t\tn_faces = (" << n_faces << ")\n";
	for(int i = 0 ; i < n_faces ; i++)
	{
		face.a = c.Short();
		face.b = c.Short();
		face.c = c.Short();
		c.Short();	// read the crappy flag

		faces.push_back(face);

		cout << "\t\t\tface[" << i << "] = ("
			<< face.a << "," << face.b << "," << face.c << ")\n";
	}

	for(Model3DSChunk cc = c.Child() ; cc ; cc = cc.Sibling())
	{
		switch(cc.ID())
		{
			case(0x4130):
				ParseFacesMaterials(cc);
			break;

			default:
			break;
		}
	}
}

void Mesh3DSObject::ParseFacesMaterials(Model3DSChunk c)
{
	string material_name = c.Str();
	vector<int> faces_applied;

	cout << "\t\t\t\tmaterial name = (" << material_name << ")\n";

	int n_faces = c.Short();
	for(int i = 0 ; i < n_faces ; i++)
	{
		int f = c.Short();
		
		faces_applied.push_back(f);
		
		cout << "\t\t\t\tmaterial assigned to face " << f << "\n";
	}

	material_faces[material_name] = faces_applied;
}

const std::string &Mesh3DSObject::Name()
{
	return name;
}
Mesh3DSMatrix Mesh3DSObject::Matrix()
{
	return matrix;
}

const std::vector<Mesh3DSVertex> &Mesh3DSObject::Vertices()
{
	return vertices;
}
const std::vector<Mesh3DSTextureCoord> &Mesh3DSObject::TextureCoords()
{
	return texturecoords;
}
const std::vector<Mesh3DSFace> &Mesh3DSObject::Faces()
{
	return faces;
}
const std::map<std::string , std::vector<int> > &Mesh3DSObject::Materials()
{
	return material_faces;
}
