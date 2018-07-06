/*
	Author : Tobias Stein
	Date   : 11th September, 2016
	File   : Logger.h
	
	Class that manages the logging.

	All Rights Reserved. (c) Copyright 2016.
*/

#if !defined(ECS_DISABLE_LOGGING)

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "Platform.h"

#include "Core/Headers/Console.h"

namespace ECS { namespace Log {

		class ECS_API Logger
		{		
			Logger(const Logger&) = delete;
			Logger& operator=(Logger&) = delete;
					

		public:

			explicit Logger();

			~Logger();

			// trace 
			template<typename... Args>
			inline void LogTrace(const char* fmt, Args... args)
			{			
                Divide::Console::printf(fmt, std::forward<Args>(args)...);
			}

			// debug
			template<typename... Args>
			inline void LogDebug(const char* fmt, Args... args)
			{
                Divide::Console::d_printf(fmt, std::forward<Args>(args)...);
			}

			// info
			template<typename... Args>
			inline void LogInfo(const char* fmt, Args... args)
			{
                Divide::Console::printf(fmt, std::forward<Args>(args)...);
			}

			// warn
			template<typename... Args>
			inline void LogWarning(const char* fmt, Args... args)
			{
                Divide::Console::warnf(fmt, std::forward<Args>(args)...);
			}

			// error
			template<typename... Args>
			inline void LogError(const char* fmt, Args... args)
			{
                Divide::Console::errorf(fmt, std::forward<Args>(args)...);
			}

			// fatal
			template<typename... Args>
			inline void LogFatal(const char* fmt, Args... args)
			{
                Divide::Console::errorf(fmt, std::forward<Args>(args)...);
                assert(false && "Fatal Error");
			}

		}; // class Logger


}} // namespace ECS::Log


#include "Log/LoggerMacro.h"


#endif // __LOGGER_H__
#endif // !ECS_DISABLE_LOGGING