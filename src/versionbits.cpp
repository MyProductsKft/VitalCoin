// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <versionbits.h>
#include <consensus/params.h>
#include <consensus/consensus.h>
#include <util.h>

const struct VBDeploymentInfo VersionBitsDeploymentInfo[Consensus::MAX_VERSION_BITS_DEPLOYMENTS] = {
    {
        /*.name =*/"testdummy",
        /*.gbt_force =*/true,
    },
    {
        /*.name =*/"csv",
        /*.gbt_force =*/true,
    },
    {
        /*.name =*/"segwit",
        /*.gbt_force =*/true,
    }};


/*
 * Deployments for a fork that forks at block 2100 into a period of 1000 blocks will see retargeting at blocks:
 *      2016        (vitalcoin chain retargeting)
 *      3000        (divisible by 1000)
 *      4000        (divisible by 1000)
 *      5000        (divisible by 1000)
 *
 * For soft fork activations, if a soft fork is to activate 2018-01-01, signaling process will go as follows:
 *      2016 - 2999 (defined, but not started, because date has not been hit yet)
 *      3000 - 3999 (started, presumably all signal for, and soft fork activates)
 *      4000 - 4999 (locked in due to threshold achieved above)
 *      5000 -      (active)
 */
static inline const CBlockIndex* Rewind1Period(const CBlockIndex* pindex, int& nPeriod, int* nThreshold = nullptr, bool calibrate = false)
{
    int ancestor_height = pindex->nHeight - (calibrate ? ((pindex->nHeight + 1) % nPeriod) : nPeriod);
    if (nPeriod == PREFORK_PERIOD) {
        return pindex->GetAncestor(ancestor_height);
    }
    if (pindex->nHeight >= FORK_BLOCK && (pindex->nHeight <= nPeriod || pindex->nHeight - nPeriod < FORK_BLOCK)) {
        // We have cases where:
        //    fork block = 10, block = 2018
        // in which case we would skip over the 2016 period; we don't want to do that as it's a bit unintuitive, so
        // we test that
        //    height - ((height + 1) % period)
        // is indeed <= fork block
        int calib_height = pindex->nHeight - ((pindex->nHeight + 1) % nPeriod);
        if (calib_height <= FORK_BLOCK || ancestor_height < FORK_BLOCK) {
            // switch to pre-fork period
            nPeriod = PREFORK_PERIOD;
            if (nThreshold) *nThreshold = PREFORK_THRESHOLD;
            ancestor_height = FORK_BLOCK - ((FORK_BLOCK + 1) % nPeriod);
        }
    }
    return pindex->GetAncestor(ancestor_height);
}


