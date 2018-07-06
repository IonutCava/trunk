#ifndef _TERRAIN_MANAGER_H
#define _TERRAIN_MANAGER_H

#include "Manager.h"
#include "Terrain/Terrain.h"
#include "Utility/Headers/Singleton.h"
#include <boost/thread.hpp>

class TerrainManager : public Manager
{
public:
	void initialize();
	void createTerrains(vector<TerrainInfo>& terrains);
	void drawTerrains(bool drawInactive = false, bool drawInReflexion = false,vec4& ambientColor = vec4(0.5f,0.5f,0.5f,0.5f));
	void joinThread(){if(_thrd) _thrd->join();}
	void detachThread(){if(_thrd) _thrd->detach();}
	void lockMutex(){boost::mutex::scoped_lock lock(_io_mutex);}
	void generateVegetation();
	void generateVegetation(const string& name);
	void drawInfinitePlane(F32 max_distance,FrameBufferObject& _fbo);


	F32& getGrassVisibility(){return _grassVisibility;}
	F32& getTreeVisibility(){return _treeVisibility;}
	F32& getGeneralVisibility(){return _generalVisibility;}

	F32& getWindSpeed(){return _windSpeed;}
	F32& getWindDirX(){return _windDirX;}
	F32& getWindDirZ(){return _windDirZ;}

	Terrain* getTerrain(int index) {return ((Terrain*)_resDB.begin()->second);}
private:

	F32           _minHeight,_maxHeight,
				  _grassVisibility,_treeVisibility,_generalVisibility,
				  _windSpeed,_windDirX, _windDirZ;

	bool	      _computedMinHeight,_loaded;
	Shader*       _waterShader;
	Texture2D*    _texWater;
	Terrain*      _terrain; //temp terrain file;
	boost::thread *_thrd;
	boost::mutex   _io_mutex;
	boost::thread_specific_ptr<int> ptr;

	void createThreadedTerrains(vector<TerrainInfo>& terrains);
	void drawTerrain(bool drawInactive = false, bool drawInReflexion = false,vec4& ambientColor = vec4(0.5f,0.5f,0.5f,0.5f));
};

#endif