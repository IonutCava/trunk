#include "stdafx.h"

#include "Headers/GUIInterface.h"

#include "Headers/GUI.h"
#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

#include "Scenes/Headers/Scene.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {

GUIInterface::GUIInterface(GUI& context)
    : _context(&context)
{
    Locale::addChangeLanguageCallback([this](const char* newLanguage) { 
        onLanguageChange(newLanguage);
    });
}

GUIInterface::~GUIInterface()
{
    for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
        for (GUIMap::value_type& it : _guiElements[i]) {
            MemoryManager::DELETE(it.second.first);
        }
    }
}

void GUIInterface::onLanguageChange(const char* newLanguage) {
    ACKNOWLEDGE_UNUSED(newLanguage);
}

void GUIInterface::addElement(U64 id, GUIElement* element) {
    assert(Runtime::isMainThread());

    U8 typeIndex = to_U8(element->getType());
    GUIMap& targetMap = _guiElements[typeIndex];

    GUIMap::iterator it = targetMap.find(id);
    if (it != std::end(targetMap)) {
        MemoryManager::SAFE_UPDATE(it->second.first, element);
        it->second.second = element ? element->isVisible() : false;
    } else {
        hashAlg::insert(targetMap, id, std::make_pair(element, element ? element->isVisible() : false));
    }
}

GUIElement* GUIInterface::getGUIElementImpl(U64 elementName, GUIType type) const {
    GUIElement* ret = nullptr;
    if (type == GUIType::COUNT) {
        for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
            GUIMap::const_iterator it = _guiElements[i].find(elementName);
            if (it != std::cend(_guiElements[i])) {
                ret = it->second.first;
                break;
            }
        }
    } else {
        GUIMap::const_iterator it = _guiElements[to_U32(type)].find(elementName);
        if (it != std::cend(_guiElements[to_U32(type)])) {
            ret = it->second.first;
        }
    }
    return ret;
}

GUIElement* GUIInterface::getGUIElementImpl(I64 elementID, GUIType type) const {
    GUIElement* ret = nullptr;
    GUIElement* element = nullptr;
    if (type == GUIType::COUNT) {
        for (U8 i = 0; i < to_base(GUIType::COUNT); ++i) {
            for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
                element = guiStackIterator.second.first;
                if (element->getGUID() == elementID) {
                    ret = element;
                    break;
                }
            }
            if (ret != nullptr) {
                break;
            }
        }
    } else {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[to_U32(type)]) {
            element = guiStackIterator.second.first;
            if (element->getGUID() == elementID) {
                ret = element;
                break;
            }
        }
    }

    return ret;
}

GUIButton* GUIInterface::addButton(U64 guiID,
                                   const stringImpl& name,
                                   const stringImpl& text,
                                   const RelativePosition2D& position,
                                   const RelativeScale2D& size,
                                   const stringImpl& rootSheetID) {
    assert(getGUIElement(guiID) == nullptr);

    CEGUI::Window* parent = _context->getCEGUIContext().getRootWindow();
    if (!rootSheetID.empty()) {
        parent = parent->getChild(rootSheetID.c_str());
    }

    ResourceDescriptor beepSound("buttonClick");
    beepSound.assetName("beep.wav");
    beepSound.assetLocation(Paths::g_assetsLocation + Paths::g_soundsLocation);
    beepSound.setFlag(false);
    AudioDescriptor_ptr onClickSound = CreateResource<AudioDescriptor>(_context->parent().resourceCache(), beepSound);

    GUIButton* btn = MemoryManager_NEW GUIButton(guiID,
                                                 name,
                                                 text,
                                                 _context->guiScheme(),
                                                 position,
                                                 size,
                                                 parent);

    btn->setEventSound(GUIButton::Event::MouseClick, onClickSound);

    addElement(guiID, btn);

    return btn;
}

GUIMessageBox* GUIInterface::addMsgBox(U64 guiID,
                                       const stringImpl& name,
                                       const stringImpl& title,
                                       const stringImpl& message,
                                       const vec2<I32>& offsetFromCentre) {
    assert(getGUIElement(guiID) == nullptr);

    GUIMessageBox* box = MemoryManager_NEW GUIMessageBox(guiID,
                                                         name,
                                                         title,
                                                         message,
                                                         offsetFromCentre,
                                                         _context->rootSheet());
    addElement(guiID, box);

    return box;
}

GUIText* GUIInterface::addText(U64 guiID,
                               const stringImpl& name,
                               const RelativePosition2D& position,
                               const stringImpl& font,
                               const UColour4& colour,
                               const stringImpl& text,
                               U8 fontSize) {
    assert(getGUIElement(guiID) == nullptr);

    GUIText* t = MemoryManager_NEW GUIText(guiID,
                                           name,
                                           text,
                                           position,
                                           font,
                                           colour,
                                           _context->rootSheet(),
                                           fontSize);
    addElement(guiID, t);

    return t;
}

GUIFlash* GUIInterface::addFlash(U64 guiID,
                                 const stringImpl& name,
                                 stringImpl movie,
                                 const RelativePosition2D& position,
                                 const RelativeScale2D& size) {
    ACKNOWLEDGE_UNUSED(position);
    ACKNOWLEDGE_UNUSED(size);

    assert(getGUIElement(guiID) == nullptr);
    
    GUIFlash* flash = MemoryManager_NEW GUIFlash(guiID, name, _context->rootSheet());
    addElement(guiID, flash);

    return flash;
}

GUIText* GUIInterface::modifyText(U64 guiID, const stringImpl& text) {
    GUIMap::iterator it = _guiElements[to_base(GUIType::GUI_TEXT)].find(guiID);

    if (it == std::cend(_guiElements[to_base(GUIType::GUI_TEXT)])) {
        return nullptr;
    }

    GUIText* textElement = static_cast<GUIText*>(it->second.first);
    assert(textElement != nullptr);

    textElement->text(text);

    return textElement;
}

CEGUI::Window* GUIInterface::createWindow(const CEGUI::String& type, const CEGUI::String& name) {
    CEGUI::Window* window = CEGUI::WindowManager::getSingleton().createWindow(type, name);
    if (window != nullptr) {
        _context->rootSheet()->addChild(window);
    }

    return window;
}

CEGUI::Window* GUIInterface::loadWindowFromLayoutFile(const char* layoutFileName) {
    CEGUI::Window* window = CEGUI::WindowManager::getSingleton().loadLayoutFromFile(layoutFileName);
    if (window != nullptr) {
        _context->rootSheet()->addChild(window);
    }

    return window;
}

bool GUIInterface::unloadWindow(CEGUI::Window*& window) {
    if (_context->rootSheet()->isChild(window)) {
        _context->rootSheet()->destroyChild(window);
        window = nullptr;
        return true;
    }

    return false;
}
}; //namespace Divide