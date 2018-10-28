#include "stdafx.h"

#include "Headers/AutoKeyRepeat.h"

namespace Divide {
namespace Input {

AutoRepeatKey::AutoRepeatKey(D64 repeatDelay, D64 initialDelay)
    : _repeatDelay(repeatDelay), 
      _initialDelay(initialDelay),
      _elapsed(0.0),
      _delay(initialDelay),
      _key(nullptr, 0)
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

// Inject key repeats if the _repeatDelay expired between calls
void AutoRepeatKey::update(const U64 deltaTimeUS) {
    if (_key._key == KeyCode::KC_UNASSIGNED) {
        return;
    }

    _elapsed += Time::MicrosecondsToSeconds(deltaTimeUS);
    if (_elapsed < _delay) return;

    _elapsed -= _delay;
    _delay = _repeatDelay;

    do {
        repeatKey(to_I32(_key._key), _key._text[0]);

        _elapsed -= _repeatDelay;
    } while (_elapsed >= _repeatDelay);

    _elapsed = 0.0;
}

};  // namespace Input
};  // namespace Divide
