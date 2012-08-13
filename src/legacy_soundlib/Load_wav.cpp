/*
 * This source code is public domain.
 *
 * Copied to OpenMPT from libmodplug.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *                    OpenMPT dev(s)        (miscellaneous modifications)
*/

#include "stdafx.h"
#include "Loaders.h"
#include "Wav.h"

#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE    0xFFFE
#endif

/////////////////////////////////////////////////////////////
// WAV file support

bool module_renderer::ReadWav(const uint8_t *lpStream, const uint32_t dwMemLength)
//---------------------------------------------------------------------
{
    uint32_t dwMemPos = 0;
    WAVEFILEHEADER *phdr = (WAVEFILEHEADER *)lpStream;
    WAVEFORMATHEADER *pfmt = (WAVEFORMATHEADER *)(lpStream + sizeof(WAVEFILEHEADER));
    if ((!lpStream) || (dwMemLength < (uint32_t)sizeof(WAVEFILEHEADER))) return false;
    if ((phdr->id_RIFF != IFFID_RIFF) || (phdr->id_WAVE != IFFID_WAVE)
     || (pfmt->id_fmt != IFFID_fmt)) return false;
    dwMemPos = sizeof(WAVEFILEHEADER) + 8 + pfmt->hdrlen;
    if ((dwMemPos + 8 >= dwMemLength)
     || ((pfmt->format != WAVE_FORMAT_PCM) && (pfmt->format != WAVE_FORMAT_EXTENSIBLE))
     || (pfmt->channels > 4)
     || (!pfmt->channels)
     || (!pfmt->freqHz)
     || (pfmt->bitspersample & 7)
     || (pfmt->bitspersample < 8)
     || (pfmt->bitspersample > 32))  return false;
    WAVEDATAHEADER *pdata;
    for (;;)
    {
            pdata = (WAVEDATAHEADER *)(lpStream + dwMemPos);
            if (pdata->id_data == IFFID_data) break;
            dwMemPos += pdata->length + 8;
            if (dwMemPos + 8 >= dwMemLength) return false;
    }
    m_nType = MOD_TYPE_WAV;
    m_nSamples = 0;
    m_nInstruments = 0;
    m_nChannels = 4;
    m_nDefaultSpeed = 8;
    m_nDefaultTempo = 125;
    m_dwSongFlags |= SONG_LINEARSLIDES; // For no resampling
    Order.resize(MAX_ORDERS, Order.GetInvalidPatIndex());
    Order[0] = 0;
    bool fail = Patterns.Insert(0, 64);
    fail = Patterns.Insert(1, 64);
    if(fail) return true;
    UINT samplesize = (pfmt->channels * pfmt->bitspersample) >> 3;
    UINT len = pdata->length, bytelen;
    if (dwMemPos + len > dwMemLength - 8) len = dwMemLength - dwMemPos - 8;
    len /= samplesize;
    bytelen = len;
    if (pfmt->bitspersample >= 16) bytelen *= 2;
    if (len > MAX_SAMPLE_LENGTH) len = MAX_SAMPLE_LENGTH;
    if (!len) return true;
    // Setting up module length
    uint32_t dwTime = ((len * 50) / pfmt->freqHz) + 1;
    uint32_t framesperrow = (dwTime + 63) / 63;
    if (framesperrow < 4) framesperrow = 4;
    UINT norders = 1;
    while (framesperrow >= 0x20)
    {
            Order[norders++] = 1;
            Order[norders] = 0xFF;
            framesperrow = (dwTime + (64 * norders - 1)) / (64 * norders);
            if (norders >= MAX_ORDERS-1) break;
    }
    m_nDefaultSpeed = framesperrow;
    for (UINT iChn=0; iChn<4; iChn++)
    {
            ChnSettings[iChn].nPan = (iChn & 1) ? 256 : 0;
            ChnSettings[iChn].nVolume = 64;
            ChnSettings[iChn].dwFlags = 0;
    }
    // Setting up speed command
    modplug::tracker::modevent_t *pcmd = Patterns[0];
    pcmd[0].command = CMD_SPEED;
    pcmd[0].param = (uint8_t)m_nDefaultSpeed;
    pcmd[0].note = 5*12+1;
    pcmd[0].instr = 1;
    pcmd[1].note = pcmd[0].note;
    pcmd[1].instr = pcmd[0].instr;
    m_nSamples = pfmt->channels;
    // Support for Multichannel Wave
    for (UINT nChn=0; nChn<m_nSamples; nChn++)
    {
            modsample_t *pSmp = &Samples[nChn+1];
            pcmd[nChn].note = pcmd[0].note;
            pcmd[nChn].instr = (uint8_t)(nChn+1);
            pSmp->length = len;
            pSmp->c5_samplerate = pfmt->freqHz;
            pSmp->default_volume = 256;
            pSmp->default_pan = 128;
            pSmp->global_volume = 64;
            pSmp->flags = (uint16_t)((pfmt->bitspersample >= 16) ? CHN_16BIT : 0);
            pSmp->flags |= CHN_PANNING;
            if (m_nSamples > 1)
            {
                    switch(nChn)
                    {
                    case 0:        pSmp->default_pan = 0; break;
                    case 1:        pSmp->default_pan = 256; break;
                    case 2: pSmp->default_pan = (uint16_t)((m_nSamples == 3) ? 128 : 64); pcmd[nChn].command = CMD_S3MCMDEX; pcmd[nChn].param = 0x91; break;
                    case 3: pSmp->default_pan = 192; pcmd[nChn].command = CMD_S3MCMDEX; pcmd[nChn].param = 0x91; break;
                    default: pSmp->default_pan = 128; break;
                    }
            }
            if ((pSmp->sample_data = AllocateSample(bytelen+8)) == NULL) return true;
            if (pfmt->bitspersample >= 16)
            {
                    int slsize = pfmt->bitspersample >> 3;
                    signed short *p = (signed short *)pSmp->sample_data;
                    signed char *psrc = (signed char *)(lpStream+dwMemPos+8+nChn*slsize+slsize-2);
                    for (UINT i=0; i<len; i++)
                    {
                            p[i] = *((signed short *)psrc);
                            psrc += samplesize;
                    }
                    p[len+1] = p[len] = p[len-1];
            } else
            {
                    signed char *p = (signed char *)pSmp->sample_data;
                    signed char *psrc = (signed char *)(lpStream+dwMemPos+8+nChn);
                    for (UINT i=0; i<len; i++)
                    {
                            p[i] = (signed char)((*psrc) + 0x80);
                            psrc += samplesize;
                    }
                    p[len+1] = p[len] = p[len-1];
            }
    }
    return true;
}


