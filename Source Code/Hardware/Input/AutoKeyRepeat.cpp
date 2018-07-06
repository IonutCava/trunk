#include "Headers\AutoKeyRepeat.h"

AutoRepeatKey::AutoRepeatKey(F32 repeatDelay, F32 initialDelay):
    _key(OIS::KC_UNASSIGNED),
    _repeatDelay(repeatDelay),
    _initialDelay(initialDelay)
{
}

void AutoRepeatKey::begin(const OIS::KeyEvent &evt) {
    _key = evt.key;
    _char = evt.text;
 
    _elapsed = 0;
    _delay = _initialDelay;
}

void AutoRepeatKey::end(const OIS::KeyEvent &evt) {
    if (_key != evt.key) return;
 
    _key = OIS::KC_UNASSIGNED;
}
 
//Inject key repeats if the _repeatDelay expired between calls
void AutoRepeatKey::update(F32 elapsed) {
    if (_key == OIS::KC_UNASSIGNED) return;
 
    _elapsed += elapsed;
    if (_elapsed < _delay) return;
 
    _elapsed -= _delay;
    _delay = _repeatDelay;
 
    do {
        repeatKey(_key, _char);
 
        _elapsed -= _repeatDelay;
    } while (_elapsed >= _repeatDelay);
 
    _elapsed = 0;
}