#include "Headers/GUIInterface.h"

#include "Headers/GUI.h"
#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

#include "Scenes/Headers/Scene.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Core/Headers/Application.h"

namespace Divide {

GUIInterface::GUIInterface(GUI& context, const vec2<U16>& resolution)
    : _context(&context),
      _resolutionCache(resolution)
{
}

GUIInterface::~GUIInterface()
{
    for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
        for (GUIMap::value_type& it : _guiElements[i]) {
            MemoryManager::DELETE(it.second.first);
        }
    }
}

void GUIInterface::onChangeResolution(U16 w, U16 h) {
    for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
            guiStackIterator.second.first->onChangeResolution(w, h);
        }
    }
    _resolutionCache.set(w, h);
}


void GUIInterface::addElement(U64 id, GUIElement* element) {
    assert(Application::instance().isMainThread());

    U8 typeIndex = to_ubyte(element->getType());
    GUIMap& targetMap = _guiElements[typeIndex];

    GUIMap::iterator it = targetMap.find(id);
    if (it != std::end(targetMap)) {
        MemoryManager::SAFE_UPDATE(it->second.first, element);
        it->second.second = element ? element->isVisible() : false;
    } else {
        hashAlg::insert(targetMap, std::make_pair(id, std::make_pair(element, element ? element->isVisible() : false)));
    }
}

GUIElement* GUIInterface::getGUIElementImpl(U64 elementName, GUIType type) const {
    GUIElement* ret = nullptr;
    if (type == GUIType::COUNT) {
        for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
            GUIMap::const_iterator it = _guiElements[i].find(elementName);
            if (it != std::cend(_guiElements[i])) {
                ret = it->second.first;
                break;
            }
        }
    } else {
        GUIMap::const_iterator it = _guiElements[to_uint(type)].find(elementName);
        if (it != std::cend(_guiElements[to_uint(type)])) {
            ret = it->second.first;
        }
    }
    return ret;
}

GUIElement* GUIInterface::getGUIElementImpl(I64 elementID, GUIType type) const {
    GUIElement* ret = nullptr;
    GUIElement* element = nullptr;
    if (type == GUIType::COUNT) {
        for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
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
        for (const GUIMap::value_type& guiStackIterator : _guiElements[to_uint(type)]) {
            element = guiStackIterator.second.first;
            if (element->getGUID() == elementID) {
                ret = element;
                break;
            }
        }
    }

    return ret;
}


void GUIInterface::mouseMoved(const GUIEvent& event) {
    for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
            guiStackIterator.second.first->mouseMoved(event);
        }
    }
}

void GUIInterface::onMouseUp(const GUIEvent& event) {
    for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
            guiStackIterator.second.first->onMouseUp(event);
        }
    }
}

void GUIInterface::onMouseDown(const GUIEvent& event) {
    for (U8 i = 0; i < to_const_uint(GUIType::COUNT); ++i) {
        for (const GUIMap::value_type& guiStackIterator : _guiElements[i]) {
            guiStackIterator.second.first->onMouseDown(event);
        }
    }
}


GUIButton* GUIInterface::addButton(U64 guiID,
                                   const stringImpl& text,
                                   const vec2<I32>& position,
                                   const vec2<U32>& dimensions,
                                   ButtonCallback callback,
                                   const stringImpl& rootSheetID) {
    assert(getGUIElement(guiID) == nullptr);

    const vec2<U16>& resolution = getDisplayResolution();

    vec2<F32> relOffset((position.x * 100.0f) / resolution.width,
                        (position.y * 100.0f) / resolution.height);

    vec2<F32> relDim((dimensions.x * 100.0f) / resolution.width,
                     (dimensions.y * 100.0f) / resolution.height);

    CEGUI::Window* parent = nullptr;
    if (!rootSheetID.empty()) {
        parent = CEGUI_DEFAULT_CTX.getRootWindow()->getChild(rootSheetID.c_str());
    }

    if (!parent) {
        parent = _context->rootSheet();
    }

    stringImpl assetPath = ParamHandler::instance().getParam<stringImpl>(_ID("assetsLocation"));
    ResourceDescriptor beepSound("buttonClick");
    beepSound.setResourceLocation(assetPath + "/sounds/beep.wav");
    beepSound.setFlag(false);
    AudioDescriptor_ptr onClickSound = CreateResource<AudioDescriptor>(_context->parent().resourceCache(), beepSound);

    GUIButton* btn = MemoryManager_NEW GUIButton(guiID, text, _context->guiScheme(), relOffset, relDim, parent, callback, onClickSound);

    addElement(guiID, btn);

    return btn;
}

GUIMessageBox* GUIInterface::addMsgBox(U64 guiID,
                                       const stringImpl& title,
                                       const stringImpl& message,
                                       const vec2<I32>& offsetFromCentre) {
    assert(getGUIElement(guiID) == nullptr);

    GUIMessageBox* box = MemoryManager_NEW GUIMessageBox(guiID,
                                                         title,
                                                         message,
                                                         offsetFromCentre,
                                                         _context->rootSheet());
    addElement(guiID, box);

    return box;
}

GUIText* GUIInterface::addText(U64 guiID,
                               const vec2<I32>& position,
                               const stringImpl& font,
                               const vec4<U8>& colour,
                               const stringImpl& text,
                               U32 fontSize) {
    assert(getGUIElement(guiID) == nullptr);

    GUIText* t = MemoryManager_NEW GUIText(guiID,
                                           text,
                                           vec2<F32>(position.width,
                                                     position.height),
                                           font,
                                           colour,
                                           _context->rootSheet(),
                                           fontSize);
    t->initialHeightCache(to_float(getDisplayResolution().height));
    addElement(guiID, t);

    return t;
}

GUIFlash* GUIInterface::addFlash(U64 guiID,
                                 stringImpl movie,
                                 const vec2<U32>& position,
                                 const vec2<U32>& extent) {
    assert(getGUIElement(guiID) == nullptr);

    GUIFlash* flash = MemoryManager_NEW GUIFlash(guiID, _context->rootSheet());
    addElement(guiID, flash);

    return flash;
}

GUIText* GUIInterface::modifyText(U64 guiID, const stringImpl& text) {
    GUIMap::iterator it = _guiElements[to_const_uint(GUIType::GUI_TEXT)].find(guiID);

    if (it == std::cend(_guiElements[to_const_uint(GUIType::GUI_TEXT)])) {
        return nullptr;
    }

    GUIText* textElement = static_cast<GUIText*>(it->second.first);
    assert(textElement != nullptr);

    textElement->text(text);

    return textElement;
}

const vec2<U16>& GUIInterface::getDisplayResolution() const {
    return _resolutionCache;
}

}; //namespace Divide