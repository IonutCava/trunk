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

#ifndef _MEMORY_TRACKER_H_
#define _MEMORY_TRACKER_H_

#include "Core/Math/Headers/MathHelper.h"
#include "Hardware/Platform/Headers/SharedMutex.h"
#include <atomic>

namespace Divide {
    namespace MemoryManager {
        /// From https://www.relisoft.com/book/tech/9new.html
        class MemoryTracker {
        private:
            class Entry	{
            public:
                Entry( char const * file, I32 line, size_t size ) : _file( file ), _line( line ), _size( size ) {
                }
                Entry() : _file( "" ), _line( 0 ), _size( 0 ) {
                }

                inline char const * File() const { return _file; }
                inline I32 Line()          const { return _line; }
                inline size_t Size()       const { return _size; }

            private:
                char const * _file;
                I32 _line;
                size_t _size;
            };

            class Lock {
            public:
                Lock( MemoryTracker & tracer ) : _tracer( tracer ) {
                    _tracer.lock();
                }
                ~Lock() {
                    _tracer.unlock();
                }
            private:
                MemoryTracker& _tracer;
            };

            typedef hashMapImpl<void *, Entry>::iterator iterator;
            friend class Lock;

        public:
            MemoryTracker() {
                _locked = false;
                Ready = true;
            }

            ~MemoryTracker() {
                Ready = false;
                bool leakDetected = false;
                size_t sizeLeaked = 0;
                Dump( leakDetected, sizeLeaked );
            }

            inline void Add( void * p, size_t size, char const * file, I32 line ) {
                if ( _locked ) {
                    return;
                } 
				WriteLock w_lock(_mutex);
                MemoryTracker::Lock lock( *this );
                hashAlg::insert( _map, hashAlg::makePair(p, Entry( file, line, size )) );
            }

            inline void Remove( void * p ) {
                if ( _locked ) {
                    return;
                }

				WriteLock w_lock(_mutex);
                MemoryTracker::Lock lock( *this );
                iterator it = _map.find( p );
                if ( it != _map.end() ) {
                    _map.erase( it );
                }
            }

            inline stringImpl Dump( bool& leakDetected, size_t& sizeLeaked ) {
                stringImpl output;
                sizeLeaked = 0;
                leakDetected = false;

                MemoryTracker::Lock lock( *this );
                WriteLock w_lock( _mutex );
                if ( !_map.empty() ) {
                    output.append( stringAlg::toBase( Util::toString( _map.size() ) + " memory leaks detected\n" ) );
                    for ( iterator it = _map.begin(); it != _map.end(); ++it ) {
                        const Entry& entry = it->second;
                        output.append( entry.File() );
                        output.append( ", " );
                        output.append( stringAlg::toBase( Util::toString( entry.Line() ) ) );
                        output.append( "\n" );
                        sizeLeaked += entry.Size();
                    }
                    leakDetected = true;
                    _map.clear();
                }
                return output;
            }

            static bool Ready;

        private:
            inline void lock() { _locked = true; }
            inline void unlock() { _locked = false; }

        private:
            mutable SharedLock _mutex;
            hashMapImpl<void *, Entry> _map;
            std::atomic_bool _locked;
        };

        extern MemoryTracker AllocTracer;
    }; //namespace MemoryManager
}; //namespace Divide
#endif //_MEMORY_TRACKER_H_