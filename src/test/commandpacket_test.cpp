#include "precomp.h"

#if defined(USE_GTEST)
#include <gtest/gtest.h>
#include "stdinc.h"
#include "io_sfen.h"
#include "commandpacket.h"

namespace godwhale {
namespace test {

TEST (CommandPacketTest, setPositionTest)
{
    shared_ptr<CommandPacket> command;
    std::string commandStr, sfen, sfen0;
    
    //
    commandStr = "setposition 10 startpos";
    command = CommandPacket::parse(commandStr);

    ASSERT_EQ(10, command->getPositionId());
    ASSERT_EQ(Position(), command->getPosition());
    ASSERT_EQ(commandStr, command->toRsi());

    //
    sfen = "l8/4+R4/1L2pgn2/1Ps1k2S1/P2p1SP2/LlP2b2p/ppNGP4/2S6/2KG5 w RBG6P2n2p 1";
    commandStr = "setposition 0 sfen " + sfen;
    command = CommandPacket::parse(commandStr);

    ASSERT_EQ(0, command->getPositionId());
    ASSERT_EQ(sfenToPosition(sfen), command->getPosition());
    ASSERT_EQ(commandStr, command->toRsi());

    //
    std::string moves =
        "7g7f 3c3d 2g2f 2b3c 8h3c+ 2a3c 5i6h 8b4b 6h7h 5a6b "
        "7h8h 6b7b 3i4h 2c2d 7i7h 4b2b 4i5h 3a3b 6g6f 7b8b "
        "9g9f 9c9d 5h6g 7a7b 3g3f 2d2e 2f2e 2b2e P*2f 2e2a "
        "4g4f 4a5b 4h4g 4c4d 2i3g B*1e 4g3h 1e2f 2h2i 6c6d "
        "8i7g 1c1d 1g1f 3b4c 8g8f P*2e 2i2g 2a2d 3h4g 4c5d "
        "5g5f 5d6c 5f5e 8c8d 6f6e 6d6e B*3b 6c7d 3b4a+ 6a5a "
        "4a3a 3d3e 5e5d 5c5d 3a3b 5a4b 3b5d 5b5c 5d5e 4b4c "
        "3f3e 9d9e 9f9e P*9f 9i9f 5c5d 5e5f P*9g 4g3f 8d8e "
        "8f8e 4d4e 4f4e 1d1e 1f1e 1a1e P*1f 1e1f 1i1f P*1e "
        "P*6c 7b6c 8e8d 1e1f L*8c 8b7b 3f2e 2f3g+ 2e2d 3g2g "
        "R*8b 7b6a 8b8a+ 6a5b N*5e R*9h 8h7i 5d5e 5f5e 2g4e "
        "5e7c 9h9i+ G*8i 9i8i 7i8i 5b5c 8a5a G*5b 7c6b 5c5d "
        "6b5b 6c5b 5a5b P*5c S*5f L*8f 8i7i B*4f P*5g P*8g 5f4e";
    sfen0 = "l8/4+R4/1L2pgn2/1Ps1k2S1/P2p1SP2/LlP2b2p/ppNGP4/2S6/2KG5 w RBG6P2n2p 1";

    commandStr = "setposition 0 startpos moves " + moves;
    command = CommandPacket::parse(commandStr);

    ASSERT_EQ(0, command->getPositionId());
    ASSERT_EQ(sfenToPosition(sfen0), command->getPosition());
    ASSERT_EQ(commandStr, command->toRsi());
}

TEST (CommandPacketTest, setPositionErrorTest)
{
    ASSERT_ANY_THROW(CommandPacket::parse("setposition sfen"));
    //ASSERT_ANY_THROW(CommandPacket::parse("setposition 0 sfen"));
    //ASSERT_ANY_THROW(CommandPacket::parse("setposition 0 odidkdkd"));
}

}
}
#endif