ThresholdState AbstractThresholdConditionChecker::GetStateFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    int nPeriod = Period(pindexPrev, params);
    int nThreshold = Threshold(pindexPrev, params);

    int64_t nTimeStart = BeginTime(params);
    int64_t nTimeTimeout = EndTime(params);

    // Check if this deployment is always active.
    if (nTimeStart == Consensus::BIP9Deployment::ALWAYS_ACTIVE) {
        return ThresholdState::ACTIVE;
    }

    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    if (pindexPrev != nullptr) {
        pindexPrev = Rewind1Period(pindexPrev, nPeriod, &nThreshold, true);
    }

    // Walk backwards in steps of nPeriod to find a pindexPrev whose information is known
    std::vector<const CBlockIndex*> vToCompute;
    while (cache.count(pindexPrev) == 0) {
        if (pindexPrev == nullptr) {
            // The genesis block is by definition defined.
            cache[pindexPrev] = ThresholdState::DEFINED;
            break;
        }
        if (pindexPrev->GetMedianTimePast() < nTimeStart) {
            // Optimization: don't recompute down further, as we know every earlier block will be before the start time
            cache[pindexPrev] = ThresholdState::DEFINED;
            break;
        }
        vToCompute.push_back(pindexPrev);

        pindexPrev = Rewind1Period(pindexPrev, nPeriod, &nThreshold);
    }

    // At this point, cache[pindexPrev] is known
    assert(cache.count(pindexPrev));
    ThresholdState state = cache[pindexPrev];

    // Now walk forward and compute the state of descendants of pindexPrev

    bool pre_fork = nPeriod == PREFORK_PERIOD && nThreshold == PREFORK_THRESHOLD;

    while (!vToCompute.empty()) {
        ThresholdState stateNext = state;
        pindexPrev = vToCompute.back();
        vToCompute.pop_back();

        if (pre_fork && pindexPrev && pindexPrev->nHeight >= FORK_BLOCK) {
            pre_fork = false;
            nPeriod = Period(pindexPrev, params);
            nThreshold = Threshold(pindexPrev, params);
        }


        switch (state) {
        case ThresholdState::DEFINED: {
            if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                LogPrintf("    pindexPrev=%d, state:DEFINED -> FAILED (prev >= timeout)\n", pindexPrev->nHeight);
                stateNext = ThresholdState::FAILED;
            } else if (pindexPrev->GetMedianTimePast() >= nTimeStart) {
                LogPrintf("    pindexPrev=%d, state:DEFINED -> STARTED (prev >= start time) [%d, %d]\n", pindexPrev->nHeight, nPeriod, nThreshold);
                stateNext = ThresholdState::STARTED;
            }
            break;
        }
        case ThresholdState::STARTED: {
            if (pindexPrev->GetMedianTimePast() >= nTimeTimeout) {
                LogPrintf("    pindexPrev=%d, state:STARTED -> FAILED (prev >= timeout)\n", pindexPrev->nHeight);
                stateNext = ThresholdState::FAILED;
                break;
            }
            // We need to count
            const CBlockIndex* pindexCount = pindexPrev;
            int count = 0;
            for (int i = 0; i < nPeriod; i++) {
                if (Condition(pindexCount, params)) {
                    count++;
                }
                pindexCount = pindexCount->pprev;
            }
            if (count >= nThreshold) {
                LogPrintf("    pindexPrev=%d, state:STARTED -> LOCKED_IN (count >= threshold)\n", pindexPrev->nHeight);
                stateNext = ThresholdState::LOCKED_IN;
            }
            break;
        }
        case ThresholdState::LOCKED_IN: {
            // Always progresses into ACTIVE.
            LogPrintf("    pindexPrev=%d, state:LOCKED_IN -> ACTIVE\n", pindexPrev->nHeight);
            stateNext = ThresholdState::ACTIVE;
            break;
        }
        case ThresholdState::FAILED:
        case ThresholdState::ACTIVE: {
            // Nothing happens, these are terminal states.
            break;
        }
        }
        cache[pindexPrev] = state = stateNext;
    }

    return state;
}

// return the numerical statistics of blocks signalling the specified BIP9 condition in this current period
BIP9Stats AbstractThresholdConditionChecker::GetStateStatisticsFor(const CBlockIndex* pindex, const Consensus::Params& params) const
{
    BIP9Stats stats = {};


    stats.period = Period(pindex, params);
    stats.threshold = Threshold(pindex, params);


    if (pindex == nullptr)
        return stats;

    // Find beginning of period

    const CBlockIndex* pindexEndOfPrevPeriod = Rewind1Period(pindex, stats.period, &stats.threshold, true);

    stats.elapsed = pindex->nHeight - pindexEndOfPrevPeriod->nHeight;

    // Count from current block to beginning of period
    int count = 0;
    const CBlockIndex* currentIndex = pindex;
    while (pindexEndOfPrevPeriod->nHeight != currentIndex->nHeight) {
        if (Condition(currentIndex, params))
            count++;
        currentIndex = currentIndex->pprev;
    }

    stats.count = count;
    stats.possible = (stats.period - stats.threshold) >= (stats.elapsed - count);

    return stats;
}

