/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INPUT_VARIABLES_H_
#define _INPUT_VARIABLES_H_

#include "resource.h"
#include "EventHandler.h"

//////////// Variable classes ////////////////////////////////////////////////////////

class Variable
{
  protected:

    double _dInitValue;
    double _dValue;

  public:

    Variable(double dInitValue) : _dInitValue(dInitValue) { reset(); }

    double getValue() const { return _dValue; }

    void reset() { _dValue = _dInitValue; }

    virtual void setValue(double dValue) { _dValue = dValue; }

    virtual std::string toString() const
    {
	  std::ostringstream oss;
	  oss << _dValue;
	  return oss.str();
	}

    virtual void update() {};
};

class Constant : public Variable
{
  public:

    Constant(double dInitValue) : Variable(dInitValue) {}

    virtual void setValue(double dValue) { }

};

class LimitedVariable : public Variable
{
  protected:

    double _dMinValue;
    double _dMaxValue;

  public:

    LimitedVariable(double dInitValue, double dMinValue, double dMaxValue) 
	: _dMinValue(dMinValue), _dMaxValue(dMaxValue), Variable(dInitValue)
    {}

    virtual void setValue(double dValue) 
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
	      << (int)(200.0 * getValue()/(_dMaxValue - _dMinValue)); // [-100%, +100%]
	  return oss.str();
	}*/
};

class TriangleVariable : public LimitedVariable
{
  protected:

    double _dDeltaValue;

  public:

    TriangleVariable(double dInitValue, double dDeltaValue, double dMinValue, double dMaxValue) 
	: LimitedVariable(dInitValue, dMinValue, dMaxValue), _dDeltaValue(dDeltaValue) {};

    virtual void update()
    {
	  double dValue = getValue() + _dDeltaValue;
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

typedef unordered_map<std::string, Variable*> MapVariables;
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

    std::string toString() const
    {
	  std::string str;
	  MapVariables::const_iterator iterVars;
	  for (iterVars = _mapVariables.begin(); iterVars != _mapVariables.end(); ++iterVars)
		str += iterVars->first + ":" + iterVars->second->toString() + " ";
	  return str;
	}
};

#endif