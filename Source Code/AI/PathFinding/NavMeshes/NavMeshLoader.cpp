#include "Headers/NavMeshLoader.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Water/Headers/Water.h"

#include <fstream>

namespace Navigation {
    namespace NavigationMeshLoader{
	
		static vec3<F32> _minVertValue, _maxVertValue;
		static const U32 cubeFaces[6][4] = {{ 0, 4, 6, 2 }, 
											{ 0, 2, 3, 1 }, 
											{ 0, 1, 5, 4 },
											{ 3, 2, 6, 7 },
											{ 7, 6, 4, 5 },
											{ 3, 7, 5, 1 }};

		NavModelData loadMeshFile(const char* filename) {

			NavModelData modelData;
		
			FILE* fp = fopen(filename, "rb");
			if (!fp) return modelData;

			modelData.setName(filename);
			fseek(fp, 0, SEEK_END);

			I32 bufSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char* buf = new char[bufSize];
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
					addVertex(&modelData, vec3<F32>(x, y, z));
				}
				if (row[0] == 'f')  {
					// Faces
					nv = parseFace(row+1, face, 32, modelData._vertexCount);
					for (I32 i = 2; i < nv; ++i) {
						const I32 a = face[0];
						const I32 b = face[i-1];
						const I32 c = face[i];
						if (a < 0 || a >= (I32)modelData._vertexCount || 
							b < 0 || b >= (I32)modelData._vertexCount || 
							c < 0 || c >= (I32)modelData._vertexCount) continue;

						addTriangle(&modelData, vec3<U32>(a, b, c));
					}
				}
			}

			SAFE_DELETE_ARRAY(buf);

			// Calculate normals.
			modelData._normals = new F32[modelData._triangleCount*3];