int AbstractThresholdConditionChecker::GetStateSinceHeightFor(const CBlockIndex* pindexPrev, const Consensus::Params& params, ThresholdConditionCache& cache) const
{
    int64_t start_time = BeginTime(params);
    if (start_time == Consensus::BIP9Deployment::ALWAYS_ACTIVE) {
        return 0;
    }

    const ThresholdState initialState = GetStateFor(pindexPrev, params, cache);

    // BIP 9 about state DEFINED: "The genesis block is by definition in this state for each deployment."
    if (initialState == ThresholdState::DEFINED) {
        return 0;
    }


    int nPeriod = Period(pindexPrev, params);


    // A block's state is always the same as that of the first of its period, so it is computed based on a pindexPrev whose height equals a multiple of nPeriod - 1.
    // To ease understanding of the following height calculation, it helps to remember that
    // right now pindexPrev points to the block prior to the block that we are computing for, thus:
    // if we are computing for the last block of a period, then pindexPrev points to the second to last block of the period, and
    // if we are computing for the first block of a period, then pindexPrev points to the last block of the previous period.
    // The parent of the genesis block is represented by nullptr.

    pindexPrev = Rewind1Period(pindexPrev, nPeriod, nullptr, true);

    const CBlockIndex* previousPeriodParent = Rewind1Period(pindexPrev, nPeriod);


    while (previousPeriodParent != nullptr && GetStateFor(previousPeriodParent, params, cache) == initialState) {
        pindexPrev = previousPeriodParent;

        previousPeriodParent = Rewind1Period(pindexPrev, nPeriod);
    }

    // Adjust the result because right now we point to the parent block.
    return pindexPrev->nHeight + 1;
}

namespace {
/**
 * Class to implement versionbits logic.
 */
class VersionBitsConditionChecker : public AbstractThresholdConditionChecker
{
private:
    const Consensus::DeploymentPos id;

protected:
    int64_t BeginTime(const Consensus::Params& params) const override { return params.vDeployments[id].nStartTime; }
    int64_t EndTime(const Consensus::Params& params) const override { return params.vDeployments[id].nTimeout; }

    int Period(const CBlockIndex* pindex, const Consensus::Params& params) const override { return !pindex || pindex->nHeight <= FORK_BLOCK ? PREFORK_PERIOD : fork_conforksus.pow_target_timespan / fork_conforksus.pow_target_spacing; }
    int Threshold(const CBlockIndex* pindex, const Consensus::Params& params) const override { return !pindex || pindex->nHeight <= FORK_BLOCK ? PREFORK_THRESHOLD : ((fork_conforksus.pow_target_timespan / fork_conforksus.pow_target_spacing) * 95) / 100; }


    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const override
    {
        return (((pindex->nVersion & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS) && (pindex->nVersion & Mask(params)) != 0);
    }

public:
    explicit VersionBitsConditionChecker(Consensus::DeploymentPos id_) : id(id_) {}
    uint32_t Mask(const Consensus::Params& params) const { return ((uint32_t)1) << params.vDeployments[id].bit; }
};

} // namespace

ThresholdState VersionBitsState(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache)
{
    return VersionBitsConditionChecker(pos).GetStateFor(pindexPrev, params, cache.caches[pos]);
}

BIP9Stats VersionBitsStatistics(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    return VersionBitsConditionChecker(pos).GetStateStatisticsFor(pindexPrev, params);
}

int VersionBitsStateSinceHeight(const CBlockIndex* pindexPrev, const Consensus::Params& params, Consensus::DeploymentPos pos, VersionBitsCache& cache)
{
    return VersionBitsConditionChecker(pos).GetStateSinceHeightFor(pindexPrev, params, cache.caches[pos]);
}

uint32_t VersionBitsMask(const Consensus::Params& params, Consensus::DeploymentPos pos)
{
    return VersionBitsConditionChecker(pos).Mask(params);
}

void VersionBitsCache::Clear()
{
    for (unsigned int d = 0; d < Consensus::MAX_VERSION_BITS_DEPLOYMENTS; d++) {
        caches[d].clear();
    }
}
