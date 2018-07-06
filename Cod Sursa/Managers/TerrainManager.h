#ifndef _TERRAIN_MANAGER_H
#define _TERRAIN_MANAGER_H

#include "Manager.h"
#include "Terrain/Terrain.h"
#include "Utility/Headers/Singleton.h"
#include "Terrain/Water.h"
#include <boost/thread.hpp>

class TerrainManager : public Manager
{
public:
	TerrainManager();
	~TerrainManager();

	void createTerrains(std::vector<TerrainInfo>& terrains);
	void drawTerrains(mat4& sunModelviewProj, bool drawInactive = false, bool drawInReflexion = false,vec4& ambientColor = vec4(0.5f,0.5f,0.5f,0.5f));
	void drawVegetation(bool drawInReflexion);
	void joinThread(){if(_thrd) _thrd->join();}
	void detachThread(){if(_thrd) _thrd->detach();}
	void lockMutex(){boost::mutex::scoped_lock lock(_io_mutex);}
	void generateVegetation();
	void generateVegetation(const std::string& name);
	void drawInfinitePlane(F32 max_distance,FrameBufferObject* fbo[]);


	F32&  getGrassVisibility()		    {return _grassVisibility;}
	F32&  getTreeVisibility()		    {return _treeVisibility;}
	F32&  getGeneralVisibility()	  	{return _generalVisibility;}
	mat4& getSunModelviewProjMatrix()   {return _sunModelviewProj;}

	F32& getWindSpeed(){return _windSpeed;}
	F32& getWindDirX(){return _windDirX;}
	F32& getWindDirZ(){return _windDirZ;}

	Terrain* getTerrain(int index) {return ((Terrain*)_resDB.begin()->second);}

	void setDepthMap(int index, FrameBufferObject* depthMap);
private:

	F32           _minHeight,
				  _grassVisibility,_treeVisibility,_generalVisibility,
				  _windSpeed,_windDirX, _windDirZ;

	bool	      _computedMinHeight,_loaded;
	Terrain*      _terrain; //temp terrain file;
	boost::thread *_thrd;
	boost::mutex   _io_mutex;
	boost::thread_specific_ptr<int> ptr;
	WaterPlane*  _water;
	mat4 _sunModelviewProj;

	void createThreadedTerrains(std::vector<TerrainInfo>& terrains);
	void drawTerrain(bool drawInactive = false, bool drawInReflexion = false, vec4& ambientColor = vec4(0.5f,0.5f,0.5f,0.5f));
};

#endif