#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace XML
{

	//Parent Function
	void loadScripts(const std::string &file);

	//Child Functions
	void loadConfig(const std::string& file);
	void loadScene(const std::string& sceneName);
	void loadGeometry(const std::string& file);
	void loadTerrain(const std::string& file);
	
	//ToDo: ....... Add more

}