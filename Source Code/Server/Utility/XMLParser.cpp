#include "Headers/XMLParser.h"
#include "Headers/Patch.h"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace Divide {
namespace XML {
using boost::property_tree::ptree;
ptree pt;

void loadScene(const stringImpl &sceneName) {
    pt.clear();
    std::cout << "XML: Loading scene [ " << sceneName << " ] " << std::endl;
    read_xml(("Scenes/" + sceneName + ".xml").c_str(), pt);
    stringImpl assetLocation("Scenes/" + sceneName + "/");
    loadGeometry(assetLocation + stringImpl(pt.get("assets", "assets.xml").c_str()));
}

void loadGeometry(const stringImpl &file) {
    pt.clear();
    std::cout << "XML: Loading Geometry: [ " << file << " ] " << std::endl;
    read_xml(file.c_str(), pt);
    ptree::iterator it;
    for (it = std::begin(pt.get_child("geometry"));
         it != std::end(pt.get_child("geometry")); ++it) {
        std::string name(it->second.data());
        std::string format(it->first.data());

        if (format.find("<xmlcomment>") != stringImpl::npos) {
            continue;
        }

        FileData model;
        model.ItemName = name.c_str();
        model.ModelName = "Assets/";
        model.ModelName.append(pt.get<stringImpl>(name + ".model"));
        model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
        model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
        model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
        model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
        model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
        model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
        model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
        model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
        model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
        model.type = GeometryType::MESH;
        model.version = pt.get<F32>(name + ".version");
        Patch::instance().addGeometry(model);
    }
    for (it = std::begin(pt.get_child("vegetation"));
         it != std::end(pt.get_child("vegetation")); ++it) {
        std::string name(it->second.data());
        std::string format(it->first.data());
        if (format.find("<xmlcomment>") != stringImpl::npos) {
            continue;
        }
        FileData model;
        model.ItemName = name.c_str();
        model.ModelName = "Assets/";
        model.ModelName.append(pt.get<stringImpl>(name + ".model"));
        model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
        model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
        model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
        model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
        model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
        model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
        model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
        model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
        model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
        model.type = GeometryType::VEGETATION;
        model.version = pt.get<F32>(name + ".version");
        Patch::instance().addGeometry(model);
    }

    if (boost::optional<ptree &> primitives =
            pt.get_child_optional("primitives"))
        for (it = std::begin(pt.get_child("primitives"));
             it != std::end(pt.get_child("primitives")); ++it) {
            std::string name(it->second.data());
            std::string format(it->first.data());
            if (format.find("<xmlcomment>") != stringImpl::npos) {
                continue;
            }
            FileData model;
            model.ItemName = name.c_str();
            model.ModelName = pt.get<stringImpl>(name + ".model");
            model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
            model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
            model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
            model.orientation.x =
                pt.get<F32>(name + ".orientation.<xmlattr>.x");
            model.orientation.y =
                pt.get<F32>(name + ".orientation.<xmlattr>.y");
            model.orientation.z =
                pt.get<F32>(name + ".orientation.<xmlattr>.z");
            model.scale.x = pt.get<F32>(name + ".scale.<xmlattr>.x");
            model.scale.y = pt.get<F32>(name + ".scale.<xmlattr>.y");
            model.scale.z = pt.get<F32>(name + ".scale.<xmlattr>.z");
            /*Primitives don't use materials yet so we can define colours*/
            model.colour.r = pt.get<F32>(name + ".colour.<xmlattr>.r");
            model.colour.g = pt.get<F32>(name + ".colour.<xmlattr>.g");
            model.colour.b = pt.get<F32>(name + ".colour.<xmlattr>.b");
            /*The data variable stores a float variable (not void*) that can
             * represent anything you want*/
            /*For Text3D, it's the line width and for Box3D it's the edge
             * length*/
            if (model.ModelName.compare("Text3D") == 0) {
                model.data = pt.get<F32>(name + ".lineWidth");
                model.data2 = pt.get<stringImpl>(name + ".text");
            } else if (model.ModelName.compare("Box3D") == 0)
                model.data = pt.get<F32>(name + ".size");

            else if (model.ModelName.compare("Sphere3D") == 0)
                model.data = pt.get<F32>(name + ".radius");
            else
                model.data = 0;

            model.type = GeometryType::PRIMITIVE;
            model.version = pt.get<F32>(name + ".version");
            Patch::instance().addGeometry(model);
        }
}
}

};  // namespace Divide