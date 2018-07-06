#include "Headers/SceneGUIElements.h"

#include "Headers/GUIFlash.h"
#include "Headers/GUIText.h"
#include "Headers/GUIButton.h"
#include "Headers/GUIConsole.h"
#include "Headers/GUIMessageBox.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#include <CEGUI/CEGUI.h>
#endif //CEGUI_STATIC

namespace Divide {

SceneGUIElements::SceneGUIElements(Scene& parentScene)
    : SceneComponent(parentScene)
{
}

SceneGUIElements::~SceneGUIElements()
{
    for (GUI::GUIMap::value_type& it : _sceneElements) {
        MemoryManager::DELETE(it.second.first);
    }
}

void SceneGUIElements::draw() {
    for (const GUI::GUIMap::value_type& guiStackIterator : _sceneElements) {
        GUIElement& element = *guiStackIterator.second.first;
        // Skip hidden elements
        if (element.isVisible()) {
            element.draw();
        }
    }
}

void SceneGUIElements::onChangeResolution(U16 w, U16 h) {
    for (const GUI::GUIMap::value_type& guiStackIterator : _sceneElements) {
        guiStackIterator.second.first->onChangeResolution(w, h);
    }
}

void SceneGUIElements::onEnable() {
    for (const GUI::GUIMap::value_type& guiStackIterator : _sceneElements) {
        guiStackIterator.second.first->setVisible(guiStackIterator.second.second);
    }
}

void SceneGUIElements::onDisable() {
    for (const GUI::GUIMap::value_type& guiStackIterator : _sceneElements) {
        guiStackIterator.second.first->setVisible(false);
    }
}

GUIElement* SceneGUIElements::get(ULL elementName) const {
    GUIElement* element = nullptr;
    for (GUI::GUIMap::value_type it : _sceneElements) {
        if (_ID_RT(it.second.first->getName()) == elementName) {
            element = it.second.first;
            break;
        }
    }

    return element;
}

GUIElement* SceneGUIElements::get(I64 elementGUID) const {
    GUIElement* element = nullptr;
    for (GUI::GUIMap::value_type it : _sceneElements) {
        if (it.second.first->getGUID() == elementGUID) {
            element = it.second.first;
            break;
        }
    }

    return element;
}

void SceneGUIElements::addElement(ULL id, GUIElement* element) {
    GUI::GUIMap::iterator it = _sceneElements.find(id);
    if (it != std::end(_sceneElements)) {
        MemoryManager::SAFE_UPDATE(it->second.first, element);
        it->second.second = element ? element->isVisible() : false;
    }
    else {
        hashAlg::insert(_sceneElements, std::make_pair(id, std::make_pair(element, element ? element->isVisible() : false)));
    }
}

void SceneGUIElements::mouseMoved(const GUIEvent& event) {
    for (const GUI::GUIMap::value_type& guiStackIterator : _sceneElements) {
        guiStackIterator.second.first->mouseMoved(event);
    }
}

void SceneGUIElements::onMouseUp(const GUIEvent& event) {
    for (const GUI::GUIMap::value_type& guiStackIterator : _sceneElements) {
        guiStackIterator.second.first->onMouseUp(event);
    }
}

void SceneGUIElements::onMouseDown(const GUIEvent& event) {
    for (const GUI::GUIMap::value_type& guiStackIterator : _sceneElements) {
        guiStackIterator.second.first->onMouseDown(event);
    }
}


GUIButton* SceneGUIElements::addButton(ULL ID,
                                       const stringImpl& text,
                                       const vec2<I32>& position,
                                       const vec2<U32>& dimensions,
                                       GUI::ButtonCallback callback,
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
        parent = GUI::instance()._rootSheet;
    }

    GUIButton* btn = MemoryManager_NEW GUIButton(ID, text, GUI::instance()._defaultGUIScheme, relOffset, relDim, parent, callback);

    addElement(ID, btn);

    return btn;
}

GUIMessageBox* SceneGUIElements::addMsgBox(ULL ID,
                                           const stringImpl& title,
                                           const stringImpl& message,
                                           const vec2<I32>& offsetFromCentre) {
    GUIMessageBox* box = MemoryManager_NEW GUIMessageBox(ID, title, message, offsetFromCentre, GUI::instance()._rootSheet);
    addElement(ID, box);

    return box;
}

GUIText* SceneGUIElements::addText(ULL ID,
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
                                           GUI::instance()._rootSheet,
                                           fontSize);

    addElement(ID, t);

    return t;
}

GUIFlash* SceneGUIElements::addFlash(ULL ID,
                                     stringImpl movie,
                                     const vec2<U32>& position,
                                     const vec2<U32>& extent) {

    GUIFlash* flash = MemoryManager_NEW GUIFlash(ID, GUI::instance()._rootSheet);
    addElement(ID, flash);

    return flash;
}

GUIText* SceneGUIElements::modifyText(ULL ID, const stringImpl& text) {
    GUI::GUIMap::iterator it = _sceneElements.find(ID);

    if (it == std::cend(_sceneElements)) {
        return nullptr;
    }

    GUIElement* element = it->second.first;
    assert(element->getType() == GUIType::GUI_TEXT);

    GUIText* textElement = dynamic_cast<GUIText*>(element);
    assert(textElement != nullptr);

    textElement->text(text);

    return textElement;
}

const vec2<U16>& SceneGUIElements::getDisplayResolution() const {
    return GUI::instance().getDisplayResolution();
}


GUIElement* SceneGUIElements::getGUIElementImpl(I64 sceneID, ULL elementName) const {
    GUIElement* ret = nullptr;
    GUI::GUIMap::const_iterator it = _sceneElements.find(elementName);
    if (it != std::cend(_sceneElements)) {
        ret = it->second.first;
    }
    

    return ret;
}

GUIElement* SceneGUIElements::getGUIElementImpl(I64 sceneID, I64 elementID) const {
    GUIElement* ret = nullptr;
    GUIElement* element = nullptr;
    for (const GUI::GUIMap::value_type& guiStackIterator : _sceneElements) {
        element = guiStackIterator.second.first;
        if (element->getGUID() == elementID) {
            ret = element;
            break;
        }
    }
    

    return ret;
}
}; // namespace Divide