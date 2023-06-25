/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2017 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <atlcoll.h>
#include <array>
#include "TextFile.h"
#include "SubtitleHelpers.h"
#include "../../include/mpc-hc_config.h"
#if USE_LIBASS
#include "SSASub.h"
#include "SubRendererSettings.h"
#endif
#include "OpenTypeLangTags.h"

enum tmode { TIME, FRAME }; // the meaning of STSEntry::start/end

class STSStyle
{
public:
    enum RelativeTo {
        WINDOW,
        VIDEO,
        AUTO // ~video for SSA/ASS, ~window for the rest
    };

    CRect      marginRect;             // measured from the sides
    int        scrAlignment;           // 1 - 9: as on the numpad, 0: default
    int        borderStyle;            // 0: outline, 1: opaque box
    double     outlineWidthX, outlineWidthY;
    double     shadowDepthX, shadowDepthY;
    std::array<COLORREF, 4> colors;    // usually: {primary, secondary, outline/background, shadow}
    std::array<BYTE, 4> alpha;
    int        charSet;
    CString    fontName;
    double     fontSize;               // height
    double     fontScaleX, fontScaleY; // percent
    double     fontSpacing;            // +/- pixels
    LONG       fontWeight;
    int        fItalic;
    int        fUnderline;
    int        fStrikeOut;
    int        fBlur;
    double     fGaussianBlur;
    double     fontAngleZ, fontAngleX, fontAngleY;
    double     fontShiftX, fontShiftY;

    RelativeTo relativeTo;

#if USE_LIBASS
    // libass stuff
    DWORD      SrtResX = 1920;
    DWORD      SrtResY = 1080;
    bool       Kerning = false;
    bool       ScaledBorderAndShadow = false;
    CString    customTags;
#endif

    STSStyle();

    void SetDefault();

    bool operator == (const STSStyle& s) const;
    bool operator != (const STSStyle& s) const {
        return !(*this == s);
    };
    bool IsFontStyleEqual(const STSStyle& s) const;

    STSStyle& operator = (LOGFONT& lf);

    friend LOGFONTA& operator <<= (LOGFONTA& lfa, const STSStyle& s);
    friend LOGFONTW& operator <<= (LOGFONTW& lfw, const STSStyle& s);

    friend CString& operator <<= (CString& style, const STSStyle& s);
    friend STSStyle& operator <<= (STSStyle& s, const CString& style);
};

class CSTSStyleMap : public CAtlMap<CString, STSStyle*, CStringElementTraits<CString>>
{
public:
    CSTSStyleMap() {}
    virtual ~CSTSStyleMap() { Free(); }
    void Free();
};

struct STSEntry {
    CStringW str;
    bool fUnicode;
    CString style, actor, effect;
    CRect marginRect;
    int layer;
    REFERENCE_TIME start, end;
    int readorder;
};

class STSSegment
{
public:
    REFERENCE_TIME start, end;
    CAtlArray<int> subs;

    STSSegment()
        : start(0)
        , end(0) {}
    STSSegment(REFERENCE_TIME s, REFERENCE_TIME e) {
        start = s;
        end = e;
    }
    STSSegment(const STSSegment& stss) {
        *this = stss;
    }
    STSSegment& operator = (const STSSegment& stss) {
        if (this != &stss) {
            start = stss.start;
            end = stss.end;
            subs.Copy(stss.subs);
        }
        return *this;
    }
};

class CSimpleTextSubtitle : public CAtlArray<STSEntry>
{
    friend class CSubtitleEditorDlg;
    friend class SubtitlesProvider;

protected:
    CAtlArray<STSSegment> m_segments;
    virtual void OnChanged() {}

public:
    CString m_name;
    LCID m_lcid;
    CString m_langname;
    CStringA openTypeLangHint;
    Subtitle::SubType m_subtitleType;
    tmode m_mode;
    CTextFile::enc m_encoding;
    CString m_path;

    CString m_provider;

    Subtitle::HearingImpairedType m_eHearingImpaired;

    CSize m_storageRes;
    CSize m_playRes;
    CSize m_layoutRes;

    int m_defaultWrapStyle;
    int m_collisions;
    int m_scaledBAS; // -1 = unknown, 0 = no, 1 = yes
    CString m_sYCbCrMatrix;

    bool m_bUsingPlayerDefaultStyle;

    CSTSStyleMap m_styles;

    enum EPARCompensationType {
        EPCTDisabled,
        EPCTDownscale,
        EPCTUpscale,
        EPCTAccurateSize,
        EPCTAccurateSize_ISR
    };

    EPARCompensationType m_ePARCompensationType;
    double m_dPARCompensation;

public:
    CSimpleTextSubtitle();
    virtual ~CSimpleTextSubtitle();

    virtual void Copy(CSimpleTextSubtitle& sts);
    virtual void Empty();

