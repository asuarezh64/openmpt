#ifndef _CONTROL_INSTRUMENTS_H_
#define _CONTROL_INSTRUMENTS_H_


class CNoteMapWnd;
class CCtrlInstruments;


//===============================
class CNoteMapWnd: public CStatic
//===============================
{
protected:
    CModDoc *m_pModDoc;
    CCtrlInstruments *m_pParent;
    UINT m_nInstrument, m_nNote, m_nOldNote, m_nOldIns;
    bool m_bIns;
    int m_nPlayingNote;
    HFONT m_hFont;
    int m_cxFont, m_cyFont;
    COLORREF colorText, colorTextSel;

private:
    void MapTranspose(int nAmount);

public:
    CNoteMapWnd()
    {
    	m_nPlayingNote = -1;
    	m_nNote = NOTE_MIDDLEC - 1;
    	m_pModDoc = nullptr;
    	m_nInstrument = 0;
    	m_bIns = false;
    	m_cxFont = m_cyFont = 0;
    	m_hFont = NULL;
    	m_nOldNote = m_nOldIns = 0;
    	m_pParent = NULL;
    }
    BOOL SetCurrentInstrument(CModDoc *pModDoc, UINT nIns);
    BOOL SetCurrentNote(UINT nNote);
    VOID Init(CCtrlInstruments *pParent) { m_pParent = pParent; }
    void EnterNote(UINT note); //rewbs.customKeys - handle notes separately from other input.
    bool HandleChar(WPARAM c); //rewbs.customKeys
    bool HandleNav(WPARAM k);  //rewbs.customKeys
    void PlayNote(int note); //rewbs.customKeys
    void StopNote(int note); //rewbs.customKeys

public:
    //{{AFX_VIRTUAL(CNoteMapWnd)
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CNoteMapWnd)
    afx_msg void OnLButtonDown(UINT, CPoint);
    afx_msg void OnLButtonUp(UINT, CPoint);
    afx_msg void OnRButtonDown(UINT, CPoint);
    afx_msg void OnLButtonDblClk(UINT, CPoint);
    afx_msg void OnSetFocus(CWnd *pOldWnd);
    afx_msg void OnKillFocus(CWnd *pNewWnd);
    afx_msg BOOL OnEraseBkGnd(CDC *) { return TRUE; }
    afx_msg void OnPaint();
    afx_msg void OnMapCopySample();
    afx_msg void OnMapCopyNote();
    afx_msg void OnMapTransposeUp();
    afx_msg void OnMapTransposeDown();
    afx_msg void OnMapReset();
    afx_msg void OnEditSample(UINT nID);
    afx_msg void OnEditSampleMap();
    afx_msg void OnInstrumentDuplicate();
    afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys 
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


