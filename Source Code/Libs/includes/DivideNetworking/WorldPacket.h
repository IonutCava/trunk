#ifndef MANGOSSERVER_WORLDPACKET_H
#define MANGOSSERVER_WORLDPACKET_H

#include "ByteBuffer.h"
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace Divide {

class WorldPacket : public ByteBuffer
{
    public:
                                                            // just container for later use
        WorldPacket() : ByteBuffer(0), m_opcode(0)
        {
        }
        explicit WorldPacket(U16 opcode, size_t res=200) : ByteBuffer(res), m_opcode(opcode) { }
                                                            // copy constructor
        WorldPacket(const WorldPacket &packet) : ByteBuffer(packet), m_opcode(packet.m_opcode)
        {
        }

        void Initialize(U16 opcode, size_t newres=200)
        {
            clear();
            _storage.reserve(newres);
            m_opcode = opcode;
        }

        U16 getOpcode() const { return m_opcode; }

        void SetOpcode(U16 opcode) { m_opcode = opcode; }

        template<typename Archive>
        void serialize(Archive& ar,  unsigned int version  )
        {
            ar & _rpos;
            ar & _wpos;
            ar & m_opcode;
            ar & _storage;
        }

   protected:
        U16 m_opcode;
};

}; //namespace Divide

//Remove Archive header / Warning: no more version updates ... hmmm
BOOST_CLASS_IMPLEMENTATION(Divide::WorldPacket, boost::serialization::object_serializable);
//Remove pointer tracking and duplication checking. Leave it up to me, why not? 8-|
BOOST_CLASS_TRACKING(Divide::WorldPacket, boost::serialization::track_never)
#endif