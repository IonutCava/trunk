#include "Headers/NavigationMeshLoader.h"

namespace Navigation {

	NavigationMeshLoader::NavigationMeshLoader(){
	}

	NavigationMeshLoader::~NavigationMeshLoader(){
	}
	       
	char* NavigationMeshLoader::parseRow(char* buf, char* bufEnd, char* row, I32 len){

		bool cont = false;
		bool start = true;
		bool done = false;
		I32 n = 0;

		while (!done && buf < bufEnd)   {
			char c = *buf;
			buf++;
			// multirow
			switch (c)  {
				case '\\':
					cont = true; // multirow
					break;
				case '\n':
					if (start) break;
					done = true;
					break;
				case '\r':
					break;
				case '\t':
				case ' ':
					if (start) break;
				default:
					start = false;
					cont = false;
					row[n++] = c;
					if (n >= len-1)
						done = true;
					break;
			}
		}
		row[n] = '\0';
		return buf;
	}

	I32 NavigationMeshLoader::parseFace(char* row, I32* data, I32 n, I32 vcnt) {
		I32 j = 0;
		while (*row != '\0')  {
			// Skip initial white space
			while (*row != '\0' && (*row == ' ' || *row == '\t'))
				row++;
			char* s = row;
			// Find vertex delimiter and terminated the string there for conversion.
			while (*row != '\0' && *row != ' ' && *row != '\t')  {
				if (*row == '/') *row = '\0';
				row++;
			}
			if (*s == '\0')
				continue;
			I32 vi = atoi(s);
			data[j++] = vi < 0 ? vi+vcnt : vi-1;
			if (j >= n) return j;
		}
		return j;
	}

	NavModelData NavigationMeshLoader::loadMeshFile(const char* filename) {

		NavModelData modelData;

		char* buf = 0;
		FILE* fp = fopen(filename, "rb");
		if (!fp)
			return modelData;
		fseek(fp, 0, SEEK_END);
		I32 bufSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buf = new char[bufSize];
		if (!buf)  {
			fclose(fp);
			return modelData;
		}
		fread(buf, bufSize, 1, fp);
		fclose(fp);

		char* src = buf;
		char* srcEnd = buf + bufSize;
		char row[512];
		I32 face[32];
		F32 x,y,z;
		I32 nv;
		//I32 vcap = 0;
		//I32 tcap = 0;
	    
		while (src < srcEnd)   {
			// Parse one row
			row[0] = '\0';
			src = parseRow(src, srcEnd, row, sizeof(row)/sizeof(char));
			// Skip comments
			if (row[0] == '#') continue;
			if (row[0] == 'v' && row[1] != 'n' && row[1] != 't') {
				// Vertex pos
				sscanf(row+1, "%f %f %f", &x, &y, &z);
				addVertex(&modelData, x, y, z);
			}
			if (row[0] == 'f')  {
				// Faces
				nv = parseFace(row+1, face, 32, modelData.vert_ct);
				for (I32 i = 2; i < nv; ++i)
				{
					const I32 a = face[0];
					const I32 b = face[i-1];
					const I32 c = face[i];
					if (a < 0 || a >= (I32)modelData.vert_ct || b < 0 || b >= (I32)modelData.vert_ct || c < 0 || c >= (I32)modelData.vert_ct)
						continue;
					addTriangle(&modelData, a, b, c);
				}
			}
		}

		SAFE_DELETE_ARRAY(buf);

		// Calculate normals.
		modelData.normals = new F32[modelData.tri_ct*3];
		for (I32 i = 0; i < (I32)modelData.tri_ct*3; i += 3)   {
			const F32* v0 = &modelData.verts[modelData.tris[i]*3];
			const F32* v1 = &modelData.verts[modelData.tris[i+1]*3];
			const F32* v2 = &modelData.verts[modelData.tris[i+2]*3];
			F32 e0[3], e1[3];
			for (I32 j = 0; j < 3; ++j)  {
				e0[j] = v1[j] - v0[j];
				e1[j] = v2[j] - v0[j];
			}
			F32* n = &modelData.normals[i];
			n[0] = e0[1]*e1[2] - e0[2]*e1[1];
			n[1] = e0[2]*e1[0] - e0[0]*e1[2];
			n[2] = e0[0]*e1[1] - e0[1]*e1[0];
			F32 d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
			if (d > 0)  {
				d = 1.0f/d;
				n[0] *= d;
				n[1] *= d;
				n[2] *= d;
			}
		}
	    
		return modelData;
	}

