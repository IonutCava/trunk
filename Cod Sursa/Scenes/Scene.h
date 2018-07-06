#ifndef _SCENE_H
#define _SCENE_H

#include "resource.h"
#include <unordered_map>
#include "Utility/Headers/BaseClasses.h"
#include "Hardware/Video/GFXDevice.h"
#include "Importer/OBJ.h"

class Scene : public Resource
{

public:
	Scene(string name, string mainScript) :
	  _name(name),
	  _mainScript(mainScript),
	  _GFX(GFXDevice::getInstance())
	  {
		numberOfTerrains = 0;
		numberOfObjects = 0;
	  };
	virtual void render() = 0;
	virtual void preRender() = 0;
	virtual bool load(const string& name) = 0;
	virtual bool unload() = 0;
	virtual void processInput() = 0;
	virtual void processEvents(F32 time) = 0;

   int getNumberOfObjects(){return numberOfObjects;}
   int getNumberOfTerrains(){return numberOfTerrains;}
   void setNumberOfObjects(int nr){numberOfObjects = nr;}
   void setNumberOfTerrains(int nr){numberOfTerrains = nr;}
   void incNumberOfObjects(){numberOfObjects++;}
   void incNumberOfTerrains(){numberOfTerrains++;}
   void loadModel(FileData *DA,int index,int mode);
   ImportedModel* findObject(string& name);

   inline vector<ImportedModel*>& getModelArray(){return ModelArray;}
   void setInitialData(const vector<FileData>& models);

protected:
	string _name, _mainScript;
	int numberOfTerrains;
	int numberOfObjects;
	//All models loaded in the currentScene
	tr1::unordered_map<string, string> _objects;
	vector<ImportedModel*> ModelArray;
	vector<ImportedModel*>::iterator ModelIterator;
	//vector<Events> _events;

protected:
	string& getName() {return _name;}
	string& getScriptName() {return _mainScript;}
	GFXDevice& _GFX;
	

	void addObject(string type, string location){_objects.insert(pair<string,string>(type,location));} 


	bool loadResources(bool continueOnErrors){return true;}
	bool loadEvents(bool continueOnErrors){return true;}

	void clearObjects(){_objects.empty();}
	void clearEvents(){/*_events.empty();*/}

};

#endif