//===========================================
class CCtrlInstruments: public CModControlDlg
//===========================================
{
protected:
    CModControlBar m_ToolBar;
    CSpinButtonCtrl m_SpinInstrument, m_SpinFadeOut, m_SpinGlobalVol, m_SpinPanning;
    CSpinButtonCtrl m_SpinMidiPR, m_SpinPPS, m_SpinMidiBK;
    CComboBox m_ComboNNA, m_ComboDCT, m_ComboDCA, m_ComboPPC, m_CbnMidiCh, m_CbnMixPlug, m_CbnResampling, m_CbnFilterMode, m_CbnPluginVelocityHandling, m_CbnPluginVolumeHandling;
    CEdit m_EditName, m_EditFileName, m_EditGlobalVol, m_EditPanning, m_EditPPS, m_EditFadeOut;
    CButton m_CheckPanning, m_CheckCutOff, m_CheckResonance;
    CSliderCtrl m_SliderVolSwing, m_SliderPanSwing, m_SliderCutSwing, m_SliderResSwing, 
    	        m_SliderCutOff, m_SliderResonance;
    CNoteMapWnd m_NoteMap;
    INSTRUMENTINDEX m_nInstrument;

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup"
    CSpinButtonCtrl m_spinRampUp;
    CSpinButtonCtrl m_spinRampDown;
// -! NEW_FEATURE#0027

    //Tuning
    CComboBox m_ComboTuning;
    void UpdateTuningComboBox();
    void BuildTuningComboBox();

    void UpdatePluginList();

    //Pitch/Tempo lock
    CEdit m_EditPitchTempoLock;
    CButton m_CheckPitchTempoLock;

    
public:
    CCtrlInstruments();
    virtual ~CCtrlInstruments();

public:
    void SetInstrumentModified(const bool modified = true);
    BOOL SetCurrentInstrument(UINT nIns, BOOL bUpdNum=TRUE);
    BOOL OpenInstrument(LPCSTR lpszFileName);
    BOOL OpenInstrument(CSoundFile *pSndFile, UINT nInstr);
    BOOL EditSample(UINT nSample);
    VOID UpdateFilterText();
    LONG* GetSplitPosRef() {return &CMainFrame::glInstrumentWindowHeight;} 	//rewbs.varWindowSize

private:
    void CCtrlInstruments::UpdateRampingInPlace(int, int, USHORT *);

public:
    //{{AFX_VIRTUAL(CCtrlInstruments)
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    virtual CRuntimeClass *GetAssociatedViewClass();
    virtual void RecalcLayout();
    virtual void OnActivatePage(LPARAM);
    virtual void OnDeactivatePage();
    virtual void UpdateView(uint32_t dwHintMask=0, CObject *pObj=NULL);
    virtual LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam);
    virtual BOOL GetToolTipText(UINT uId, LPSTR pszText);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL
protected:
    //{{AFX_MSG(CCtrlInstruments)
    afx_msg void OnVScroll(UINT nCode, UINT nPos, CScrollBar *pSB);
    afx_msg void OnHScroll(UINT nCode, UINT nPos, CScrollBar *pSB);
    afx_msg void OnInstrumentChanged();
    afx_msg void OnPrevInstrument();
    afx_msg void OnNextInstrument();
    afx_msg void OnInstrumentNew();
    afx_msg void OnInstrumentDuplicate();
    afx_msg void OnInstrumentOpen();
    afx_msg void OnInstrumentSave();
    afx_msg void OnInstrumentPlay();
    afx_msg void OnNameChanged();
    afx_msg void OnFileNameChanged();
    afx_msg void OnFadeOutVolChanged();
    afx_msg void OnGlobalVolChanged();
    afx_msg void OnSetPanningChanged();
    afx_msg void OnPanningChanged();
    afx_msg void OnNNAChanged();
    afx_msg void OnDCTChanged();
    afx_msg void OnDCAChanged();
    afx_msg void OnMPRChanged();
    afx_msg void OnMBKChanged();	//rewbs.MidiBank
    afx_msg void OnMCHChanged();
    afx_msg void OnResamplingChanged();
    afx_msg void OnMixPlugChanged();  //rewbs.instroVSTi
    afx_msg void OnPPSChanged();
    afx_msg void OnPPCChanged();
    afx_msg void OnFilterModeChanged();
    afx_msg void OnPluginVelocityHandlingChanged();
    afx_msg void OnPluginVolumeHandlingChanged();


// -> CODE#0027
// -> DESC="per-instrument volume ramping setup"
    afx_msg void OnRampChanged();
// -! NEW_FEATURE#0027

    afx_msg void OnEnableCutOff();
    afx_msg void OnEnableResonance();
    afx_msg void OnEditSampleMap();
    afx_msg void TogglePluginEditor();  //rewbs.instroVSTi
    afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys
    afx_msg void OnCbnSelchangeCombotuning();
    afx_msg void OnEnChangeEditPitchtempolock();
    afx_msg void OnBnClickedCheckPitchtempolock();
    afx_msg void OnEnKillfocusEditPitchtempolock();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


#endif