	NavModelData NavigationMeshLoader::mergeModels(NavModelData a, NavModelData b, bool delOriginals/* = false*/) {

		NavModelData mergedData;
		if(a.verts || b.verts)  {
			if(!a.verts) {
				return b;
			}else if (! b.verts ) {
				return a;
			}
	        
			mergedData.clear();

			I32 totalVertCt = (a.vert_ct + b.vert_ct);
			I32 newCap = 8;

			while(newCap < totalVertCt)
				newCap *= 2;

			mergedData.verts = new F32[newCap*3];
			mergedData.vert_cap = newCap;
			mergedData.vert_ct = totalVertCt;
			memcpy(mergedData.verts,               a.verts, a.vert_ct*3*sizeof(F32));
			memcpy(mergedData.verts + a.vert_ct*3, b.verts, b.vert_ct*3*sizeof(F32));

			I32 totalTriCt = (a.tri_ct + b.tri_ct);
			newCap = 8;

			while(newCap < totalTriCt)
				newCap *= 2;

			mergedData.tris = new I32[newCap*3];
			mergedData.tri_cap = newCap;
			mergedData.tri_ct = totalTriCt;
			I32 aFaceSize = a.tri_ct * 3;
			memcpy(mergedData.tris, a.tris, aFaceSize*sizeof(I32));
	        
			I32 bFaceSize = b.tri_ct * 3;
			I32* bFacePt = mergedData.tris + a.tri_ct * 3;// i like pointing at faces
			memcpy(bFacePt, b.tris, bFaceSize*sizeof(I32));
	        
	        
			for(U32 i = 0; i < (U32)bFaceSize;i++)
				*(bFacePt + i) += a.vert_ct;

			if(mergedData.vert_ct > 0) {
				if(delOriginals)  {
					a.clear();
					b.clear();
				}
			}else
				mergedData.clear();
		}
		return mergedData;
	}

	bool NavigationMeshLoader::saveModelData(NavModelData data, const char* filename, const char* activeScene/* = NULL*/) {
		if( ! data.getVertCount() || ! data.getTriCount())
			return false;
	    
		std::string path("navigationCache/");
		std::string filenamestring(filename);
		//CreatePath //IsDirectory

		if(activeScene) {
			path +=  std::string(activeScene);
	        
		}

		path += std::string("/") + filenamestring + std::string(".obj");
		/// Create the file if it doesn't exists
		FILE *fp = NULL;
		fopen_s(&fp,path.c_str(),"w");
		fclose(fp);

		std::ofstream myfile;
		myfile.open(path.c_str());
		if(!myfile.is_open())
			return false;

		F32* vstart = data.verts;
		I32* tstart = data.tris;

		for(U32 i = 0; i < data.getVertCount(); i++)  {
			F32* vp = vstart + i*3;
			myfile << "v " << (*(vp)) << " " << *(vp+1) << " " << (*(vp+2)) << "\n";
		}

		for(U32 i = 0; i < data.getTriCount(); i++)  {
			I32* tp = tstart + i*3;
			myfile << "f " << *(tp) << " " << *(tp+1) << " " << *(tp+2) << "\n";
		}

		myfile.close();
		return true;
	}

	void NavigationMeshLoader::addVertex(NavModelData* modelData, F32 x, F32 y, F32 z){

		if (modelData->vert_ct+1 > modelData->vert_cap)  {
			modelData->vert_cap = ! modelData->vert_cap ? 8 : modelData->vert_cap*2;
			F32* nv = new F32[modelData->vert_cap*3];
			if (modelData->vert_ct)
				memcpy(nv, modelData->verts, modelData->vert_ct*3*sizeof(F32));
			if( modelData->verts )
				delete [] modelData->verts;
			modelData->verts = nv;
		}

		F32* dst = &modelData->verts[modelData->vert_ct*3];
		*dst++ = x;
		*dst++ = y;
		*dst++ = z;
		modelData->vert_ct++;
	}

