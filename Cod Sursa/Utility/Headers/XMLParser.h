#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
using namespace std;

namespace XML
{

	//Parent Function
	void loadScripts(const string &file);

	//Child Functions
	void loadConfig(const string& file);
	void loadScene(const string& sceneName);
	void loadGeometry(const string& file);
	void loadTerrain(const string& file);
	
	//ToDo: ....... Add more

}