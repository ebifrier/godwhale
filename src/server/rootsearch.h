
#ifndef GODWHALE_SERVER_ROOTSEARCH_H
#define GODWHALE_SERVER_ROOTSEARCH_H

#include "movenode.h"

namespace godwhale {
namespace server {

/**
 * @brief ルートからの探索を行い、その結果を保持するためのクラスです。
 */
class SearchData : private boost::noncopyable
{
public:
    explicit SearchData();

    int getIterationDepth() const
    {
        return m_iterationDepth;
    }

    int getValue() const
    {
        return m_value;
    }

    ULEType getULEType() const
    {
        return m_ule;
    }

    int getPVSize() const
    {
        return m_pv.size();
    }

    Move getPV(int ply) const
    {
        return m_pv[ply];
    }

    void reset(int iterationDepth, int value);

private:
    int m_iterationDepth;
    std::vector<Move> m_pv;

    int m_value;
    ULEType m_ule;

    /*// input
    int valid, issued;
    int haveList; // FIXME? need chkList? - do without it for now 10/30

    // out for ROOT, in for FIRST
    int opcode, fromRoot;

    // output
    int exd, bestseqLen;
    Move bestseq[BESTSEQ_LEN_MAX];*/
};

/**
 * @brief 最左ノードの探索結果を保持します。
 */
class FirstSearchData : private boost::noncopyable
{
public:
    explicit FirstSearchData();

    int getIterationDepth() const
    {
        return m_iterationDepth;
    }

    int getValue() const
    {
        return m_value;
    }

    ULEType getULEType() const
    {
        return m_ule;
    }

    int getPVSize() const
    {
        return m_pv.size();
    }

    Move getPV(int ply) const
    {
        return m_pv[ply];
    }

    void reset(int iterationDepth, int value);

private:
    int m_iterationDepth;
    std::vector<Move> m_pv;

    int m_value;
    ULEType m_ule;

    /*// input
    int valid, issued;
    int haveList; // FIXME? need chkList? - do without it for now 10/30

    // output
    int exd, bestseqLen;
    Move bestseq[BESTSEQ_LEN_MAX];*/
};

int generateRootMove(SearchData * result);

}
}

#endif
