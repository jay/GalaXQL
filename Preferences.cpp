/////////////////////////////////////////////////////////////////////////////
// Name:        Preferences.cpp
// Author:      Jay Satiro
// License:     BSD
//
// Copyright (c) 2014, Jay Satiro <raysatiro@yahoo.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms,
// with or without modification, are permitted provided
// that the following conditions are met:
//
// - Redistributions of source code must retain the above
//   copyright notice, this list of conditions and the
//   following disclaimer.
// - Redistributions in binary form must reproduce the above
//   copyright notice, this list of conditions and the
//   following disclaimer in the documentation and/or other
//   materials provided with the distribution.
// - The authors' names may not be used to endorse or promote
//   products derived from this software without specific
//   prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if defined(_DEBUG) && defined(__WXMSW__)
    #ifndef _CRTDBG_MAP_ALLOC
        #define _CRTDBG_MAP_ALLOC
    #endif
    #include "wx/msw/msvcrt.h"
    #ifndef _INC_CRTDBG
        #error Debug CRT functions have not been included!
    #endif
    #include <iostream>
#endif

#include <wx/font.h>
#include <wx/tokenzr.h>

#include "Preferences.h"
#include "galaxql.h"
#include "sqlite3.h"


void CreatePrefsTableIfNotExists()
{
    GalaxqlApp *app = (GalaxqlApp *)wxTheApp;

    if(!app)
    {
        return;
    }

    sqlite3_exec(
                app->db,
                "CREATE TABLE IF NOT EXISTS prefs ( "
                "name TEXT PRIMARY KEY NOT NULL, "
                "value TEXT "
                ");",
                NULL,
                NULL,
                NULL);
}


wxString GetPreference(const wxString &name, const wxString &default_value)
{
    GalaxqlApp *app = (GalaxqlApp *)wxTheApp;

    if(!app || !name)
    {
        return default_value;
    }

    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(app->db,
        "SELECT value FROM prefs WHERE name = ?", -1, &stmt, NULL);

    if(rc != SQLITE_OK)
    {
        return default_value;
    }

    rc = sqlite3_bind_text(stmt, 1, name.utf8_str(), -1, SQLITE_TRANSIENT);

    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return default_value;
    }

    rc = sqlite3_step(stmt);

    if(rc != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return default_value;
    }

    const unsigned char *text = sqlite3_column_text(stmt, 0);
    wxString value = text ? wxString::FromUTF8((const char *)text) : default_value;
    sqlite3_finalize(stmt);
    return value;
}

int GetPreference(const wxString &name, const int default_value)
{
    return wxAtoi(GetPreference(name, wxString::Format(wxT("%i"), default_value)));
}


void SetPreference(const wxString &name, const wxString &value)
{
    GalaxqlApp *app = (GalaxqlApp *)wxTheApp;

    if(!app || !name)
    {
        return;
    }

    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(app->db,
        "REPLACE INTO prefs (name, value) VALUES (?, ?)", -1, &stmt, NULL);

    if(rc != SQLITE_OK)
    {
        return;
    }

    rc = sqlite3_bind_text(stmt, 1, name.utf8_str(), -1, SQLITE_TRANSIENT);

    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return;
    }

    rc = sqlite3_bind_text(stmt, 2, value.utf8_str(), -1, SQLITE_TRANSIENT);

    if(rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return;
    }

    rc = sqlite3_step(stmt);

    if(rc != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);
}

void SetPreference(const wxString &name, const int value)
{
    SetPreference(name, wxString::Format(wxT("%i"), value));
}


