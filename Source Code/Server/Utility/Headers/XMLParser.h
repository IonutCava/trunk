#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "Utility/Headers/String.h"

namespace Divide {
namespace XML {
void loadScene(const stringImpl& sceneName);
void loadGeometry(const stringImpl& file);
}

};  // namespace Divide