#include "Headers/GUIInterface.h"

#include "Headers/GUI.h"
#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

namespace Divide {

GUIInterface::GUIInterface(const vec2<U16>& resolution)
    : _resolutionCache(resolution)
{
}

GUIInterface::~GUIInterface()
{
    for (GUIMap::value_type& it : _guiElements) {
        MemoryManager::DELETE(it.second.first);
    }
}

void GUIInterface::onChangeResolution(U16 w, U16 h) {
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        guiStackIterator.second.first->onChangeResolution(w, h);
    }

    _resolutionCache.set(w, h);
}


void GUIInterface::addElement(ULL id, GUIElement* element) {
    GUIMap::iterator it = _guiElements.find(id);
    if (it != std::end(_guiElements)) {
        MemoryManager::SAFE_UPDATE(it->second.first, element);
        it->second.second = element ? element->isVisible() : false;
    } else {
        hashAlg::insert(_guiElements, std::make_pair(id, std::make_pair(element, element ? element->isVisible() : false)));
    }
}

GUIElement* GUIInterface::getGUIElementImpl(ULL elementName) const {
    GUIElement* ret = nullptr;
    GUIMap::const_iterator it = _guiElements.find(elementName);
    if (it != std::cend(_guiElements)) {
        ret = it->second.first;
    }


    return ret;
}

GUIElement* GUIInterface::getGUIElementImpl(I64 elementID) const {
    GUIElement* ret = nullptr;
    GUIElement* element = nullptr;
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        element = guiStackIterator.second.first;
        if (element->getGUID() == elementID) {
            ret = element;
            break;
        }
    }

    return ret;
}


void GUIInterface::mouseMoved(const GUIEvent& event) {
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        guiStackIterator.second.first->mouseMoved(event);
    }
}

void GUIInterface::onMouseUp(const GUIEvent& event) {
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        guiStackIterator.second.first->onMouseUp(event);
    }
}

void GUIInterface::onMouseDown(const GUIEvent& event) {
    for (const GUIMap::value_type& guiStackIterator : _guiElements) {
        guiStackIterator.second.first->onMouseDown(event);
    }
}


GUIButton* GUIInterface::addButton(ULL ID,
                                   const stringImpl& text,
                                   const vec2<I32>& position,
                                   const vec2<U32>& dimensions,
                                   ButtonCallback callback,
                                   const stringImpl& rootSheetID) {

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
        parent = GUI::instance().rootSheet();
    }

    GUIButton* btn = MemoryManager_NEW GUIButton(ID, text, GUI::instance().guiScheme(), relOffset, relDim, parent, callback);

    addElement(ID, btn);

    return btn;
}

GUIMessageBox* GUIInterface::addMsgBox(ULL ID,
                                       const stringImpl& title,
                                       const stringImpl& message,
                                       const vec2<I32>& offsetFromCentre) {
    GUIMessageBox* box = MemoryManager_NEW GUIMessageBox(ID, title, message, offsetFromCentre, GUI::instance().rootSheet());
    addElement(ID, box);

    return box;
}

GUIText* GUIInterface::addText(ULL ID,
                               const vec2<I32>& position,
                               const stringImpl& font,
                               const vec4<U8>& color,
                               const stringImpl& text,
                               U32 fontSize) {

    GUIText* t = MemoryManager_NEW GUIText(ID,
                                           text,
                                           vec2<F32>(position.width,
                                                     position.height),
                                           font,
                                           color,
                                           GUI::instance().rootSheet(),
                                           fontSize);
    t->initialHeightCache(to_float(getDisplayResolution().height));
    addElement(ID, t);

    return t;
}

GUIFlash* GUIInterface::addFlash(ULL ID,
                                 stringImpl movie,
                                 const vec2<U32>& position,
                                 const vec2<U32>& extent) {

    GUIFlash* flash = MemoryManager_NEW GUIFlash(ID, GUI::instance().rootSheet());
    addElement(ID, flash);

    return flash;
}

GUIText* GUIInterface::modifyText(ULL ID, const stringImpl& text) {
    GUIMap::iterator it = _guiElements.find(ID);

    if (it == std::cend(_guiElements)) {
        return nullptr;
    }

    GUIElement* element = it->second.first;
    assert(element->getType() == GUIType::GUI_TEXT);

    GUIText* textElement = dynamic_cast<GUIText*>(element);
    assert(textElement != nullptr);

    textElement->text(text);

    return textElement;
}

const vec2<U16>& GUIInterface::getDisplayResolution() const {
    return _resolutionCache;
}
}; //namespace Divide