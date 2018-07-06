#ifndef MANGOSSERVER_WORLDPACKET_H
#define MANGOSSERVER_WORLDPACKET_H

#include "ByteBuffer.h"
#include <boost/serialization/base_object.hpp>

namespace Divide {

class WorldPacket : public ByteBuffer {
   public:
    // just container for later use
    WorldPacket() : ByteBuffer(0), m_opcode(0) {}
    explicit WorldPacket(U16 opcode, size_t res = 200)
        : ByteBuffer(res), m_opcode(opcode) {}
    // copy constructor
    WorldPacket(const WorldPacket& packet)
        : ByteBuffer(packet), m_opcode(packet.m_opcode) {}

    void Initialize(U16 opcode, size_t newres = 200) {
        clear();
        _storage.reserve(newres);
        m_opcode = opcode;
    }

    U16 getOpcode() const { return m_opcode; }

    void SetOpcode(U16 opcode) { m_opcode = opcode; }

   private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        /*ar & boost::serialization::base_object<ByteBuffer>(*this);
        ar & m_opcode;*/
    }

   protected:
    U16 m_opcode;
};

};  // namespace Divide

#endif
