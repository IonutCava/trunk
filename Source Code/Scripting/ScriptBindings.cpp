#include "Headers/ScriptBindings.h"
#include "Headers/GameScript.h"

#include <chaiscript/chaiscript_stdlib.hpp>

namespace Divide {

    std::shared_ptr<chaiscript::Module> create_chaiscript_stl_extra()
    {

        auto module = chaiscript::bootstrap::standard_library::list_type<std::list<chaiscript::Boxed_Value> >("List");
        module->add(chaiscript::bootstrap::standard_library::vector_type<std::vector<uint16_t> >("u16vector"));
        module->add(chaiscript::vector_conversion<std::vector<uint16_t>>());
        return module;
    }

    std::shared_ptr<chaiscript::Module> create_chaiscript_stdlib() {
        return chaiscript::Std_Lib::library();
    }

    std::shared_ptr<chaiscript::Module> create_chaiscript_bindings() {
        auto module = std::make_shared<chaiscript::Module>();

        return module;
    }
};