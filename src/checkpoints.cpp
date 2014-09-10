// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (     0, uint256("0xd58888d61baa00c7acd6388356e449ca5d54beacab38361fd48cad0012959a2d"))
        (     1, uint256("0xc8b400c4920f8fac074a2c0ee9dfbe1238ae8d1058f671ca2a687dce3f020bd3"))
        (     2, uint256("0x2bb1ba9134dc3586e4ed530f93ceed6bbafddbf0101209dcad0a04e39cf306d2"))
        (     4, uint256("0x1a25b800b63ce88712c6922b1250650fdedfa69dd4bdc7ce9416f91fa0509656"))
        (     8, uint256("0x03a74ca916a18c70269295c5dc99cd8324493a0f8a70e197b08940b1a02a4d97"))
        (    16, uint256("0x49787d2751d24fa66c9c951a2b6a9fd74e5fab31220324bc7e88c82ff1f6276a"))
        (    32, uint256("0x3e8440b622dfe14d0b31d3ee89743fb4e0461c83fc7736f438ee2a4f58f2c09a"))
        (    64, uint256("0x049c83dddabb4348cba5b2d33d0983cd1f9f45031dd92f69ecf28720fe768df7"))
        (   128, uint256("0xc95b575778f4521e3bfa193396303a076a9d10ec026cc5e24c04d8c1bbc006bb"))
        (   256, uint256("0xeaa0c8504f7072cedfdc9304629ebead05eb2489ea965ddad7647820a6294bc3"))
        (   512, uint256("0x2f1f852dcb7646424f211f289f01cc494da867899f0a57b6cc7bd6caa1bc78ef"))
        (  1024, uint256("0xda3ea3b06fec5d586f31b24cb351e3f7f364a2da89731242c7c46190d1494ff2"))
        (  2048, uint256("0xb073e6cf24e3351b6e48dcd38a3739f51465799a3463cc168bf12fe87b36fb14"))
        (  4096, uint256("0x9ae88c13647e26a7a2b6fda70375a38564af6bbdfd3d95e54a4df3f6ad4a4935"))
        (  8192, uint256("0x9576bdfb790f80043f014235d8f803fa0fb1aabd0c3cfea6c87fefed56f56a8a"))
        ( 16384, uint256("0x2c6722c5d7359779e8e564b34358561e734453c414241276440a9ac1bbaa2694"))
        ( 32768, uint256("0x514895ae1a7ff8cb57436fb4ee543f6d6561b21d327e3735403652c300764a1f"))
        ( 61712, uint256("0x302391a7f45300cfd9b40e25526f194206ed05b7193126a0419da5b7c01d746c"))
        
        ;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1410303386, // * UNIX timestamp of last checkpoint block
        62071,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        100.0     // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        (   546, uint256("0xa0fea99a6897f531600c8ae53367b126824fd6a847b2b2b73817a95b8e27e602"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1365458829,
        547,
        576
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
