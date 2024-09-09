#pragma once

#include <stdio.h>
#include <mutex>

#ifdef WV_PLATFORM_WINDOWS
#include <Windows.h>
#undef min // these need to be undefined to keep ::min and ::max functions from not dying...
#undef max 
#endif

///////////////////////////////////////////////////////////////////////////////////////

namespace wv
{
	namespace Debug
	{

		namespace Internal
		{

	///////////////////////////////////////////////////////////////////////////////////////

			static const char* LEVEL_STR[] = {
				"          ", // info
				"DEBUG",
				" WARN",
				"ERROR",
				"FATAL",
				"TRACE",
				" GPU "
			};

			static int LEVEL_COL[] = {
				7,   // info
				11,  // debug
				14,  // warning
				4,   // error
				12,  // fatal
				13,  // trace
				13  // render call
			};

		#ifdef WV_PLATFORM_WINDOWS
			static HANDLE hConsole = nullptr;

			static std::mutex PRINT_MUTEX;
		#endif

			struct sPrintEnabled
			{
				static inline bool TRACE = false;
				static inline bool RENDER = false;
			};

	///////////////////////////////////////////////////////////////////////////////////////

			static inline void SetPrintLevelColor( int _level )
			{
			#ifdef WV_PLATFORM_WINDOWS
				if( !hConsole )
					hConsole = GetStdHandle( STD_OUTPUT_HANDLE );

				if( _level < 0 )
					_level = 0; // default print

				SetConsoleTextAttribute( hConsole, LEVEL_COL[ _level ] );
			#endif
			}
		}

	///////////////////////////////////////////////////////////////////////////////////////

		enum PrintLevel
		{
			WV_PRINT_INFO,
			WV_PRINT_DEBUG,
			WV_PRINT_WARN,
			WV_PRINT_ERROR,
			WV_PRINT_FATAL,
			WV_PRINT_TRACE,
			WV_PRINT_RENDER
		};

	///////////////////////////////////////////////////////////////////////////////////////

		static void SetRenderPrints( bool _enabled ) { Internal::sPrintEnabled::RENDER = _enabled; }
		static void SetTracePrints ( bool _enabled ) { Internal::sPrintEnabled::TRACE  = _enabled; }

		template<typename... Args>
		inline void Print( const char* _str, Args... _args )
		{
		#ifdef WV_PLATFORM_WINDOWS
			Internal::PRINT_MUTEX.lock();
		#endif
			printf( "%s", Internal::LEVEL_STR[ 0 ] );
			printf( _str, _args... );

		#ifdef WV_PLATFORM_WINDOWS
			Internal::PRINT_MUTEX.unlock();
		#endif
		}

		template<typename... Args>
		inline void Print( PrintLevel _printLevel, const char* _str, Args... _args )
		{
		#ifdef WV_PLATFORM_WINDOWS
			Internal::PRINT_MUTEX.lock();
		#endif

			
			bool skip;
			skip |= _printLevel == WV_PRINT_RENDER && !Debug::Internal::sPrintEnabled::RENDER;
			skip |= _printLevel == WV_PRINT_TRACE  && !Debug::Internal::sPrintEnabled::TRACE;
		#ifndef WV_DEBUG
			skip |= _printLevel == WV_PRINT_DEBUG;
		#endif

			if( skip )
			{
			#ifdef WV_PLATFORM_WINDOWS
				Internal::PRINT_MUTEX.unlock();
			#endif
				return;
			}

			if( _printLevel != WV_PRINT_INFO )
			{
				printf( "[ " );
				Internal::SetPrintLevelColor( _printLevel );
				printf( Internal::LEVEL_STR[ _printLevel ] );
				Internal::SetPrintLevelColor( -1 );
				printf( " ] " );
			}
			else
				printf( Internal::LEVEL_STR[ _printLevel ] );

			printf( _str, _args... );

		#ifdef WV_PLATFORM_WINDOWS
			Internal::PRINT_MUTEX.unlock();
		#endif
		}

	#define WV_RENDER_PRINT() wv::Debug::Print( wv::Debug::WV_PRINT_RENDER, "%s\n", __FUNCTION__ )
	#define WV_TRACE_PRINT()  wv::Debug::Print( wv::Debug::WV_PRINT_TRACE, "%s\n", __FUNCTION__ )

	}
}


