#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace XML
{
	void loadScene(const std::string& sceneName);
	void loadGeometry(const std::string& file);
}