    void Sort(bool fRestoreReadorder = false);
    void CreateSegments();

    void Append(CSimpleTextSubtitle& sts, REFERENCE_TIME timeoff = -1);

    bool Open(CString fn, int CharSet, CString name = _T(""), CString videoName = _T(""));
    bool Open(CTextFile* f, int CharSet, CString name);
    bool Open(BYTE* data, int length, int CharSet, CString provider, CString lang, CString ext = _T(""));
    bool Open(CString data, CTextFile::enc SaveCharSet, int ReadCharSet, CString provider, CString lang, CString ext = _T(""));
    bool Open(BYTE* data, int len, int CharSet, CString name);
    bool Open(CString provider, BYTE* data, int len, int CharSet, CString name, Subtitle::HearingImpairedType eHearingImpaired, LCID lcid);
    bool SaveAs(CString fn, Subtitle::SubType type, double fps = -1, LONGLONG delay = 0, CTextFile::enc e = CTextFile::DEFAULT_ENCODING, bool bCreateExternalStyleFile = true);

    void Add(CStringW str, bool fUnicode, REFERENCE_TIME start, REFERENCE_TIME end, CString style = _T("Default"), CString actor = _T(""), CString effect = _T(""), const CRect& marginRect = CRect(0, 0, 0, 0), int layer = 0, int readorder = -1);
    STSStyle* CreateDefaultStyle(int CharSet);
    void ChangeUnknownStylesToDefault();
    void AddStyle(CString name, STSStyle* style); // style will be stored and freed in Empty() later
    bool CopyStyles(const CSTSStyleMap& styles, bool fAppend = false);

    bool SetDefaultStyle(const STSStyle& s);
    bool GetDefaultStyle(STSStyle& s) const;

    void ConvertToTimeBased(double fps);
    void ConvertToFrameBased(double fps);

    REFERENCE_TIME TranslateStart(int i, double fps);
    REFERENCE_TIME TranslateEnd(int i, double fps);
    int SearchSub(REFERENCE_TIME t, double fps);

    REFERENCE_TIME TranslateSegmentStart(int i, double fps);
    REFERENCE_TIME TranslateSegmentEnd(int i, double fps);
    const STSSegment* SearchSubs(REFERENCE_TIME t, double fps, /*[out]*/ int* iSegment = nullptr, int* nSegments = nullptr);
    const STSSegment* GetSegment(int iSegment) {
        return iSegment >= 0 && iSegment < (int)m_segments.GetCount() ? &m_segments[iSegment] : nullptr;
    }

    STSStyle* GetStyle(int i);
    bool GetStyle(int i, STSStyle& stss);
    bool GetStyle(CString styleName, STSStyle& stss);
    int GetCharSet(int i);
    bool IsEntryUnicode(int i);
    void ConvertUnicode(int i, bool fUnicode);

    CStringA GetStrA(int i, bool fSSA = false);
    CStringW GetStrW(int i, bool fSSA = false);
    CStringW GetStrWA(int i, bool fSSA = false);

    void SetStr(int i, CStringA str, bool fUnicode /* ignored */);
    void SetStr(int i, CStringW str, bool fUnicode);
    void SetOpenTypeLangHint(CStringA openTypeLangHint) { this->openTypeLangHint = openTypeLangHint; }

public:
    STSStyle m_styleOverride; // the app can decide to use this style instead of a built-in one

#if USE_LIBASS
public:
    bool m_renderUsingLibass;
    OpenTypeLang::HintStr m_openTypeLangHint;
    SubRendererSettings subRendererSettings;
    void SetSubRenderSettings(SubRendererSettings settings);



    bool m_assloaded;
    bool m_assfontloaded;

    IFilterGraph* m_pGraph;
    std::unique_ptr<ASS_Library, ASS_LibraryDeleter> m_ass;
    std::unique_ptr<ASS_Renderer, ASS_RendererDeleter> m_renderer;
    std::unique_ptr<ASS_Track, ASS_TrackDeleter> m_track;

    void ResetASS();
    bool LoadASSFile(Subtitle::SubType subType);
    bool LoadASSTrack(char* data, int size, Subtitle::SubType subType);
    void UnloadASS();
    void LoadASSSample(char* data, int dataSize, REFERENCE_TIME tStart, REFERENCE_TIME tStop);
    void LoadASSFont(IPin* pPin, ASS_Library* ass, ASS_Renderer* renderer);
    void SetFilterGraph(IFilterGraph* g) { m_pGraph = g; };
    void SetPin(IPin* i) { m_pPin = i; };

protected:
    IPin* m_pPin;
#endif
};

extern const BYTE CharSetList[];
extern const TCHAR* CharSetNames[];
extern const int CharSetLen;

class CHtmlColorMap : public CAtlMap<CString, DWORD, CStringElementTraits<CString>>
{
public:
    CHtmlColorMap();
};

extern const CHtmlColorMap g_colors;
