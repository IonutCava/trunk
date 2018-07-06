#include "Headers\AutoKeyRepeat.h"
#include "Hardware/Input/Headers/InputInterface.h"

namespace Divide {
    namespace Input {

AutoRepeatKey::AutoRepeatKey(D32 repeatDelay, D32 initialDelay):
    _repeatDelay(repeatDelay),
    _initialDelay(initialDelay)
{
}

void AutoRepeatKey::begin(const KeyEvent &evt) {
    _key = evt;
    _elapsed = 0.0;
    _delay = _initialDelay;
}

void AutoRepeatKey::end(const KeyEvent &evt) {
    if (_key._text != evt._text) {
        return;
    }

    _key._key = KeyCode::KC_UNASSIGNED;
}

//Inject key repeats if the _repeatDelay expired between calls
void AutoRepeatKey::update(const U64 deltaTime) {
    if (_key._key == KeyCode::KC_UNASSIGNED) {
        return;
    }
    _elapsed += (deltaTime * 0.000001); //< use seconds
    if (_elapsed < _delay) return;

    _elapsed -= _delay;
    _delay = _repeatDelay;

    do {
        repeatKey(_key._key, _key._text);

        _elapsed -= _repeatDelay;
    } while (_elapsed >= _repeatDelay);

    _elapsed = 0.0;
}

    }; //namespace Input
}; //namespace Divide