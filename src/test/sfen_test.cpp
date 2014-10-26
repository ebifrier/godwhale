#include "precomp.h"

#if defined(USE_GTEST)
#include <gtest/gtest.h>
#include "stdinc.h"
#include "io_sfen.h"

namespace godwhale {
namespace test {

TEST (SfenTest, simpleMoveSfenTest)
{
    Position position;
    Move move;
    std::string sfen;

    move = Move::create(SQ(7,7), SQ(7,6), pawn, 0, false);
    sfen = moveToSfen(move);
    ASSERT_EQ("7g7f", sfen);
    ASSERT_EQ(move, sfenToMove(position, sfen));

    move = Move::create(SQ(2,8), SQ(5,8), rook, 0, false);
    sfen = moveToSfen(move);
    ASSERT_EQ("2h5h", moveToSfen(move));
    ASSERT_EQ(move, sfenToMove(position, sfen));
}

TEST (SfenTest, sfenToPositionTest)
{
    // 初期局面
    ASSERT_EQ(Position(), sfenToPosition(
        "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1"));

    // 最後の数字はなくてもよい
    ASSERT_EQ(Position(), sfenToPosition(
        "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b -"));

    // 持ち駒はないとダメ
    ASSERT_THROW(
        sfenToPosition("lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b"),
        SfenException);

    // 筋が少ない
    ASSERT_THROW(
        sfenToPosition("9/9/p/9/9/9/9/9/9 b -"),
        SfenException);

    // 筋が多い
   ASSERT_THROW(
        sfenToPosition("9/9/p9/9/9/9/9/9/9 b -"),
        SfenException);

    // 段が多い
    ASSERT_THROW(
        sfenToPosition("9/9/9/9/9/9/9/9/9/p b -"),
        SfenException);

    // 段が少ないのは問題にしない
    ASSERT_EQ(Position(false),
        sfenToPosition("9/9/9/9/9/9/9/9 b -"));

    // 成れない駒を成る
    ASSERT_THROW(
        sfenToPosition("9/9/9/9/9/9/9/+G8/9 b -"),
        SfenException);
}

static std::vector<Move> makeMoveList(std::string const & sfen)
{
    Position position;
    std::vector<std::string> list;

    boost::split(list, sfen, boost::is_any_of(" "));
    return sfenToMoveList(position, list);
}

TEST (SfenTest, sfenToMoveTest)
{
    // すべて正しい指し手
    auto moveList = makeMoveList("1g1f 4a3b 6i7h");
    ASSERT_EQ(3, moveList.size());

    // 2つめの指し手が正しくない
    moveList = makeMoveList("1g1f 4a8b 6i7h");
    ASSERT_EQ(1, moveList.size());
}

static void boardAndMoveTest(std::string const & boardSfen,
                             std::string const & moveSfen)
{
    // 指し手から局面を作ります。
    std::vector<std::string> moveSfenList;
    boost::split(moveSfenList, moveSfen, boost::is_any_of(" "));
    moveSfenList.erase(
        std::remove(moveSfenList.begin(), moveSfenList.end(), ""),
        moveSfenList.end());

    // 指し手を読み込みます。
    Position position1;
    auto moveList = sfenToMoveList(position1, moveSfenList);
    ASSERT_EQ(moveSfenList.size(), moveList.size());

    BOOST_FOREACH(auto m, moveList) {
        ASSERT_TRUE(position1.makeMove(m));
    }

    // 出力したSFEN形式の指し手が同じかどうかの確認もします。
    std::vector<std::string> result;
    BOOST_FOREACH(auto m, moveList) {
        //std::cout << result.size() << m.str() << std::endl;
        result.push_back(moveToSfen(m));
    }
    ASSERT_EQ(moveSfenList, result);


    // 局面を直接読み込みます。
    auto position2 = sfenToPosition(boardSfen);            

    // 出力したSFEN形式の局面が同じかどうかの確認もします。
    // (最後の数字には意味がないため、意図的に削っています)
    auto boardSfen2 = positionToSfen(position2);
    ASSERT_EQ(
        boardSfen.substr(0, boardSfen.length() - 1),
        boardSfen2.substr(0, boardSfen2.length() - 1));


    // ２つの局面を比較します。
    ASSERT_EQ(position1, position2);
}

TEST (SfenTest, complexTest)
{
    boardAndMoveTest(
        "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1",
        "");

    boardAndMoveTest(
        "l8/4+R4/1L2pgn2/1Ps1k2S1/P2p1SP2/LlP2b2p/ppNGP4/2S6/2KG5 w RBG6P2n2p 1",

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
        "6b5b 6c5b 5a5b P*5c S*5f L*8f 8i7i B*4f P*5g P*8g 5f4e");
}

}
}
#endif