/* Note if you are adding preferences to be restored:
Follow the pattern below and after applying each preference get the top window
again to make sure it's still good and return if it's not. This is in case the
preference you're applying changed or destroyed the window.
*/
void RestorePreferences()
{
    GalaxqlApp *app = (GalaxqlApp *)wxTheApp;

    if(!app)
    {
        return;
    }

    Galaxql *g = (Galaxql *)app->GetTopWindow();

    if(!g)
    {
        return;
    }

    /** RenderQuality {text value}

    - "Glow"
    Menu > Appearance > Galaxy: Glow (More CPU)

    - "Normal" (default)
    Menu > Appearance > Galaxy: Normal

    - "Low"
    Menu > Appearance > Galaxy: Low-quality (Less CPU)
    */
    wxString quality = GetPreference("RenderQuality", "Normal");

    if(quality == "Glow" && !g->mGfxPanel->mRenderWithGlow)
    {
        wxCommandEvent e;
        g->OnMenuglowClick(e);
    }
    else if(quality == "Low" && !g->mGfxPanel->mRenderInLowQuality)
    {
        wxCommandEvent e;
        g->OnMenulowqualityClick(e);
    }
    else
    {
        wxCommandEvent e;
        g->OnNormalrenderClick(e);
    }

    g = (Galaxql *)app->GetTopWindow();

    if(!g)
    {
        return;
    }

    /** RenderGrid {int value}

    Menu > Appearance > Galaxy: Draw Grid

    - 0 (Default)
    Disabled

    - 1
    Enabled
    */
    int grid = GetPreference("RenderGrid", 0);

    if(grid != g->mGfxPanel->mRenderGrid)
    {
        wxCommandEvent e;
        g->OnRendergridClick(e);
    }

    g = (Galaxql *)app->GetTopWindow();

    if(!g)
    {
        return;
    }

    /** ShowProfessor {int value}

    Menu > Appearance > Show &Professor's picture

    - 0
    Hide

    - 1 (Default)
    Show
    */
    bool professor = !!GetPreference("ShowProfessor", 1);

    if(professor != g->GetMenuBar()->IsChecked(MENU_TOGGLE_GURU_PIC))
    {
        g->GetMenuBar()->Check(MENU_TOGGLE_GURU_PIC, professor);
        wxCommandEvent e;
        g->OnShowProfessorClick(e);
    }

    g = (Galaxql *)app->GetTopWindow();

    if(!g)
    {
        return;
    }

    /** ChapterSelect {int value}

    Guru Tab > Combobox

    - [0, 20] (default 0)
    Refer to mChapterSelect creation in Galaxql::Create().
    */
    int chapter = GetPreference("ChapterSelect", 0);

    g->ResetGuruPanelToChapter(chapter);

    g = (Galaxql *)app->GetTopWindow();

    if(!g)
    {
        return;
    }

    /** LessonFont {text value}

    LessonFont is two font attribute tokens separated by a colon.
    <Normal Font Face Name>:<Normal Font Point Size>

    For example:
    Arial:12

    If either token is empty both the face and point size are based on the
    platform's default font.
    */
    wxStringTokenizer tokenizer(GetPreference("LessonFont", ""), ":");

    wxString face = tokenizer.GetNextToken();
    int size = wxAtoi(tokenizer.GetNextToken());

    if(face.IsEmpty() || (size <= 1))
    {
        face = wxNORMAL_FONT->GetBaseFont().GetFaceName();
        size = wxNORMAL_FONT->GetBaseFont().GetPointSize();
        /* The standard HTML font size we use in the lessons is typically -1,
        which corresponds in points to 83% of the normal point size according to
        wxBuildFontSizes(). Also wxGetDefaultHTMLFontSize() makes the normal
        point size 10 if it's less than that. Which makes the minimum allowable
        point size 8: int(10 * 0.83).
        Both functions can be found in wxWidgets src/html/winpars.cpp.
        */
        size = int((size < 10 ? 10 : size) * 0.83);
    }

    g->SetLessonFonts(size, face);

    /* Sometimes face names contain style information, eg 'Arial Black'. The
    font picker usually cannot discern which font it should show in the picker
    when the face name contains style information. Therefore those names are
    unsupported. In such a scenario the font picker may not appear set to any
    font even though the face name is valid and is the lesson font.
    */
    g->mLessonFontData->SetInitialFont(wxFontInfo(size).FaceName(face));

    g = (Galaxql *)app->GetTopWindow();

    if(!g)
    {
        return;
    }
}


void ResetPreferences()
{
    GalaxqlApp *app = (GalaxqlApp *)wxTheApp;

    if(!app)
    {
        return;
    }

    sqlite3_exec(
                app->db,
                "DROP TABLE IF EXISTS prefs;",
                NULL,
                NULL,
                NULL);

    CreatePrefsTableIfNotExists();
    RestorePreferences();
}