	void NavigationMeshLoader::addTriangle(NavModelData* modelData,I32 a, I32 b, I32 c){

		if (modelData->tri_ct+1 > modelData->tri_cap)  {
			modelData->tri_cap = !modelData->tri_cap ? 8 : modelData->tri_cap*2;
			I32* nv = new int[modelData->tri_cap*3];
			if (modelData->tri_ct)
				memcpy(nv, modelData->tris, modelData->tri_ct*3*sizeof(int));
			if( modelData->tris )
				delete [] modelData->tris;
			modelData->tris = nv;
		}

		I32* dst = &modelData->tris[modelData->tri_ct*3];
		*dst++ = a;
		*dst++ = b;
		*dst++ = c;
		modelData->tri_ct++;
	}

	NavModelData NavigationMeshLoader::parseTerrainData(BoundingBox box, SceneNode* set/* = NULL*/, I32* count/* = NULL*/){
		if(set->getType() != TYPE_TERRAIN){
			PRINT_FN("WARNING! Tried to parse node [ %s ] as terrain for NavigationMesh generation!",set->getName().c_str());
			NavModelData data;
			data.clear(true);
			return data;
		}
		return parse(box, (TYPE_TERRAIN), DETAIL_ABSOLUTE, set, count);
	}

	NavModelData NavigationMeshLoader::parseStaticObjects(BoundingBox box, SceneNode* set/* = NULL*/, I32* count/* = NULL*/){
		if(set->getType() != TYPE_WATER || set->getType() == TYPE_TERRAIN || set->getType() == TYPE_OBJECT3D){
			PRINT_FN("WARNING! Tried to parse node [ %s ] as static object for NavigationMesh generation!",set->getName().c_str());
			NavModelData data;
			data.clear(true);
			return data;
		}
		return parse(box, (TYPE_WATER), DETAIL_ABSOLUTE, set, count);
	}

	NavModelData NavigationMeshLoader::parseDynamicObjects(BoundingBox box, SceneNode* set/* = NULL*/, I32* count/* = NULL*/){
		if(set->getType() != TYPE_OBJECT3D || set->getType() == TYPE_TERRAIN || set->getType() == TYPE_WATER){
			PRINT_FN("WARNING! Tried to parse node [ %s ] as terrain for NavigationMesh generation!",set->getName().c_str());
			NavModelData data;
			data.clear(true);
			return data;
		}
		return parse(box, (TYPE_OBJECT3D), DETAIL_BOUNDINGBOX, set, count);
	}


	static const vec3<F32> cubePoints[8] = {

	   vec3<F32>(-1.0f, -1.0f, -1.0f), vec3<F32>(-1.0f, -1.0f,  1.0f), vec3<F32>(-1.0f,  1.0f, -1.0f), vec3<F32>(-1.0f,  1.0f,  1.0f),
	   vec3<F32>( 1.0f, -1.0f, -1.0f), vec3<F32>( 1.0f, -1.0f,  1.0f), vec3<F32>( 1.0f,  1.0f, -1.0f), vec3<F32>( 1.0f,  1.0f,  1.0f)
	};

	static const U32 cubeFaces[6][4] = {

	   { 0, 4, 6, 2 }, { 0, 2, 3, 1 }, { 0, 1, 5, 4 },
	   { 3, 2, 6, 7 }, { 7, 6, 4, 5 }, { 3, 7, 5, 1 }
	};

	NavModelData NavigationMeshLoader::parse(BoundingBox box, U32 mask, MESH_DETAIL_LEVEL level, SceneNode* set/* = NULL*/, I32* count /*= NULL*/) {

		if(count)
			*count = 0;
	    
		NavModelData data;
	    

		data.clear(false);

		U32 numVert = 1;

		unordered_map<U32, vec3<F32> > globalPointIdxMap;
		unordered_map<U32, U32> translationMap;
		std::vector<vec3<F32> > vertexVector;
		std::vector<vec3<I32> > faceVector;

		PRINT_FN("WARNING! NavigationMesh Generation is not yet implemented!");

		return data;
	}

};