			for (I32 i = 0; i < (I32)modelData._triangleCount*3; i += 3)   {
				const F32* v0 = &modelData._vertices[modelData._triangles[i]*3];
				const F32* v1 = &modelData._vertices[modelData._triangles[i+1]*3];
				const F32* v2 = &modelData._vertices[modelData._triangles[i+2]*3];

				F32 e0[3], e1[3];
				for (I32 j = 0; j < 3; ++j)  {
					e0[j] = v1[j] - v0[j];
					e1[j] = v2[j] - v0[j];
				}

				F32* n = &modelData._normals[i];
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

		bool saveMeshFile(const NavModelData& data, const char* filename, const std::string& activeSceneName/* = NULL*/) {
			if(!data.getVertCount() || !data.getTriCount())	return false;

			std::string path(ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/navMeshes/");
			std::string filenamestring(filename);
			//CreatePath //IsDirectory

			if(!activeSceneName.empty()) path +=  activeSceneName;
		
			path += std::string("/") + filenamestring + std::string(".obj");
			// Create the file if it doesn't exists
			FILE *fp = NULL;
			fopen_s(&fp,path.c_str(),"w");
			fclose(fp);

			std::ofstream myfile;
			myfile.open(path.c_str());
			if(!myfile.is_open()) return false;

			F32* vstart = data._vertices;
			I32* tstart = data._triangles;

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

		NavModelData mergeModels(NavModelData& a,NavModelData& b, bool delOriginals/* = false*/) {

			NavModelData mergedData;
			if(a.getVerts() || b.getVerts())  {
				if(!a.getVerts())         return b;
				else if (! b.getVerts() ) return a;
			
				mergedData.clear();

				I32 totalVertCt = (a.getVertCount() + b.getVertCount());
				I32 newCap = 8;

				while(newCap < totalVertCt)	newCap *= 2;

				mergedData._vertices = new F32[newCap*3];
				mergedData._vertexCapacity = newCap;
				mergedData._vertexCount = totalVertCt;

				memcpy(mergedData._vertices,                      a.getVerts(), a.getVertCount()*3*sizeof(F32));
				memcpy(mergedData._vertices + a.getVertCount()*3, b.getVerts(), b.getVertCount()*3*sizeof(F32));

				I32 totalTriCt = (a.getTriCount() + b.getTriCount());
				newCap = 8;

				while(newCap < totalTriCt)	newCap *= 2;

				mergedData._triangles = new I32[newCap*3];
				mergedData._triangleCapacity = newCap;
				mergedData._triangleCount = totalTriCt;
				I32 aFaceSize = a.getTriCount() * 3;
				memcpy(mergedData._triangles, a.getTris(), aFaceSize*sizeof(I32));

				I32 bFaceSize = b.getTriCount() * 3;
				I32* bFacePt = mergedData._triangles + a.getTriCount() * 3;// i like pointing at faces
				memcpy(bFacePt, b.getTris(), bFaceSize*sizeof(I32));

				for(U32 i = 0; i < (U32)bFaceSize;i++)	*(bFacePt + i) += a.getVertCount();

				if(mergedData._vertexCount > 0) {
					if(delOriginals)  {
						a.clear();
						b.clear();
					}
				}else mergedData.clear();
			}

			mergedData.setName(a.getName()+"+"+b.getName());
			return mergedData;
		}

		void addVertex(NavModelData* modelData, const vec3<F32>& vertex){

			if (modelData->getVertCount()+1 > modelData->_vertexCapacity)  {
				modelData->_vertexCapacity = ! modelData->_vertexCapacity ? 8 : modelData->_vertexCapacity*2;

				F32* nv = new F32[modelData->_vertexCapacity*3];

				if(modelData->getVertCount())	memcpy(nv, modelData->getVerts(), modelData->getVertCount()*3*sizeof(F32));
				if(modelData->getVerts() )	    SAFE_DELETE_ARRAY(modelData->_vertices);

				modelData->_vertices = nv;
			}

			F32* dst = &modelData->_vertices[modelData->getVertCount()*3];
			*dst++ = vertex.x;
			*dst++ = vertex.y;
			*dst++ = vertex.z;
			modelData->_vertexCount++;
		}
			
		void addTriangle(NavModelData* modelData, const vec3<U32>& triangleIndices, const SamplePolyAreas& areaType){

			if (modelData->getTriCount()+1 > modelData->_triangleCapacity)  {
				modelData->_triangleCapacity = !modelData->_triangleCapacity ? 8 : modelData->_triangleCapacity*2;
				I32* nv = new I32[modelData->_triangleCapacity*3];

				if (modelData->getTriCount())	memcpy(nv, modelData->_triangles, modelData->getTriCount()*3*sizeof(I32));
				if( modelData->getTris() )   	SAFE_DELETE_ARRAY(modelData->_triangles);
				modelData->_triangles = nv;
			}

			I32* dst = &modelData->_triangles[modelData->getTriCount()*3];
			*dst++ = triangleIndices.x;
			*dst++ = triangleIndices.y;
			*dst++ = triangleIndices.z;

			modelData->getAreaTypes().push_back(areaType);
			modelData->_triangleCount++;
		}

		NavModelData parseNode(SceneGraphNode* sgn, const std::string& navMeshName){

			NavModelData data;
			data.clear(false);
			data.setName(navMeshName);

			return parse(sgn->getBoundingBox(), data, sgn);
		}

		NavModelData parse(const BoundingBox& box, NavModelData& data, SceneGraphNode* sgn) {
  
			//Ignore if specified
			if(sgn->getNavigationContext() == SceneGraphNode::NODE_IGNORE)  goto next;
			//Skip small objects
			if(/*box.getWidth() < 0.05f || box.getDepth() < 0.05f || */box.getHeight() < 0.05f)  goto next;

			SceneNode* sn = sgn->getNode<SceneNode>();
			assert(sn != NULL);

			SceneNodeType nodeType = sn->getType();
			U32 ignoredNodeType = TYPE_LIGHT | TYPE_ROOT | TYPE_PARTICLE_EMITTER | TYPE_TRIGGER | TYPE_SKY;
			U32 allowedNodeType = TYPE_WATER | TYPE_TERRAIN | TYPE_OBJECT3D;
			if(!bitCompare(allowedNodeType, nodeType)){
				if(!bitCompare(ignoredNodeType, nodeType)){
					PRINT_FN(Locale::get("WARN_NAV_UNSUPPORTED"),sn->getName().c_str());
					goto next;
				}else{
					//we just ignore these types
				}
			}
			
			if(nodeType == TYPE_OBJECT3D) {
				U32 ignored3DObjectType = Object3D::MESH | Object3D::TEXT_3D | Object3D::FLYWEIGHT;
				if(bitCompare(ignored3DObjectType, dynamic_cast<Object3D*>(sn)->getType())) goto next;
			}

			U32 numVert = 1;
			U32 curNumVert = numVert;
			MeshDetailLevel level = DETAIL_LOW;
			VertexBufferObject* geometry = NULL;
			SamplePolyAreas areType = SAMPLE_AREA_OBSTACLE;

			switch(nodeType){
				case TYPE_WATER    : { level = DETAIL_BOUNDINGBOX; areType = SAMPLE_POLYAREA_WATER;   }break;
				case TYPE_TERRAIN  : { level = DETAIL_ABSOLUTE;	   areType = SAMPLE_POLYAREA_GROUND;  }break;
				case TYPE_OBJECT3D :
					level = (sgn->getUsageContext() == SceneGraphNode::NODE_DYNAMIC) ? DETAIL_BOUNDINGBOX : DETAIL_ABSOLUTE;
					break;
				default:{
					assert(false);//we should never reach this due to the bit checks above
					}break;
			};
		
			//I should remove this hack - Ionut
			if(nodeType == TYPE_WATER)  sgn = sgn->getChildren()["waterPlane"];

			D_PRINT_FN(Locale::get("NAV_MESH_CURRENT_NODE"),sn->getName().c_str(), (U32)level);

			if(level == DETAIL_ABSOLUTE){
				if(sn->getType() == TYPE_OBJECT3D)     geometry = dynamic_cast<Object3D* >(sn)->getGeometryVBO();
				else if(sn->getType() == TYPE_TERRAIN) geometry = dynamic_cast<Terrain* >(sn)->getGeometryVBO();
				else /*sn->getType() == TYPE_WATER*/   geometry = dynamic_cast<WaterPlane* >(sn)->getQuad()->getGeometryVBO();
				assert(geometry != NULL);

				const vectorImpl<vec3<F32> >& _vertices = geometry->getPosition();

				vec4<F32> hPos;
				for (U32 i = 0; i < _vertices.size(); ++i ){
					//Convert the current vertex position to homogeneous coordinates
					const vec3<F32>& pos = _vertices[i];
					hPos.set(pos.x,pos.y,pos.z,1.0f);
					//Apply the node's transform
					hPos = sgn->getTransform()->getGlobalMatrix() * hPos;
					//And add the vertex to the NavMesh
					addVertex(&data,hPos);
				}

 				for (U32 i = 0; i < geometry->getTriangles().size(); ++i){
					addTriangle(&data,geometry->getTriangles()[i], areType);
				}

			}else if(level == DETAIL_LOW || level == DETAIL_BOUNDINGBOX ){
           
				const vectorImpl<vec3<F32> >& _vertices = box.getPoints();

				for(U32 i = 0; i < 8; i++){
					numVert++;
					addVertex(&data, _vertices[i]);
				}

				for(U32 f = 0; f < 6; f++){
				   for(U32 v = 2; v < 4; v++){
					   // Note: We reverse the normal on the polygons to prevent things from going inside out
					   addTriangle(&data,  vec3<U32>(curNumVert+cubeFaces[f][0]+1,
													 curNumVert+cubeFaces[f][v]+1,
													 curNumVert+cubeFaces[f][v-1]+1),
								   areType);
				   }
				}
			}else if( level == DETAIL_MEDIUM || level == DETAIL_HIGH){
				ERROR_FN(Locale::get("ERROR_RECAST_MEDIUM_DETAIL"));
			}else{
				ERROR_FN(Locale::get("ERROR_RECAST_DETAIL_LEVEL"),level);
			}

			PRINT_FN(Locale::get("NAV_MESH_ADD_NODE"), sn->getName().c_str());

			//altough labels are bad, skipping here using them is the easiest solution to follow -Ionut
			next:;
			for_each(SceneGraphNode::NodeChildren::value_type& it,sgn->getChildren()){
				parse(it.second->getBoundingBox(), data, it.second);
			}

			return data;
		}

		char* parseRow(char* buf, char* bufEnd, char* row, I32 len){
			bool cont = false;
			bool start = true;
			bool done = false;
			I32 n = 0;

			while (!done && buf < bufEnd)   {
				char c = *buf;
				buf++;
				// multirow
				switch (c)  {
					case '\\': cont = true; break; // multirow
					case '\n': { if (start) break; done = true; } break;
					case '\r': break;
					case '\t': 
					case ' ' : if (start) break;
					default  : {
						start = false;
						cont = false;
						row[n++] = c;
						if (n >= len-1)	done = true;
					}break;
				}
			}
			row[n] = '\0';
			return buf;
		}

		I32 parseFace(char* row, I32* data, I32 n, I32 vcnt) {
			I32 j = 0;
			while (*row != '\0')  {
				// Skip initial white space
				while (*row != '\0' && (*row == ' ' || *row == '\t')) row++;
				char* s = row;
				// Find vertex delimiter and terminated the string there for conversion.
				while (*row != '\0' && *row != ' ' && *row != '\t')  {
					if (*row == '/') *row = '\0';
					row++;
				}

				if (*s == '\0')	continue;

				I32 vi = atoi(s);
				data[j++] = vi < 0 ? vi+vcnt : vi-1;
				if (j >= n) return j;
			}
			return j;
		}
	};
};