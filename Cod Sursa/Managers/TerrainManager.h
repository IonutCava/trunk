#ifndef _TERRAIN_MANAGER_H
#define _TERRAIN_MANAGER_H

#include "Manager.h"
#include "resource.h"
#include "Terrain/Terrain.h"
#include "Utility/Headers/Singleton.h"
#include <boost/thread.hpp>

SINGLETON_BEGIN_EXT1(TerrainManager, Manager)

private:

	F32           _minHeight,_maxHeight,_grassVisibility,_treeVisibility,_windSpeed,_windDirX, _windDirZ;
	bool	      _computedMinHeight,_loaded;
	Shader*       _waterShader;
	Texture2D*    _texWater;
	Terrain*      _terrain; //temp terrain file;
	boost::thread *_thrd;
	boost::mutex   _io_mutex;
	boost::thread_specific_ptr<int> ptr;

	void createThreadedTerrains(vector<TerrainInfo>& terrains);
	void drawTerrain(bool drawInactive = false, bool drawInReflexion = false);

public:
	void initialize();
	void createTerrains(vector<TerrainInfo>& terrains);
	void drawTerrains(bool drawInactive = false, bool drawInReflexion = false);
	void joinThread(){if(_thrd) _thrd->join();}
	void detachThread(){if(_thrd) _thrd->detach();}
	void lockMutex(){boost::mutex::scoped_lock lock(_io_mutex);}
	void generateVegetation();
	void generateVegetation(const string& name);
	void drawInfinitePlane(const vec3& eye, F32 max_distance,FrameBufferObject& _fbo);
	void setGrassVisibility(F32 grassVisibility){_grassVisibility = grassVisibility;}
	void setTreeVisibility(F32 treeVisibility){_treeVisibility = treeVisibility;}
	void setWindSpeed(F32 windSpeed){_windSpeed = windSpeed;}
	void setWindDirX(F32 windDirX){_windDirX = windDirX;}
	void setWindDirZ(F32 windDirZ){_windDirZ = windDirZ;}
	F32 getGrassVisibility(){return _grassVisibility;}
	F32 getTreeVisibility(){return _treeVisibility;}
	F32 getWindSpeed(){return _windSpeed;}
	F32 getWindDirX(){return _windDirX;}
	F32 getWindDirZ(){return _windDirZ;}

SINGLETON_END()

#endif