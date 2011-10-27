#ifndef _MODPLUGDLGS_
#define _MODPLUGDLGS_

#define NUMMIXRATE    16

class CSoundFile;
class CMainFrame;

//===========================================
class COptionsSoundcard: public CPropertyPage
//===========================================
{
protected:
    CComboBoxEx m_CbnDevice;
    CComboBox m_CbnBufferLength, m_CbnMixingFreq, m_CbnPolyphony, m_CbnQuality;
    CSliderCtrl m_SliderStereoSep, m_SliderPreAmp;
    uint32_t m_dwRate, m_dwSoundSetup, m_nBitsPerSample, m_nChannels;
    uint32_t m_nBufferLength;
    uint32_t m_nSoundDevice;
    bool m_PreAmpNoteShowed;

public:
    COptionsSoundcard(uint32_t rate, uint32_t q, uint32_t bits, uint32_t chns, uint32_t bufsize, uint32_t sd):CPropertyPage(IDD_OPTIONS_SOUNDCARD)
    	{ m_dwRate = rate; m_dwSoundSetup = q; m_nBitsPerSample = bits; m_nChannels = chns;
    	  m_nBufferLength = bufsize; m_nSoundDevice = sd; m_PreAmpNoteShowed = false;}

private:
    void UpdateSampleRates(int dev);
    void SetPreAmpSliderPosition();

protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual BOOL OnSetActive();
    virtual void DoDataExchange(CDataExchange* pDX);
    void UpdateStereoSep();
    afx_msg void OnDeviceChanged();
    afx_msg void OnSettingsChanged() { SetModified(TRUE); }
    afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
    afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
    DECLARE_MESSAGE_MAP()
};


//========================================
class COptionsPlayer: public CPropertyPage
//========================================
{
protected:
    CComboBox m_CbnResampling, m_CbnReverbPreset, m_CbnWFIRType; //rewbs.resamplerConf: added m_CbnWFIRType
    CSliderCtrl m_SbXBassDepth, m_SbXBassRange;
    CSliderCtrl m_SbSurroundDepth, m_SbSurroundDelay;
    CSliderCtrl m_SbReverbDepth;
    CEdit m_CEditWFIRCutoff;
    CEdit m_CEditRampIn;
    CEdit m_CEditRampOut;

public:
    COptionsPlayer():CPropertyPage(IDD_OPTIONS_PLAYER) {}

protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual BOOL OnSetActive();
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnHScroll(UINT, UINT, CScrollBar *);
    afx_msg void OnSettingsChanged() { SetModified(TRUE); }
    //rewbs.resamplerConf
    afx_msg void OnWFIRTypeChanged();
    afx_msg void OnResamplerChanged();
    afx_msg void OnDefaultResampling();
    //end rewbs.resamplerConf

    DECLARE_MESSAGE_MAP()
};



//=================================
class CEQSlider: public CSliderCtrl
//=================================
{
public:
    CWnd *m_pParent;
    UINT m_nSliderNo;
    short int m_x, m_y;
public:
    CEQSlider() {}
    VOID Init(UINT nID, UINT n, CWnd *parent);
    BOOL PreTranslateMessage(MSG *pMsg);
};


//=====================================
class CEQSetupDlg: public CPropertyPage
//=====================================
{
protected:
    CEQSlider m_Sliders[MAX_EQ_BANDS];
    PEQPRESET m_pEqPreset;
    UINT m_nSliderMenu;

public:
    CEQSetupDlg(PEQPRESET pEq):CPropertyPage(IDD_SETUP_EQ) { m_pEqPreset = pEq; }
    void UpdateDialog();
    void UpdateEQ(BOOL bReset);

public:
    static EQPRESET gUserPresets[4];
    static VOID LoadEQ(HKEY key, LPCSTR pszName, PEQPRESET pEqSettings);
    static VOID SaveEQ(HKEY key, LPCSTR pszName, PEQPRESET pEqSettings);

protected:
    virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive();
    afx_msg void OnSettingsChanged() { SetModified(TRUE); }
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEqFlat();
    afx_msg void OnEqJazz();
    afx_msg void OnEqPop();
    afx_msg void OnEqRock();
    afx_msg void OnEqConcert();
    afx_msg void OnEqClear();
    afx_msg void OnEqUser1();
    afx_msg void OnEqUser2();
    afx_msg void OnEqUser3();
    afx_msg void OnEqUser4();
    afx_msg void OnSavePreset();
    afx_msg void OnSliderMenu(UINT);
    afx_msg void OnSliderFreq(UINT);
    DECLARE_MESSAGE_MAP()
};


//=======================================
class CMidiSetupDlg: public CPropertyPage
//=======================================
{
public:
    uint32_t m_dwMidiSetup;
    LONG m_nMidiDevice;

protected:
    CSpinButtonCtrl m_SpinSpd, m_SpinPat;

public:
    CMidiSetupDlg(uint32_t d, LONG n):CPropertyPage(IDD_OPTIONS_MIDI)
    	{ m_dwMidiSetup = d; m_nMidiDevice = n; }

protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual BOOL OnSetActive();
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnSettingsChanged() { SetModified(TRUE); }
    DECLARE_MESSAGE_MAP()
};

#endif