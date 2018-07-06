/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _INPUT_VARIABLES_H_
#define _INPUT_VARIABLES_H_

#include "core.h"
#include "EventHandler.h"
namespace Divide {
    namespace Input {
//////////// Variable classes ////////////////////////////////////////////////////////

class Variable
{
  protected:

    D32 _dInitValue;
    D32 _dValue;

  public:

    Variable(D32 dInitValue) : _dInitValue(dInitValue) { reset(); }

    D32 getValue() const { return _dValue; }

    void reset() { _dValue = _dInitValue; }

    virtual void setValue(D32 dValue) { _dValue = dValue; }

    virtual stringImpl toString() const
    {
      std::ostringstream oss;
      oss << _dValue;
      return stringAlg::toBase(oss.str());
    }

    virtual void update() {};
};

class Constant : public Variable
{
  public:

    Constant(D32 dInitValue) : Variable(dInitValue) {}

    virtual void setValue(D32 dValue) { }
};

class LimitedVariable : public Variable
{
  protected:

    D32 _dMinValue;
    D32 _dMaxValue;

  public:

    LimitedVariable(D32 dInitValue, D32 dMinValue, D32 dMaxValue)
    : _dMinValue(dMinValue), _dMaxValue(dMaxValue), Variable(dInitValue)
    {}

    virtual void setValue(D32 dValue)
    {
      _dValue = dValue;
      if (_dValue > _dMaxValue)
        _dValue = _dMaxValue;
      else if (_dValue < _dMinValue)
        _dValue = _dMinValue;
    }

/*    virtual string toString() const
    {
      ostringstream oss;
      oss << setiosflags(ios_base::right) << setw(4)
          << (I32)(200.0 * getValue()/(_dMaxValue - _dMinValue)); // [-100%, +100%]
      return oss.str();
    }*/
};

class TriangleVariable : public LimitedVariable
{
  protected:

    D32 _dDeltaValue;

  public:

    TriangleVariable(D32 dInitValue, D32 dDeltaValue, D32 dMinValue, D32 dMaxValue)
    : LimitedVariable(dInitValue, dMinValue, dMaxValue), _dDeltaValue(dDeltaValue) {};

    virtual void update()
    {
      D32 dValue = getValue() + _dDeltaValue;
      if (dValue > _dMaxValue)
      {
        dValue = _dMaxValue;
        _dDeltaValue = -_dDeltaValue;
        //cout << "Decreasing variable towards " << _dMinValue << endl;
      }
      else if (dValue < _dMinValue)
      {
        dValue = _dMinValue;
        _dDeltaValue = -_dDeltaValue;
        //cout << "Increasing variable towards " << _dMaxValue << endl;
      }
      setValue(dValue);
      //cout << "TriangleVariable::update : delta=" << _dDeltaValue << ", value=" << dValue << endl;
    }
};

//////////// Variable effect class //////////////////////////////////////////////////////////

typedef hashMapImpl<stringImpl, Variable*> MapVariables;
typedef void (*EffectVariablesApplier)(MapVariables& mapVars, OIS::Effect* pEffect);

class VariableEffect
{
  protected:

    // Effect description
    const char* _pszDesc;

    // The associate OIS effect
    OIS::Effect* _pEffect;

    // The effect variables.
    MapVariables _mapVariables;

    // The effect variables applier function.
    EffectVariablesApplier _pfApplyVariables;

    // True if the effect is currently being played.
    bool _bActive;

  public:

    VariableEffect(const char* pszDesc, OIS::Effect* pEffect,
                   const MapVariables& mapVars, const EffectVariablesApplier pfApplyVars)
    : _pszDesc(pszDesc), _pEffect(pEffect),
      _mapVariables(mapVars), _pfApplyVariables(pfApplyVars), _bActive(false)
    {}

    ~VariableEffect()
    {
      SAFE_DELETE(_pEffect);
      MapVariables::iterator iterVars;
      for (iterVars = _mapVariables.begin(); iterVars != _mapVariables.end(); ++iterVars)
          SAFE_DELETE(iterVars->second);
    }

    inline void setActive(bool bActive = true) { reset(); _bActive = bActive; }
    inline bool isActive(){  return _bActive; }
    inline OIS::Effect* getFFEffect()  { return _pEffect; }

    const char* getDescription() const { return _pszDesc; }

    void update()
    {
      if (isActive())
      {
        // Update the variables.
        MapVariables::iterator iterVars;
        for (iterVars = _mapVariables.begin(); iterVars != _mapVariables.end(); ++iterVars)
          iterVars->second->update();

        // Apply the updated variable values to the effect.
        _pfApplyVariables(_mapVariables, _pEffect);
      }
    }

    void reset()
    {
      MapVariables::iterator iterVars;
      for (iterVars = _mapVariables.begin(); iterVars != _mapVariables.end(); ++iterVars)
        iterVars->second->reset();
      _pfApplyVariables(_mapVariables, _pEffect);
    }

    stringImpl toString() const
    {
      stringImpl str;
      MapVariables::const_iterator iterVars;
      for (iterVars = _mapVariables.begin(); iterVars != _mapVariables.end(); ++iterVars)
        str += iterVars->first + ":" + iterVars->second->toString() + " ";
      return str;
    }
};

    }; //namespace Input
}; //namespace Divide
#endif