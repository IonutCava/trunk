#include "stdafx.h"
///-------------------------------------------------------------------------------------------------
/// File:	src\Event\IEvent.cpp.
///
/// Summary:	Declares the IEvent interface.
///-------------------------------------------------------------------------------------------------


#include "Event/IEvent.h"

#include "Engine.h"
#include "util/Timer.h"

namespace ECS { namespace Event {

	IEvent::IEvent(ECSEngine* engine, EventTypeId typeId) :
		m_TypeId(typeId)
	{

        assert(engine != nullptr && "ECS engine not initialized!");
		this->m_TimeCreated = engine->ECS_EngineTime->GetTimeStamp();
	}

}} // namespace ECS::Event
