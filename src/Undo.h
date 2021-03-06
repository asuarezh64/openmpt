/*
 * Undo.h
 * ------
 * Purpose: Header file for undo functionality
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 */

#pragma once

#include "tracker/tracker.hpp"

#define MAX_UNDO_LEVEL 1000    // 1000 undo steps for each undo type!

/////////////////////////////////////////////////////////////////////////////////////////
// Pattern Undo

struct PATTERNUNDOBUFFER
{
    modplug::tracker::patternindex_t pattern;
    modplug::tracker::rowindex_t patternsize;
    modplug::tracker::chnindex_t firstChannel, numChannels;
    modplug::tracker::rowindex_t firstRow, numRows;
    modplug::tracker::modevent_t *pbuffer;
    bool linkToPrevious;
};

//================
class CPatternUndo
//================
{

private:

    std::vector<PATTERNUNDOBUFFER> UndoBuffer;
    CModDoc *m_pModDoc;

    // Pattern undo helper functions
    void DeleteUndoStep(const UINT nStep);
    modplug::tracker::patternindex_t Undo(bool linkedFromPrevious);

public:

    // Pattern undo functions
    void ClearUndo();
    bool PrepareUndo(modplug::tracker::patternindex_t pattern, modplug::tracker::chnindex_t firstChn, modplug::tracker::rowindex_t firstRow, modplug::tracker::chnindex_t numChns, modplug::tracker::rowindex_t numRows, bool linkToPrevious = false);
    modplug::tracker::patternindex_t Undo();
    bool CanUndo();
    void RemoveLastUndoStep();

    void SetParent(CModDoc *pModDoc) {m_pModDoc = pModDoc;}

    CPatternUndo()
    {
            UndoBuffer.clear();
            m_pModDoc = nullptr;
    };
    ~CPatternUndo()
    {
            ClearUndo();
    };

};



/////////////////////////////////////////////////////////////////////////////////////////
// Sample Undo

// We will differentiate between different types of undo actions so that we don't have to copy the whole sample everytime.
enum sampleUndoTypes
{
    sundo_none,                // no changes to sample itself, e.g. loop point update
    sundo_update,        // silence, amplify, normalize, dc offset - update complete sample section
    sundo_delete,        // delete part of the sample
    sundo_invert,        // invert sample phase, apply again to undo
    sundo_reverse,        // reverse sample, dito
    sundo_unsign,        // unsign sample, dito
    sundo_insert,        // insert data, delete inserted data to undo
    sundo_replace,        // replace complete sample (16->8Bit, up/downsample, downmix to mono, pitch shifting / time stretching, trimming, pasting)
};

struct SAMPLEUNDOBUFFER
{
    modplug::tracker::modsample_t OldSample;
    CHAR szOldName[MAX_SAMPLENAME];
    LPSTR SamplePtr;
    UINT nChangeStart, nChangeEnd;
    sampleUndoTypes nChangeType;
};

//===============
class CSampleUndo
//===============
{

private:

    // Undo buffer
    std::vector<std::vector<SAMPLEUNDOBUFFER> > UndoBuffer;
    CModDoc *m_pModDoc;

    // Sample undo helper functions
    void DeleteUndoStep(const modplug::tracker::sampleindex_t nSmp, const UINT nStep);
    bool SampleBufferExists(const modplug::tracker::sampleindex_t nSmp, bool bForceCreation = true);
    void RestrictBufferSize();
    UINT GetUndoBufferCapacity();

public:

    // Sample undo functions
    void ClearUndo();
    void ClearUndo(const modplug::tracker::sampleindex_t nSmp);
    bool PrepareUndo(const modplug::tracker::sampleindex_t nSmp, sampleUndoTypes nChangeType, UINT nChangeStart = 0, UINT nChangeEnd = 0);
    bool Undo(const modplug::tracker::sampleindex_t nSmp);
    bool CanUndo(const modplug::tracker::sampleindex_t nSmp);
    void RemoveLastUndoStep(const modplug::tracker::sampleindex_t nSmp);

    void SetParent(CModDoc *pModDoc) {m_pModDoc = pModDoc;}

    CSampleUndo()
    {
            UndoBuffer.clear();
            m_pModDoc = nullptr;
    };
    ~CSampleUndo()
    {
            ClearUndo();
    };

};