////////////////////////////////////////////////////////////////////////
// IMA ADPCM Support

#pragma pack(1)

typedef struct IMAADPCMBLOCK
{
    uint16_t sample;
    uint8_t index;
    uint8_t Reserved;
} DVI_ADPCMBLOCKHEADER;

#pragma pack()

static const int gIMAUnpackTable[90] =
{
  7,     8,     9,    10,    11,    12,    13,    14,
  16,    17,    19,    21,    23,    25,    28,    31,
  34,    37,    41,    45,    50,    55,    60,    66,
  73,    80,    88,    97,   107,   118,   130,   143,
  157,   173,   190,   209,   230,   253,   279,   307,
  337,   371,   408,   449,   494,   544,   598,   658,
  724,   796,   876,   963,  1060,  1166,  1282,  1411,
  1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
  15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
  32767, 0
};


BOOL IMAADPCMUnpack16(signed short *pdest, UINT nLen, LPBYTE psrc, uint32_t dwBytes, UINT pkBlkAlign)
//------------------------------------------------------------------------------------------------
{
    static const int gIMAIndexTab[8] =  { -1, -1, -1, -1, 2, 4, 6, 8 };
    UINT nPos;
    int value;

    if ((nLen < 4) || (!pdest) || (!psrc)
     || (pkBlkAlign < 5) || (pkBlkAlign > dwBytes)) return false;
    nPos = 0;
    while ((nPos < nLen) && (dwBytes > 4))
    {
            int nIndex;
            value = *((short int *)psrc);
            nIndex = psrc[2];
            psrc += 4;
            dwBytes -= 4;
            pdest[nPos++] = (short int)value;
            for (UINT i=0; ((i<(pkBlkAlign-4)*2) && (nPos < nLen) && (dwBytes)); i++)
            {
                    uint8_t delta;
                    if (i & 1)
                    {
                            delta = (uint8_t)(((*(psrc++)) >> 4) & 0x0F);
                            dwBytes--;
                    } else
                    {
                            delta = (uint8_t)((*psrc) & 0x0F);
                    }
                    int v = gIMAUnpackTable[nIndex] >> 3;
                    if (delta & 1) v += gIMAUnpackTable[nIndex] >> 2;
                    if (delta & 2) v += gIMAUnpackTable[nIndex] >> 1;
                    if (delta & 4) v += gIMAUnpackTable[nIndex];
                    if (delta & 8) value -= v; else value += v;
                    nIndex += gIMAIndexTab[delta & 7];
                    if (nIndex < 0) nIndex = 0; else
                    if (nIndex > 88) nIndex = 88;
                    if (value > 32767) value = 32767; else
                    if (value < -32768) value = -32768;
                    pdest[nPos++] = (short int)value;
            }
    }
    return true;
}