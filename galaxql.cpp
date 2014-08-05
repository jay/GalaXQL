/////////////////////////////////////////////////////////////////////////////
// Name:        galaxql.cpp
// Author:      Jari Komppa
// Licence:     
//
// Copyright (c) 2005, Jari Komppa
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
    #include "wx/msw/msvcrt.h"
    #if !defined(_INC_CRTDBG) || !defined(_CRTDBG_MAP_ALLOC)
        #error Debug CRT functions have not been included!
    #endif
    #include <iostream>
#endif

#include <wx/clipbrd.h>

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "galaxql.h"
#endif

#define MAX_STARS (32*1024)

#include "wx/glcanvas.h"
#include "wx/splitter.h"
#include "wx/button.h"
#include "wx/textctrl.h"
#include "wx/slider.h"
#include "wx/dcbuffer.h"
#include "wx/panel.h"
#include "wx/image.h"
#include "wx/dialog.h"
#include "wx/checkbox.h"
#if defined(__WXMAC__) || defined(__WXCOCOA__)
#include <OpenGL/glu.h>
#include <sys/syslimits.h>
#else
#include <GL/glu.h>
#endif
#include "sqlite3.h"
#include "RegenerationDialog.h"
#include "wx/fs_zip.h"
#include "wx/filesys.h"
#include "wx/filedlg.h"
#include "wx/ffile.h"
#include "wx/stdpaths.h"
////@begin includes
////@end includes

#include "galaxql.h"
#include "sqlqueryctrl.h"

const int starcolors[]= // Hagen numbers
{
    0xD1D1FF, // -3
    0xDBF4FF, // -2
    0xEAFBFF, // -1
    0xFFFFFF, // 0
    0xFFFFE5, // 1
    0xFFFFDB, // 2
    0xFFFFD1, // 3
    0xFFF1DB, // 4
    0xFBE8CF, // 5
    0xFFE6D6, // 6
    0xFAD8D0, // 7
    0xFFD6D6, // 8
    0xFCCECF, // 9
    0xFFD1D1, // 10
};

const int planetcolors[]= // more or less random numbers
{
    0x7FFF7F,
    0x3F3F3F,
    0xFF7FFF,
    0x7F7F7F,
    0x3F3FFF,
    0xFF3F3F,
    0x3F7FFF,
    0xFF7F3F,
    0x7F3FFF,
    0xFF3F7F,  
};


////@begin XPM images
#include "galaxql-16x16.xpm"
#include "galaxql-32x32.xpm"
#include "galaxql-64x64.xpm"
////@end XPM images

/*!
 * Application instance implementation
 */

////@begin implement app
IMPLEMENT_APP( GalaxqlApp )
////@end implement app

/*!
 * GalaxqlApp type definition
 */

IMPLEMENT_CLASS( GalaxqlApp, wxApp )

/*!
 * GalaxqlApp event table definition
 */

BEGIN_EVENT_TABLE( GalaxqlApp, wxApp )

////@begin GalaxqlApp event table entries
////@end GalaxqlApp event table entries

END_EVENT_TABLE()

/*!
 * Constructor for GalaxqlApp
 */

GalaxqlApp::GalaxqlApp()
{
////@begin GalaxqlApp member initialisation
////@end GalaxqlApp member initialisation    
}

/*!
 * Initialisation for GalaxqlApp
 */

bool GalaxqlApp::OnInit()
{ 
#if defined(__WXMAC__) || defined(__WXCOCOA__)
    // Change working directory to bundle's 'Resources'

    // Get the URL to the application bundle
    CFURLRef appURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());

    // Convert to native file system representation
    UInt8 path[PATH_MAX];
    CFURLGetFileSystemRepresentation(appURL, TRUE, path, PATH_MAX);

    // Change directory
    chdir((const char*)path);
    chdir("Contents/Resources");

    // Clean up
    CFRelease(appURL);
#else
    // Much of GalaXQL assumes that the current working directory contains the
    // .dat and .db file.  This should be the same directory that the main
    // executable is in.  In the event that GalaXQL was launched from a different
    // directory, we must switch to the executable's directory or else GalaXQL
    // will crash when it cannot load a resource.
    wxFileName executableFileName(wxStandardPaths::Get().GetExecutablePath());
    wxSetWorkingDirectory(executableFileName.GetPath(wxPATH_GET_VOLUME));
#endif

    if (!wxFileExists(wxT("galaxql.dat")))
    {
        wxMessageBox(
            wxString::Format(
                wxT("Unable to locate the data file \"%s\".  Try reinstalling GalaXQL."),
                wxT("galaxql.dat")),
            wxT("Error"),
            wxOK | wxICON_ERROR);

        return false;
    }

    bool regen_needed = true;
    FILE * testf;
    testf = fopen("galaxql.db","rb");
    if (testf != NULL && fgetc(testf) != EOF)
    {
        // exists, close up and that's it
        fclose(testf);
        regen_needed = false;
    }
    else
    {
        testf = fopen("galaxql.db","wb");
        if (testf == NULL)
        {
            // on a read-only media - probably a .dmg on a mac
            wxMessageBox(
                wxT("Can't create database. Try copying GalaXQL to a non-readonly media and try again."),
                wxT("Error"),
                wxOK | wxICON_ERROR);

            return false;
        }
        fclose(testf);
    }

    wxFileSystem::AddHandler(new wxZipFSHandler);
    wxInitAllImageHandlers();

    int rc = sqlite3_open("galaxql.db", &db);
    if (rc)
    {
        wxMessageBox(wxT("Can't open galaxql.db!"), wxT("Error"),
                 wxOK | wxICON_ERROR);
        return false;
    }

    Galaxql* mainWindow = new Galaxql( NULL, ID_FRAME );

    if (regen_needed)
        mainWindow->RegenerateWorld(1);

    mainWindow->Center();
    mainWindow->Show(true);

    return true;
}

/*!
 * Cleanup for GalaxqlApp
 */
int GalaxqlApp::OnExit()
{    
    sqlite3_close(db);
////@begin GalaxqlApp cleanup
    return wxApp::OnExit();
////@end GalaxqlApp cleanup
}


/*!
 * Galaxql type definition
 */

IMPLEMENT_CLASS( Galaxql, wxFrame )

/*!
 * Galaxql event table definition
 */

BEGIN_EVENT_TABLE( Galaxql, wxFrame )

////@begin Galaxql event table entries
    EVT_MENU( ID_REGENERATE25K, Galaxql::OnRegenerate25kClick )

    EVT_MENU( ID_REGENERATE10K, Galaxql::OnRegenerate10kClick )

    EVT_MENU( ID_REGENERATE1KSPHERE, Galaxql::OnRegenerate1ksphereClick )

    EVT_MENU( wxID_OPEN, Galaxql::OnLoadqueryClick )

    EVT_MENU( wxID_SAVE, Galaxql::OnSavequeryClick )

    EVT_MENU( wxID_EXIT, Galaxql::OnQuitappClick )

    EVT_MENU( ID_SQLGETINFO, Galaxql::OnSqlgetinfoClick )

    EVT_MENU( ID_SQLCOUNTSTARS, Galaxql::OnSqlcountstarsClick )

    EVT_MENU( ID_PLANETCOUNT, Galaxql::OnPlanetcountClick )

    EVT_MENU( ID_MAXPLANETS, Galaxql::OnMaxplanetsClick )

    EVT_MENU( ID_SQLRESETHILITE, Galaxql::OnSqlresethiliteClick )

    EVT_MENU( ID_HILIGHT_MOON_PLANETS, Galaxql::OnHilightMoonPlanetsClick )

    EVT_MENU( ID_SQLSHRINK, Galaxql::OnSqlshrinkClick )

    EVT_MENU( ID_TURNGALAXY, Galaxql::OnTurngalaxyClick )

    EVT_MENU( ID_MENURUNQUERY, Galaxql::OnMenurunqueryClick )

    EVT_MENU( ID_MENUGLOW, Galaxql::OnMenuglowClick )

    EVT_MENU( ID_NORMALRENDER, Galaxql::OnNormalrenderClick )

    EVT_MENU( ID_MENULOWQUALITY, Galaxql::OnMenulowqualityClick )

    EVT_MENU( ID_RENDERGRID, Galaxql::OnRendergridClick )

    EVT_MENU( wxID_ABOUT, Galaxql::OnAboutClick )

    EVT_BUTTON( wxID_OPEN, Galaxql::OnLoadqueryClick )

    EVT_BUTTON( wxID_SAVE, Galaxql::OnSavequeryClick )

    EVT_BUTTON( ID_RUNQUERY, Galaxql::OnRunqueryClick )

    EVT_NOTEBOOK_PAGE_CHANGED( ID_NOTEBOOK, Galaxql::OnNotebookPageChanged )

    EVT_CHOICE( ID_CHAPTERSELECT, Galaxql::OnChapterselectSelected )

    EVT_BUTTON( ID_GURU_DONE, Galaxql::OnGuruDoneClick )

////@end Galaxql event table entries

END_EVENT_TABLE()

/*!
 * Galaxql constructors
 */

Galaxql::Galaxql( )
{
}

Galaxql::Galaxql( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * Galaxql creator
 */

bool Galaxql::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin Galaxql member initialisation
    mGfxPanel = NULL;
    mQuery = NULL;
    mLoadQuery = NULL;
    mSaveQuery = NULL;
    mRunQuery = NULL;
    mNotebook = NULL;
    mGuruPicture = NULL;
    mChapterSelect = NULL;
    mGuruSpeaks = NULL;
    mGuruDone = NULL;
    mQueryResult = NULL;
    mHtmlPanel = NULL;
////@end Galaxql member initialisation

////@begin Galaxql creation
    wxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();

    if (FindWindow(ID_SPLITTERWINDOW))
        ((wxSplitterWindow*) FindWindow(ID_SPLITTERWINDOW))->SetSashPosition(500);
    if (FindWindow(ID_DISPLAYQUERYSPLITTER))
        ((wxSplitterWindow*) FindWindow(ID_DISPLAYQUERYSPLITTER))->SetSashPosition(400);
////@end Galaxql creation

    wxFileSystem *fs = new wxFileSystem();
    wxFSFile * f = fs->OpenFile(wxT("file:galaxql.dat#zip:sad.png"));

    mGuruSad.LoadFile(*f->GetStream(), wxBITMAP_TYPE_PNG);
    delete f;
    f = fs->OpenFile(wxT("file:galaxql.dat#zip:happy.png"));
    mGuruHappy.LoadFile(*f->GetStream(), wxBITMAP_TYPE_PNG);
    delete f;
    f = fs->OpenFile(wxT("file:galaxql.dat#zip:normal.png"));
    mGuruNormal.LoadFile(*f->GetStream(), wxBITMAP_TYPE_PNG);
    delete f;
    f = fs->OpenFile(wxT("file:galaxql.dat#zip:baddb.png"));
    mGuruBadDb.LoadFile(*f->GetStream(), wxBITMAP_TYPE_PNG);
    delete f;
    f = fs->OpenFile(wxT("file:galaxql.dat#zip:easter.png"));
    mGuruEaster.LoadFile(*f->GetStream(), wxBITMAP_TYPE_PNG);
    delete f;
        
    mGuruPicture->SetBitmap(mGuruNormal);
    delete fs;

    mChapterSelect->Append(wxT("1. Welcome.."));
    mChapterSelect->Append(wxT("2. SELECT .."));
    mChapterSelect->Append(wxT("3. SELECT .. FROM .."));
    mChapterSelect->Append(wxT("4. SELECT .. FROM .. WHERE .. ORDER BY .. DESC"));
    mChapterSelect->Append(wxT("5. MAX(), MIN(), COUNT(), AVG(), SUM(), NULL"));
    mChapterSelect->Append(wxT("6. INSERT INTO .. VALUES .."));
    mChapterSelect->Append(wxT("7. INSERT INTO .. SELECT .."));
    mChapterSelect->Append(wxT("8. Transactions, DELETE FROM .. WHERE .."));
    mChapterSelect->Append(wxT("9. UPDATE .. SET .. WHERE .."));
    mChapterSelect->Append(wxT("10. SELECT FROM table1, table2.., DISTINCT"));
    mChapterSelect->Append(wxT("11. SELECT .. FROM (SELECT .. FROM ..)"));
    mChapterSelect->Append(wxT("12. SELECT FROM .. JOIN .."));
    mChapterSelect->Append(wxT("13. CREATE VIEW, DROP VIEW"));
    mChapterSelect->Append(wxT("14. CREATE TABLE, DROP TABLE"));
    mChapterSelect->Append(wxT("15. Constraints"));
    mChapterSelect->Append(wxT("16. ALTER TABLE"));
    mChapterSelect->Append(wxT("17. SELECT .. GROUP BY .. HAVING .."));
    mChapterSelect->Append(wxT("18. UNION, UNION ALL, INTERSECT, EXCEPT"));
    mChapterSelect->Append(wxT("19. Triggers"));
    mChapterSelect->Append(wxT("20. Indexes"));
    mChapterSelect->Append(wxT("That's all for now"));
    mChapterSelect->Select(0);
    wxCommandEvent e;
    OnChapterselectSelected(e);

    mHtmlPanel->LoadPage(wxT("file:galaxql.dat#zip:index.htm"));

    mUserQuery.mRows = 0;
    mUserQuery.mColumns = 0;
    mUserQuery.mStatus = wxEmptyString;

    return true;
}

/*!
 * Control creation for Galaxql
 */

void Galaxql::CreateControls()
{    
////@begin Galaxql content construction
    // Generated by DialogBlocks, 03/06/2006 15:22:22 (Personal Edition)

    Galaxql* itemFrame1 = this;

    wxMenuBar* menuBar = new wxMenuBar;
    wxMenu* itemMenu3 = new wxMenu;
    wxMenu* itemMenu4 = new wxMenu;
    itemMenu4->Append(ID_REGENERATE25K, _("Regenerate database (&25k)"), _T(""), wxITEM_NORMAL);
    itemMenu4->Append(ID_REGENERATE10K, _("Regenerate database (&10k)"), _T(""), wxITEM_NORMAL);
    itemMenu4->Append(ID_REGENERATE1KSPHERE, _("Regenerate database (1k &sphere)"), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(ID_RADOPER, _("&Radical operations"), itemMenu4);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_SAVE, _("&Save query.."), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(wxID_OPEN, _("&Load query.."), _T(""), wxITEM_NORMAL);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_EXIT, _("&Quit"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu3, _("&File"));
    wxMenu* itemMenu13 = new wxMenu;
    wxMenu* itemMenu14 = new wxMenu;
    itemMenu14->Append(ID_SQLGETINFO, _("&Get info about tables"), _T(""), wxITEM_NORMAL);
    itemMenu14->Append(ID_SQLCOUNTSTARS, _("&Count stars"), _T(""), wxITEM_NORMAL);
    itemMenu14->Append(ID_PLANETCOUNT, _("Count of &planets per star"), _T(""), wxITEM_NORMAL);
    itemMenu14->Append(ID_MAXPLANETS, _("&Highest number of planets for a star"), _T(""), wxITEM_NORMAL);
    itemMenu13->Append(ID_MENU, _("&Information"), itemMenu14);
    wxMenu* itemMenu19 = new wxMenu;
    itemMenu19->Append(ID_SQLRESETHILITE, _("&Reset hilighted state"), _T(""), wxITEM_NORMAL);
    itemMenu19->Append(ID_HILIGHT_MOON_PLANETS, _("&Hilight all stars with planets that have moons"), _T(""), wxITEM_NORMAL);
    itemMenu13->Append(ID_MENU1, _("&Hilights"), itemMenu19);
    wxMenu* itemMenu22 = new wxMenu;
    itemMenu22->Append(ID_SQLSHRINK, _("&Shrink the galaxy"), _T(""), wxITEM_NORMAL);
    itemMenu22->Append(ID_TURNGALAXY, _("&Turn galaxy"), _T(""), wxITEM_NORMAL);
    itemMenu13->Append(ID_MENU2, _("&Alter geometry"), itemMenu22);
    itemMenu13->AppendSeparator();
    itemMenu13->Append(ID_MENURUNQUERY, _("&Run query\tF5"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu13, _("&SQL"));
    wxMenu* itemMenu27 = new wxMenu;
    itemMenu27->Append(ID_MENUGLOW, _("Render with &Glow"), _T(""), wxITEM_RADIO);
    itemMenu27->Append(ID_NORMALRENDER, _("Render &Normally"), _T(""), wxITEM_RADIO);
    itemMenu27->Check(ID_NORMALRENDER, true);
    itemMenu27->Append(ID_MENULOWQUALITY, _("Render in &Low-quality mode"), _T(""), wxITEM_RADIO);
    itemMenu27->AppendSeparator();
    itemMenu27->Append(ID_RENDERGRID, _("&Draw Grid"), _T(""), wxITEM_CHECK);
    menuBar->Append(itemMenu27, _("&Graphics"));
    wxMenu* itemMenu33 = new wxMenu;
    itemMenu33->Append(wxID_ABOUT, _("&About.."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu33, _("&Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxSplitterWindow* itemSplitterWindow35 = new wxSplitterWindow( itemFrame1, ID_SPLITTERWINDOW, wxDefaultPosition, wxSize(600, 100), wxSP_3DSASH|wxSP_LIVE_UPDATE|wxNO_BORDER );
    itemSplitterWindow35->SetMinimumPaneSize(1);

    wxSplitterWindow* itemSplitterWindow36 = new wxSplitterWindow( itemSplitterWindow35, ID_DISPLAYQUERYSPLITTER, wxDefaultPosition, wxSize(100, 100), wxSP_3DBORDER|wxSP_3DSASH|wxNO_BORDER );
    itemSplitterWindow36->SetMinimumPaneSize(1);

    mGfxPanel = new GfxPanel( itemSplitterWindow36, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );

    wxPanel* itemPanel38 = new wxPanel( itemSplitterWindow36, ID_QUERYEDITPANEL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel38->SetSizer(itemBoxSizer39);

    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer39->Add(itemBoxSizer40, 1, wxGROW|wxALL, 1);
    mQuery = new SqlQueryCtrl( itemPanel38, ID_QUERYEDIT);
    itemBoxSizer40->Add(mQuery, 1, wxGROW|wxALL, 1);

    wxBoxSizer* itemBoxSizer42 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer40->Add(itemBoxSizer42, 0, wxALIGN_RIGHT|wxALL, 5);
    mLoadQuery = new wxButton( itemPanel38, wxID_OPEN, _("&Load.."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer42->Add(mLoadQuery, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSaveQuery = new wxButton( itemPanel38, wxID_SAVE, _("S&ave.."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer42->Add(mSaveQuery, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mRunQuery = new wxButton( itemPanel38, ID_RUNQUERY, _("&Run Query"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer42->Add(mRunQuery, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemSplitterWindow36->SplitHorizontally(mGfxPanel, itemPanel38, 400);

    mNotebook = new wxNotebook( itemSplitterWindow35, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    wxPanel* itemPanel47 = new wxPanel( mNotebook, ID_GURUPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer48 = new wxBoxSizer(wxVERTICAL);
    itemPanel47->SetSizer(itemBoxSizer48);

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer48->Add(itemBoxSizer49, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    wxBitmap mGuruPictureBitmap(wxNullBitmap);
    mGuruPicture = new wxStaticBitmap( itemPanel47, wxID_STATIC, mGuruPictureBitmap, wxDefaultPosition, wxSize(180, 180), wxSTATIC_BORDER );
    itemBoxSizer49->Add(mGuruPicture, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxSHAPED, 5);

    wxBoxSizer* itemBoxSizer51 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer48->Add(itemBoxSizer51, 0, wxGROW|wxALL, 5);
    wxString* mChapterSelectStrings = NULL;
    mChapterSelect = new wxChoice( itemPanel47, ID_CHAPTERSELECT, wxDefaultPosition, wxDefaultSize, 0, mChapterSelectStrings, 0 );
    itemBoxSizer51->Add(mChapterSelect, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer53 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer48->Add(itemBoxSizer53, 4, wxGROW|wxALL, 1);
    mGuruSpeaks = new wxGuruSpeaksPanel( itemPanel47, ID_GURUSPEAKS, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer53->Add(mGuruSpeaks, 1, wxGROW|wxALL, 1);

    wxBoxSizer* itemBoxSizer55 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer48->Add(itemBoxSizer55, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    mGuruDone = new wxButton( itemPanel47, ID_GURU_DONE, _("Ok, I'm done.."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer55->Add(mGuruDone, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mNotebook->AddPage(itemPanel47, _("Guru"));

    wxPanel* itemPanel57 = new wxPanel( mNotebook, ID_QUERYPANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer58 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel57->SetSizer(itemBoxSizer58);

    wxBoxSizer* itemBoxSizer59 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer58->Add(itemBoxSizer59, 1, wxGROW|wxALL, 1);
    wxStaticText* itemStaticText60 = new wxStaticText( itemPanel57, wxID_STATIC, _("Query Result"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer59->Add(itemStaticText60, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    mQueryResult = new wxHtmlWindow( itemPanel57, ID_QUERYRESULT, wxDefaultPosition, wxSize(200, 150), wxHW_SCROLLBAR_AUTO|wxSTATIC_BORDER|wxHSCROLL|wxVSCROLL );
    mQueryResult->SetBackgroundColour(wxColour(192, 192, 192));
    itemBoxSizer59->Add(mQueryResult, 1, wxGROW|wxALL, 1);

    mNotebook->AddPage(itemPanel57, _("Query Result"));

    mHtmlPanel = new wxHtmlWindow( mNotebook, ID_HTMLWINDOW, wxDefaultPosition, wxSize(200, 150), wxHW_SCROLLBAR_AUTO|wxSTATIC_BORDER|wxHSCROLL|wxVSCROLL );

    mNotebook->AddPage(mHtmlPanel, _("Reference"));

    itemSplitterWindow35->SplitVertically(itemSplitterWindow36, mNotebook, 500);

////@end Galaxql content construction

    // Set the icon that appears in the task manager,
    // and various other places in the OS's window manager.
    wxIcon icon64x64(galaxql_64x64_xpm);
    wxIcon icon32x32(galaxql_32x32_xpm);
    wxIcon icon16x16(galaxql_16x16_xpm);

    wxIconBundle icons;
    icons.AddIcon(icon16x16);
    icons.AddIcon(icon32x32);
    icons.AddIcon(icon64x64);

    SetIcons(icons);
}

/*!
 * Should we show tooltips?
 */

bool Galaxql::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap Galaxql::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin Galaxql bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end Galaxql bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon Galaxql::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin Galaxql icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end Galaxql icon retrieval
}

/*!
 * GfxPanel type definition
 */

IMPLEMENT_CLASS( GfxPanel, wxGLCanvas )

/*!
 * GfxPanel event table definition
 */

BEGIN_EVENT_TABLE( GfxPanel, wxGLCanvas )
    EVT_PAINT(GfxPanel::onPaint)
    EVT_ERASE_BACKGROUND(GfxPanel::onEraseBackground)
    EVT_IDLE(GfxPanel::onIdle)
    EVT_MOUSE_EVENTS(GfxPanel::onMouseEvent)
END_EVENT_TABLE()

/*!
 * GfxPanel constructors
 */

GfxPanel::GfxPanel(wxWindow *parent, int, wxPoint, wxSize, int)
    : wxGLCanvas(parent),
      xpos(0),
      ypos(0),
      mVtx(0),
      mGatewayVtx(0),
      mCol(0),
      mStars(0),
      mRenderWithGlow(0),
      mRenderInLowQuality(0),
      mRenderGrid(0),
      mContext(this)
{
}

GfxPanel::~GfxPanel()
{
    delete[] mVtx;
    delete[] mCol;
    delete[] mGatewayVtx;
}


/*!
 * Should we show tooltips?
 */

bool GfxPanel::ShowToolTips()
{
    return TRUE;
}

void GfxPanel::render(float *vtx, float *col, int count) 
{ 
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    if (!mRenderInLowQuality)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glClearColor(0.5,0.5,0.5,1.0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }
 
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );

    wxSize s = GetSize();

    {
        float x = xpos;
        float y = ypos;
        float z = -1;
        
        float l = 1.5f / sqrt(x*x+y*y+z*z);
        x *= l;
        y *= l;
        z *= l;

        gluLookAt(x,y,z,0,0,0,1,0,0);
    }
//    glTranslatef( 0.0, -80.0, -200.0 );

    //glRotatef( 30, 1.0f, 0, 0);

    float angle = mTicker.Time() / 50000.0f;

    glRotatef( -60,0,1,0);
    glRotatef( -20,1,0,0);
    glRotatef( angle * 120, 0, 0, 1.0f);

    //glRotatef( angle*20, 0.5, 0.1,0.3);
    //glRotatef(xpos,0,1,0);
    //glRotatef(ypos,1,0,0);


    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, vtx);
    glColorPointer(4, GL_FLOAT, 0, col);

    glDisable(GL_CULL_FACE);

    if (!mRenderInLowQuality)
    {
        glEnable(GL_POINT_SMOOTH);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_POINT_SIZE);
        glEnable(GL_BLEND);
        glDisable(GL_DITHER);
        
        glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnableClientState(GL_COLOR_ARRAY);
    }
    else
    {
        glDisable(GL_POINT_SMOOTH);
        glEnable(GL_POINT_SIZE);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        //glDisableClientState(GL_COLOR_ARRAY);
        //glColor4f(0.8f,0.8f,0.8f,1);
    }

    if (mRenderGrid)
    {
        glBegin(GL_LINES);
            // main grid
            glColor4f(0.75,0,0,0.75);
            glVertex3f(-2,0,0);
            glVertex3f(2,0,0);
            glVertex3f(0,2,0);
            glVertex3f(0,-2,0);
            glVertex3f(0,0,2);
            glVertex3f(0,0,-2);

            // secondary grid
            glColor4f(0.5,0,0,0.5);
            glVertex3f(-2,1,0);
            glVertex3f(2,1,0);
            glVertex3f(-2,-1,0);
            glVertex3f(2,-1,0);

            glVertex3f(1,2,0);
            glVertex3f(1,-2,0);
            glVertex3f(-1,2,0);
            glVertex3f(-1,-2,0);

            glVertex3f(0,1,-2);
            glVertex3f(0,1,2);
            glVertex3f(0,-1,-2);
            glVertex3f(0,-1,2);

            glVertex3f(1,0,2);
            glVertex3f(1,0,-2);
            glVertex3f(-1,0,2);
            glVertex3f(-1,0,-2);

            // arrows
            glColor4f(0.75f,0,0,0.75f);
            glVertex3f(2,0,0);
            glVertex3f(1.8f,0.1f,0);
            glVertex3f(2,0,0);
            glVertex3f(1.8f,-0.1f,0);

            glVertex3f(0,2,0);
            glVertex3f(0.1f,1.8f,0);
            glVertex3f(0,2,0);
            glVertex3f(-0.1f,1.8f,0);

            glVertex3f(0,0,2);
            glVertex3f(0.1f,0,1.8f);
            glVertex3f(0,0,2);
            glVertex3f(-0.1f,0,1.8f);

            // letters
            // x
            glVertex3f(1.9f,0.15f,0);
            glVertex3f(1.7f,0.25f,0);
            glVertex3f(1.9f,0.25f,0);
            glVertex3f(1.7f,0.15f,0);

            // y
            glVertex3f(-0.15f,1.9f,0);
            glVertex3f(-0.2f,1.8f,0);
            glVertex3f(-0.25f,1.9f,0);
            glVertex3f(-0.2f,1.8f,0);
            glVertex3f(-0.2f,1.8f,0);
            glVertex3f(-0.2f,1.7f,0);
            
            // z
            glVertex3f(0.15f,0,1.9f);
            glVertex3f(0.25f,0,1.7f);
            glVertex3f(0.15f,0,1.9f);
            glVertex3f(0.25f,0,1.9f);
            glVertex3f(0.15f,0,1.7f);
            glVertex3f(0.25f,0,1.7f);

        glEnd();
    }

    
    glEnableClientState(GL_VERTEX_ARRAY);

    if (!mRenderWithGlow)
    {
        if (!mRenderInLowQuality)
            glPointSize(2.5);
        else
            glPointSize(1.0);
        glDrawArrays(GL_POINTS, 0, count);
    }
    else
    {
        float *tempcolor;
        tempcolor=new float[4*mStars];
        glColorPointer(4, GL_FLOAT, 0, tempcolor);
        int i;
        for (i = 0; i < mStars*4; i++)
            tempcolor[i] = col[i]*0.6;
        glPointSize(1);
        glDrawArrays(GL_POINTS, 0, count);
        for (i = 0; i < mStars*4; i++)
            tempcolor[i] = col[i]*0.5;
        glPointSize(2.5);
        glDrawArrays(GL_POINTS, 0, count);      
        for (i = 0; i < mStars*4; i++)
            tempcolor[i] = col[i]*0.4;
        glPointSize(5);
        glDrawArrays(GL_POINTS, 0, count);
        for (i = 0; i < mStars*4; i++)
            tempcolor[i] = col[i]*0.3;
        glPointSize(10);
        glDrawArrays(GL_POINTS, 0, count);      
        for (i = 0; i < mStars*4; i++)
            tempcolor[i] = col[i]*0.2;
        glPointSize(20);
        glDrawArrays(GL_POINTS, 0, count);      

        delete[] tempcolor;
    }

    glDisableClientState(GL_COLOR_ARRAY);

    glDisableClientState(GL_VERTEX_ARRAY);    

    glDisable(GL_DEPTH_TEST);

    if (mGateways > 0)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        
        glVertexPointer(3, GL_FLOAT, 0, mGatewayVtx);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glLineWidth(2);
        glColor4f(0,0,0.5f,0.25f);
        glDrawArrays(GL_LINES, 0, mGateways*2);        

        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if (mHilighted)
    {

        float test[2*3];
        test[0] = mHilightInfo.x;
        test[1] = mHilightInfo.y;
        test[2] = mHilightInfo.z;
        double ox,oy,oz;
        double proj[16], model[16];
        GLint viewport[4];
        
        glGetDoublev(GL_PROJECTION_MATRIX,&proj[0]);
        glGetDoublev(GL_MODELVIEW_MATRIX,&model[0]);
        glGetIntegerv(GL_VIEWPORT,&viewport[0]);
        gluUnProject(s.GetWidth()*0.5,s.GetHeight()*0.24,0,model,proj,viewport,&ox,&oy,&oz);
        test[3] = ox;
        test[4] = oy;
        test[5] = oz;

        glEnableClientState(GL_VERTEX_ARRAY);
        
        glVertexPointer(3, GL_FLOAT, 0, test);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glLineWidth(1.5);
        glColor4f(1.0f,0,0,0.5f);
        glDrawArrays(GL_LINES, 0, 2);        

        glDisableClientState(GL_VERTEX_ARRAY);
    }


    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
}

void GfxPanel::onMouseEvent(wxMouseEvent &event)
{
    if (event.Dragging())
    {
        wxSize s = GetSize();
        ypos = -(event.GetX() - (s.GetWidth()/2.0f))/(s.GetWidth()/4.0f); 
        xpos = (event.GetY() - (s.GetHeight()/2.0f))/(s.GetHeight()/4.0f);
    }
}

inline void GfxPanel::onEraseBackground(wxEraseEvent &) {}

int sql_callback_count(void*userptr,int columns,char**values, char**names)
{
    GfxPanel * d = (GfxPanel*)userptr;
    d->mStars = atoi(values[0]);
    if (d->mStars > MAX_STARS)
        d->mStars = MAX_STARS;
    return 0;
}

int sql_callback_hilight(void*userptr,int columns,char**values, char**names)
{
    GfxPanel * d = (GfxPanel*)userptr;
    d->mHilighted = atoi(values[0]);
    d->mHilightInfo.starclass = atoi(values[1]);
    d->mHilightInfo.x = atof(values[2]);
    d->mHilightInfo.y = atof(values[3]);
    d->mHilightInfo.z = atof(values[4]);
    if (d->mHilighted > MAX_STARS)
        d->mHilighted = 0;
    return 1;
}

int sql_callback_hilightinfo(void*userptr,int columns,char**values, char**names)
{
    //planets.orbitdistance,planets.color,moons.orbitdistance,moons.color
    GfxPanel * d = (GfxPanel*)userptr;
    float planetdistance = atof(values[0]);
    int planetcolor = atoi(values[1]);
    // new planet?
    if (d->mHilightInfo.planets == 0 || d->mHilightInfo.planet[d->mHilightInfo.planets-1].distance != planetdistance)
    {
        // This is probably a new planet (assuming that all planets have different orbital distances)
        // Store the planet's information.
        d->mHilightInfo.planet[d->mHilightInfo.planets].distance = planetdistance;
        d->mHilightInfo.planet[d->mHilightInfo.planets].color = planetcolor;

        d->mHilightInfo.planets++;
        if (d->mHilightInfo.planets >= 20)
            return 1;
    }
    // do we have a moon?
    if (values[2] != 0)
    {
        float moondistance = atof(values[2]);
        int mooncolor = atoi(values[3]);
        d->mHilightInfo.moon[d->mHilightInfo.planets-1][d->mHilightInfo.moons[d->mHilightInfo.planets-1]].distance = moondistance;
        d->mHilightInfo.moon[d->mHilightInfo.planets-1][d->mHilightInfo.moons[d->mHilightInfo.planets-1]].color = mooncolor;
        d->mHilightInfo.moons[d->mHilightInfo.planets-1]++;
        if (d->mHilightInfo.moons[d->mHilightInfo.planets-1] >= 20)
            return 1;
    }
    return 0;
}


int sql_callback_grab(void*userptr,int columns,char**values, char**names)
{
    GfxPanel * d = (GfxPanel*)userptr;
    if (d->mInspectedStar >= MAX_STARS)
        return 1;
    
    d->mVtx[d->mInspectedStar*3+0] = atof(values[0]);
    d->mVtx[d->mInspectedStar*3+1] = atof(values[1]);
    d->mVtx[d->mInspectedStar*3+2] = atof(values[2]);

    int color = atoi(values[3])+3;
    float intensity = atof(values[4]);
    int hilite = values[5]?atoi(values[5]):0;
    d->mCol[d->mInspectedStar*4+0]=(((starcolors[color]>>0) & 0xff) / (float)0xff)*(intensity) + hilite;
    d->mCol[d->mInspectedStar*4+1]=(((starcolors[color]>>8) & 0xff) / (float)0xff)*(intensity) + hilite;
    d->mCol[d->mInspectedStar*4+2]=(((starcolors[color]>>16) & 0xff) / (float)0xff)*(intensity) + hilite;
    d->mCol[d->mInspectedStar*4+3]=1;
    
    d->mInspectedStar++;
    
    return 0;
}

int sql_callback_gateways(void*userptr,int columns,char**values, char**names)
{
    GfxPanel * d = (GfxPanel*)userptr;
    
    d->mGatewayVtx[d->mGateways*6+0] = atof(values[0]);
    d->mGatewayVtx[d->mGateways*6+1] = atof(values[1]);
    d->mGatewayVtx[d->mGateways*6+2] = atof(values[2]);
    d->mGatewayVtx[d->mGateways*6+3] = atof(values[3]);
    d->mGatewayVtx[d->mGateways*6+4] = atof(values[4]);
    d->mGatewayVtx[d->mGateways*6+5] = atof(values[5]);

    d->mGateways++;
    
    return 0;
}

// Renders the hilighted star system (star, planets, and moons) into the panel.
void GfxPanel::render_system()
{
    float angle = (mTicker.Time() / 5000.0f) + 1337;
    int vtxcount = 1;
    vtxcount += mHilightInfo.planets;
    int i;
    for (i = 0; i < mHilightInfo.planets; i++)
        vtxcount += mHilightInfo.moons[i];
    float *vtx = new float[3*vtxcount]; // array of vertices [x,y,z]
    float *col = new float[4*vtxcount]; // array of colors [r,g,b,alpha]

    vtx[0]=vtx[1]=0;
    vtx[2]=-1;
    int color = mHilightInfo.starclass + 3;
    col[0]=(((starcolors[color]>>0) & 0xff) / (float)0xff);//*intensity + hilite;
    col[1]=(((starcolors[color]>>8) & 0xff) / (float)0xff);//*intensity + hilite;
    col[2]=(((starcolors[color]>>16) & 0xff) / (float)0xff);//*intensity + hilite;
    col[3]=1;

    int moonid = 0;
    int moonoffset = mHilightInfo.planets + 1;
    for (i = 0; i < mHilightInfo.planets; i++)
    {
        float d = mHilightInfo.planet[i].distance / 150000;
        vtx[i*3+3+0]=sin(angle/d)*d;
        vtx[i*3+3+1]=cos(angle/d)*d;
        vtx[i*3+3+2]=-1;
        int color = mHilightInfo.planet[i].color / 2;
        if (color < 0 || color > 9) color = 0;
        col[i*4+4+0]=(((planetcolors[color]>>0) & 0xff) / (float)0xff);//*intensity + hilite;
        col[i*4+4+1]=(((planetcolors[color]>>8) & 0xff) / (float)0xff);//*intensity + hilite;
        col[i*4+4+2]=(((planetcolors[color]>>16) & 0xff) / (float)0xff);//*intensity + hilite;
        col[i*4+4+3]=1;
        int j;
        for (j = 0; j < mHilightInfo.moons[i]; j++)
        {
            float md = mHilightInfo.moon[i][j].distance / 150000;
            vtx[moonid*3+moonoffset*3+0] = vtx[i*3+3+0] + sin(angle/md)*md;
            vtx[moonid*3+moonoffset*3+1] = vtx[i*3+3+1] + cos(angle/md)*md;
            vtx[moonid*3+moonoffset*3+2] = -1;           
            int color = mHilightInfo.moon[i][j].color / 2;
            if (color < 0 || color > 9) color = 0;
            col[moonid*4+moonoffset*4+0]=(((planetcolors[color]>>0) & 0xff) / (float)0xff);//*intensity + hilite;
            col[moonid*4+moonoffset*4+1]=(((planetcolors[color]>>8) & 0xff) / (float)0xff);//*intensity + hilite;
            col[moonid*4+moonoffset*4+2]=(((planetcolors[color]>>16) & 0xff) / (float)0xff);//*intensity + hilite;
            col[moonid*4+moonoffset*4+3]=1;
            moonid++;
        }
    }
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();

    glColor4f((float)(0x24/(float)0xff), 
                (float)(0x3a/(float)0xff),
                (float)(0x49/(float)0xff), 0.5 );
    wxSize s = GetSize();

    glViewport( (int)((s.GetWidth() - s.GetWidth()/1.5f)/2.0f), -s.GetHeight()/10, (int)(s.GetWidth() / 1.5f), (int)(s.GetHeight() / 1.5f));

    {
        float x = -0.75f;//xpos;
        float y = 0;//ypos;
        float z = -1.0f;
        
        float l = 2.5f / sqrt(x*x+y*y+z*z);
        x *= l;
        y *= l;
        z *= l;

        gluLookAt(x,y,z,0,0,-1,1,0,0);
    }

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
        glVertex3f(-0.7f, 0.7f,-1.1f);
        glVertex3f( 0.7f, 0.7f,-1.1f);
        glVertex3f( 0.7f,-0.7f,-1.1f);
        glVertex3f(-0.7f,-0.7f,-1.1f);
    /*
        glVertex3f(-0.3,-0.175,-1);
        glVertex3f(0.3,-0.175,-1);
        glVertex3f(0.9,-0.75,-1);
        glVertex3f(-0.9,-0.75,-1);
        */
    glEnd();


    glEnableClientState(GL_VERTEX_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, vtx);
    glColor4f(0,0,0,0.5f);

    glPointSize(7);
    glDrawArrays(GL_POINTS, 0, 1);        
    glPointSize(5);
    glDrawArrays(GL_POINTS, 1, mHilightInfo.planets);        
    glPointSize(2);
    glDrawArrays(GL_POINTS, 1+mHilightInfo.planets, vtxcount - (mHilightInfo.planets+1));        

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_FLOAT, 0, col);

    for (i = 0; i < vtxcount; i++)
        vtx[i*3+2]=-1.04f;

    glPointSize(7);
    glDrawArrays(GL_POINTS, 0, 1);        
    glPointSize(5);
    glDrawArrays(GL_POINTS, 1, mHilightInfo.planets);        
    glPointSize(2);
    glDrawArrays(GL_POINTS, 1+mHilightInfo.planets, vtxcount - (mHilightInfo.planets+1));        

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    delete[] col;
    delete[] vtx;

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

};

void GfxPanel::onPaint(wxPaintEvent &)
{
    wxPaintDC dc(this);
    SetCurrent(mContext);

    GalaxqlApp * app = (GalaxqlApp*)wxTheApp;  

    if (mVtx == NULL)
    {
        mStars = 0;
        sqlite3_exec(
                app->db,
                "select count() from gateways",
                sql_callback_count,
                this,
                NULL);
        mGateways = mStars;

        mStars = 0;
        sqlite3_exec(
                app->db,
                "select count() from stars",
                sql_callback_count,
                this,
                NULL);

        delete[] mVtx;
        delete[] mCol;
        delete[] mGatewayVtx;
        mVtx = new float[mStars * 3];
        mCol = new float[mStars * 4];
        mGatewayVtx = new float[mGateways * 3 * 2];
        mInspectedStar = 0;
        sqlite3_exec(
                app->db, // Since NULL*0+1=NULL, light is either NULL or 1.
                "SELECT x,y,z,class,intensity,hilight.starid*0+1 AS light "
                "FROM stars "
                "LEFT JOIN hilight "
                "ON stars.starid=hilight.starid",                
                sql_callback_grab,
                this,
                NULL);

        mHilighted = 0;
        sqlite3_exec(
                app->db,
                "SELECT stars.starid, class, x, y, z "
                "FROM stars,hilight "
                "WHERE stars.starid=hilight.starid",
                sql_callback_hilight,
                this,
                NULL);
        if (mHilighted)
        {
            mHilightInfo.planets = 0;
            int i;
            for (i = 0; i < 20; i++)
                mHilightInfo.moons[i] = 0;
            char temp[256];
            sprintf(temp, "SELECT planets.orbitdistance,planets.color,moons.orbitdistance,moons.color "
                          "FROM planets "
                          "LEFT JOIN moons "
                          "ON planets.starid=%d "
                          "AND planets.planetid=moons.planetid",mHilighted);
            sqlite3_exec(
                    app->db,
                    temp,
                    sql_callback_hilightinfo,
                    this,
                    NULL);
        }
        if (mGateways > 0)
        {
            mGateways = 0;
            sqlite3_exec(
                    app->db,
                    "SELECT st1.x,st1.y,st1.z,st2.x,st2.y,st2.z "
                    "FROM stars AS st1, stars AS st2, gateways "
                    "WHERE st1.starid=gateways.star1 "
                    "AND st2.starid=gateways.star2",
                    sql_callback_gateways,
                    this,
                    NULL);
        }
    }


    wxSize s = GetSize();


    glShadeModel( GL_SMOOTH );

    glCullFace( GL_BACK );
    glFrontFace( GL_CCW );
    glDisable( GL_CULL_FACE );

    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor( (float)(0x24/(float)0xff*0.25), 
                  (float)(0x3a/(float)0xff*0.25),
                  (float)(0x49/(float)0xff*0.25), 0 );

    glViewport( 0, 0, s.GetWidth(), s.GetHeight());

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );
    gluPerspective( 70.0, s.GetWidth()/(float)s.GetHeight(), 0.01, 1024.0 );

    glClear(GL_COLOR_BUFFER_BIT);    

    render(mVtx,mCol,mStars);

    if (mHilighted)
    {
        render_system();
    }

    glFlush(); 
    SwapBuffers();
}

void GfxPanel::onIdle(wxIdleEvent &event)
{
    // refresh the panel
    Refresh(false);
    
    // If we don't request more 'onIdle' events, the starmap will just stay idle
    // until the next window message (on some platforms)
    event.RequestMore();

    // throttle to keep from flooding the event queue
    wxMilliSleep(33);
}

/*!
 * Get bitmap resources
 */

wxBitmap GfxPanel::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin GfxPanel bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end GfxPanel bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon GfxPanel::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin GfxPanel icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end GfxPanel icon retrieval
}

#if 0 // dialogblocks wants to regenerate these all the time..

/*!
 * GfxPanel type definition
 */

IMPLEMENT_DYNAMIC_CLASS( GfxPanel, wxGLCanvas )

/*!
 * GfxPanel event table definition
 */

BEGIN_EVENT_TABLE( GfxPanel, wxGLCanvas )

////@begin GfxPanel event table entries
////@end GfxPanel event table entries

END_EVENT_TABLE()

/*!
 * GfxPanel constructors
 */

GfxPanel::GfxPanel( )
{
}

GfxPanel::GfxPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * GfxPanel creator
 */

bool GfxPanel::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin GfxPanel member initialisation
////@end GfxPanel member initialisation

////@begin GfxPanel creation
    wxGLCanvas::Create( parent, id, pos, size, style );

    CreateControls();
////@end GfxPanel creation
    return TRUE;
}

/*!
 * Control creation for GfxPanel
 */

void GfxPanel::CreateControls()
{    
////@begin GfxPanel content construction
    // Generated by DialogBlocks, 03/06/2006 15:22:22 (Personal Edition)

    GfxPanel* itemGLCanvas37 = this;

////@end GfxPanel content construction
}

/*!
 * Should we show tooltips?
 */

bool GfxPanel::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap GfxPanel::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin GfxPanel bitmap retrieval
    return wxNullBitmap;
////@end GfxPanel bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon GfxPanel::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin GfxPanel icon retrieval
    return wxNullIcon;
////@end GfxPanel icon retrieval
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
 */

#endif 

int sql_callback(void*userptr,int columns,char**values, char**names)
{
    Query * d = (Query*)userptr;

    int i;
    if (d->mRows == 0)
    {
        if (columns > 32)
        {
            d->mStatus = wxT("Dataset too wide. Columns truncated. ");
            columns = 32;
        }
        
        d->mColumns = columns;
        for (i = 0; i < columns; i++)
        {
            d->mResults[d->mRows][i] = wxString(names[i],wxConvUTF8);
        }
        d->mRows++;
    }
    if (columns > 32)
    {
        columns = 32;
    }

    for (i = 0; i < columns; i++)
    {
        // TODO: handle NULLs in some other way
        if (values[i]== NULL)
            d->mResults[d->mRows][i] = wxT("<font color=red>NULL</font>");
        else
        {
            d->mResults[d->mRows][i] = wxString(values[i], wxConvUTF8);
            // TODO: this should be done when printing the data..
            d->mResults[d->mRows][i].Replace(wxT("\n"),wxT("<br>"),true);

        }
    }
    
    d->mRows++;

    if (d->mRows == 1001)
    {
        d->mStatus += wxT("Dataset too large. Results truncated.");
        return 1;
    }
    return 0;
}

int guess_align(const char * data)
{
    const char * walker = data;
    while (*walker)
    {
        if ((*walker < '0' || *walker > '9') && *walker != '-')
            return 1;
        walker++;
    }
    return 0;
}

void Galaxql::OnRunqueryClick( wxCommandEvent& WXUNUSED(event) )
{
    GalaxqlApp * app = (GalaxqlApp*)wxTheApp;  
    Galaxql * g = (Galaxql*)app->GetTopWindow();
    mLoadQuery->Enable(0);
    mSaveQuery->Enable(0);
    mRunQuery->Enable(0);
    char *errmsg = NULL;
    mUserQuery.mRows = 0;
    mUserQuery.mColumns = 0;
    mUserQuery.mStatus = wxT("");
    int i,j;
    for (i = 0; i < 1001; i++)
        for (j = 0; j < 32; j++)
            mUserQuery.mResults[i][j] = wxT("");

    int rc =  sqlite3_exec(
                app->db,
                mQuery->GetText().utf8_str(),
                sql_callback,
                &mUserQuery,
                &errmsg);

    if (rc != SQLITE_OK && rc != SQLITE_ABORT)
    {
        wxString s;
        s = wxT("<html><body bgcolor=#c0c0c0><font color=red>");
        s += wxString(errmsg,wxConvUTF8);
        s += wxT("</font></body></html>");
        mQueryResult->SetPage(s);
        sqlite3_free(errmsg);
    }
    else
    {
        if (mUserQuery.mRows < 1)
        {
            mQueryResult->SetPage(wxT("<html><body bgcolor=#c0c0c0>Query ok - no rows returned</body></html>"));
        }
        else
        {
            // wxwidgets string gets slow when handling long strings,
            // so compiling one row of html in a temporary string is
            // MUCH faster than adding to the master string all the time.
            wxString s,s2;
            s+=wxT("<html><body bgcolor=#c0c0c0><table border=0 cellpadding=0 cellspacing=2>");
            int i;
            for (i = 0; i < mUserQuery.mRows; i++)
            {
                s2=wxT("");
                if (i == 0)
                    s2 += wxT("<tr bgcolor=#f0f0f0>");
                else
                if ((((i+1) >> 1) & 1) == 0)
                    s2 += wxT("<tr bgcolor=#d0d0d0>");
                else
                    s2 += wxT("<tr bgcolor=#b0b0b0>");
                int j;
                for (j = 0; j < mUserQuery.mColumns; j++)
                {
                    if (i == 0)
                    {
                        s2 += wxT("<td align=center><b>");
                    }
                    else
                    {
                        if (guess_align(mUserQuery.mResults[i][j].mb_str()))
                            s2 += wxT("<td>");
                        else
                            s2 += wxT("<td align=right>");
                    }
                    s2 += mUserQuery.mResults[i][j];

                    if (i == 0)
                    {
                        s2 += wxT("</b>");
                    }
                    s2 += wxT("</td>");
                }
                s2 += wxT("</tr>");
                s += s2;
            }
            s2 = wxT("");
            s2 += wxT("</table><br clear=all>");
            s2 += mUserQuery.mStatus;
            s2 += wxT("</body></html>");
            s += s2;
            mQueryResult->SetPage(s);
        }
        // refresh starmap. Only really needed when the database changes,
        // but there's no dead-easy way to detect this.        
        delete[] mGfxPanel->mVtx;
        delete[] mGfxPanel->mCol;
        mGfxPanel->mVtx = NULL;
        mGfxPanel->mCol = NULL;
    }

    mLoadQuery->Enable(1);
    mSaveQuery->Enable(1);
    mRunQuery->Enable(1);
    
    // only switch if there are results..
    if (mUserQuery.mRows > 0 || (rc != SQLITE_OK && rc != SQLITE_ABORT))
        g->mNotebook->SetSelection(1);
}


void Galaxql::RegenerateWorld(int which)
{
    // With GTK, no window events are processed for the
    // duration of this call, including the repaint events.
    // The first thing we do is explicitly repaint the
    // application window so that the menus which invoked
    // this function will disappear.
    if (IsShown())
    {
        wxPaintEvent dummyPaintEvent;
        mGfxPanel->onPaint(dummyPaintEvent);
        Refresh();
        Update();
    }

    // Because this routine can take many seconds to complete,
    // we show a progress bar to the user.
    RegenerationDialog * reg = new RegenerationDialog(this);
    reg->UpdateProgress(0);
    reg->Show(true);

    GalaxqlApp * app = (GalaxqlApp*)wxTheApp;
    sqlite3_close(app->db);
    remove("galaxql.db");
    sqlite3_open("galaxql.db",&app->db);

    sqlite3_exec(
                app->db,
                "CREATE TABLE stars ( "
                "starid INTEGER PRIMARY KEY, "
                "name TEXT, "
                "x DOUBLE NOT NULL, "
                "y DOUBLE NOT NULL, "
                "z DOUBLE NOT NULL, "
                "class INTEGER NOT NULL, "
                "intensity DOUBLE NOT NULL "
                ");",
                NULL,
                NULL,
                NULL);

    sqlite3_exec(
                app->db,
                "CREATE TABLE hilight ("
                "starid INTEGER UNIQUE, "
                "FOREIGN KEY(starid) REFERENCES stars(starid) "
                ");",
                NULL,
                NULL,
                NULL);

    sqlite3_exec(
                app->db,
                "CREATE TABLE planets ("
                "planetid INTEGER PRIMARY KEY, "
                "starid INTEGER NOT NULL, "
                "orbitdistance DOUBLE NOT NULL, "
                "name TEXT, "
                "color INTEGER NOT NULL, "
                "radius DOUBLE NOT NULL, "
                "FOREIGN KEY(starid) REFERENCES stars(starid) "
                ");",
                NULL,
                NULL,
                NULL);

    sqlite3_exec(
                app->db,
                "CREATE TABLE moons ("
                "moonid INTEGER PRIMARY KEY, "
                "planetid INTEGER NOT NULL, "
                "orbitdistance DOUBLE NOT NULL, "
                "name TEXT, "
                "color INTEGER NOT NULL, "
                "radius DOUBLE NOT NULL, "
                "FOREIGN KEY(planetid) REFERENCES planets(planetid)"
                ");",
                NULL,
                NULL,
                NULL);

    sqlite3_exec(
                app->db,
                "BEGIN",
                NULL,
                NULL,
                NULL);

    switch (which)
    {
    case 1:
        {
            int planetid = 0;
            float step = 1.0f/2.5f;
            srand(0);
            int i;
            for (i = 0; i < 25000; i++)
            {
                float x = sin(pow((float)i*step,0.1f)*10 + (i&3)*3.14*0.5) + (rand() / (float)(RAND_MAX/2)-1)*0.6;
                float y = cos(pow((float)i*step,0.1f)*10 + (i&3)*3.14*0.5) + (rand() / (float)(RAND_MAX/2)-1)*0.6;
                float l = sqrt(x*x+y*y);
                float z = sin((i*step)/1000.0)*((10000-(i*step))/10000.0f) * (rand() / (float)(RAND_MAX/2)-1) * 0.2;
                x = (x/l) * 0.5 * i * 0.00025 * step;
                y = (y/l) * 0.5 * i * 0.00025 * step;
                z = z * 0.75;
                int color = rand()%14;
                float intensity = 2*(rand()%31 + (14-color))/450.0f;
                char temp[1024];
                sprintf(temp,"INSERT INTO stars ("
                        "name,"
                        "x,"
                        "y,"
                        "z,"
                        "class,"
                        "intensity"
                        ") VALUES ("
                        "'%c-%05d',"
                        "%3.9f,"
                        "%3.9f,"
                        "%3.9f,"
                        "%d,"
                        "%3.9f"
                        ");",
                        "OBAFGKM"[color/2],i,
                        x,
                        y,
                        z,
                        color-3,
                        intensity);

                sqlite3_exec(
                        app->db,
                        &temp[0],
                        NULL,
                        NULL,
                        NULL);

                if (rand() % 100 < 20)
                {
                    // 20% of stars have planets
                    int planets = (int)(pow((rand()/(float)(RAND_MAX)),2)*20);
                    int p;
                    for (p = 0; p < planets; p++)
                    {
                        sprintf(temp,"INSERT INTO planets ("
                                "starid, "
                                "orbitdistance, "
                                "name, "
                                "color, "
                                "radius "
                                ") VALUES ("
                                "%d,"
                                "%3.9f,"
                                "'%c-%05d/%c',"
                                "%d,"
                                "%3.9f"
                                ");",
                                i+1,
                                (rand()/(float)(RAND_MAX))*1000+((p+1) / 20.0f) * 100000,
                                "OBAFGKM"[color/2],i+1,"ABCDEFGHIJKLMNOPQRSTUVWXYZ"[p],
                                rand()%16,
                                (rand()/(float)(RAND_MAX))*90+10);

                        sqlite3_exec(
                                app->db,
                                &temp[0],
                                NULL,
                                NULL,
                                NULL);
                        planetid++;


                        if (rand() % 100 < 20)
                        {
                            // 20% of planets have moons
                            int moons = (int)(pow((rand()/(float)(RAND_MAX)),2)*20);
                            int m;
                            for (m = 0; m < moons; m++)
                            {
                                sprintf(temp,"INSERT INTO moons ("
                                        "planetid, "
                                        "orbitdistance, "
                                        "name, "
                                        "color, "
                                        "radius "
                                        ") VALUES ("
                                        "%d,"
                                        "%3.9f,"
                                        "'%c-%05d/%c-%d',"
                                        "%d,"
                                        "%3.9f"
                                        ");",
                                        planetid,
                                        (rand()/(float)(RAND_MAX))*100+((m + 1) / 20.0f) * 10000,
                                        "OBAFGKM"[color/2],i,"ABCDEFGHIJKLMNOPQRSTUVWXYZ"[p],m+1,
                                        rand()%16,
                                        (rand()/(float)(RAND_MAX))*9+1);

                                sqlite3_exec(
                                        app->db,
                                        &temp[0],
                                        NULL,
                                        NULL,
                                        NULL);
                            }
                        }
                    }

                }

                reg->UpdateProgress((i * 100)/25000);
            }
        }
        break;
    case 2:
        {
            float step = 1.0f;
            srand(0);
            int i;
            for (i = 0; i < 10000; i++)
            {
                float x = sin(pow((float)i*step,0.1f)*10 + (i&3)*3.14*0.5) + (rand() / (float)(RAND_MAX/2)-1)*0.6;
                float y = cos(pow((float)i*step,0.1f)*10 + (i&3)*3.14*0.5) + (rand() / (float)(RAND_MAX/2)-1)*0.6;
                float l = sqrt(x*x+y*y);
                float z = sin((i*step)/1000.0)*((10000-(i*step))/10000.0f) * (rand() / (float)(RAND_MAX/2)-1) * 0.2;
                x = (x/l) * 0.5 * i * 0.00025 * step;
                y = (y/l) * 0.5 * i * 0.00025 * step;
                z = z * 0.75;
                int color = rand()%14;
                float intensity = 2*(rand()%31 + (14-color))/450.0f;
                char temp[1024];
                sprintf(temp,"INSERT INTO stars ("
                        "name,"
                        "x,"
                        "y,"
                        "z,"
                        "class,"
                        "intensity"
                        ") VALUES ("
                        "'%c-%05d',"
                        "%3.9f,"
                        "%3.9f,"
                        "%3.9f,"
                        "%d,"
                        "%3.9f"
                        ");",
                        "OBAFGKM"[color/2],i,
                        x,
                        y,
                        z,
                        color-3,
                        intensity);

                sqlite3_exec(
                        app->db,
                        &temp[0],
                        NULL,
                        NULL,
                        NULL);
                reg->UpdateProgress((i * 100) / 10000);
            }
        }
        break;
    case 3:
        {
            srand(0);
            int i;
            for (i = 0; i < 1000; i++)
            {
                float x = (rand() / (float)(RAND_MAX/2)-1);
                float y = (rand() / (float)(RAND_MAX/2)-1);
                float z = (rand() / (float)(RAND_MAX/2)-1);
                float l = sqrt(x*x+y*y+z*z);
                x/=l;
                y/=l;
                z/=l;
                x*=0.5;
                y*=0.5;
                z*=0.5;

                int color = rand()%14;
                float intensity = 2*(rand()%31 + (14-color))/450.0f;
                char temp[1024];
                sprintf(temp,"INSERT INTO stars ("
                        "name,"
                        "x,"
                        "y,"
                        "z,"
                        "class,"
                        "intensity"
                        ") VALUES ("
                        "'%c-%05d',"
                        "%3.9f,"
                        "%3.9f,"
                        "%3.9f,"
                        "%d,"
                        "%3.9f"
                        ");",
                        "OBAFGKM"[color/2],i,
                        x,
                        y,
                        z,
                        color-3,
                        intensity);

                sqlite3_exec(
                        app->db,
                        &temp[0],
                        NULL,
                        NULL,
                        NULL);

                reg->UpdateProgress((i * 100) / 1000);
            }
        }
        break;
    }

    sqlite3_exec(
                app->db,
                "COMMIT",
                NULL,
                NULL,
                NULL);

    // We're close enough to being done to show 100% progress.
    reg->UpdateProgress(100);

    sqlite3_exec(
                app->db,
                "CREATE INDEX planets_starid ON planets (starid)",
                NULL,
                NULL,
                NULL);

    sqlite3_exec(
                app->db,
                "CREATE INDEX moons_planetid ON moons (planetid)",
                NULL,
                NULL,
                NULL);

    delete[] mGfxPanel->mVtx;
    delete[] mGfxPanel->mCol;
    mGfxPanel->mVtx = NULL;
    mGfxPanel->mCol = NULL;
    reg->Show(false);
    delete reg;
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_REGENERATE25K
 */

void Galaxql::OnRegenerate25kClick( wxCommandEvent& WXUNUSED(event) )
{
    RegenerateWorld(1);
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU
 */

void Galaxql::OnQuitappClick( wxCommandEvent& WXUNUSED(event) )
{
    Close();
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ABOUT
 */

void Galaxql::OnAboutClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString msg;
    msg.Printf(
        wxT("GalaXQL\n")
        wxT("\n")
        wxT("An interactive SQL tutorial\n")
        wxT("\n")
        wxT("Copyright 2005-2006 Jari Komppa\n")
        wxT("\n")
        wxT("http://iki.fi/sol\n")
        wxT("\n")
        wxT("Uses wxWidgets and SQLite\n")
        wxT("http://www.wxwidgets.org\n")
        wxT("http://www.sqlite.org\n")
        wxT("\n")
        wxT("Prof. Guru images by Mikko Finneman"));

    wxMessageBox(msg, wxT("About GalaXQL"),
                 wxOK | wxICON_INFORMATION, this);
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SQLGETINFO
 */

void Galaxql::OnSqlgetinfoClick( wxCommandEvent& WXUNUSED(event) )
{
    mQuery->SetText(wxT("SELECT sql FROM sqlite_master WHERE type='table'"));    
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SQLCOUNTSTARS
 */

void Galaxql::OnSqlcountstarsClick( wxCommandEvent& WXUNUSED(event) )
{
    mQuery->SetText(wxT("SELECT COUNT() FROM stars"));    
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SQLRESETHILITE
 */

void Galaxql::OnSqlresethiliteClick( wxCommandEvent& WXUNUSED(event) )
{
    mQuery->SetText(wxT("DELETE FROM hilight"));
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_REGENERATE10K
 */

void Galaxql::OnRegenerate10kClick( wxCommandEvent& WXUNUSED(event) )
{
    RegenerateWorld(2);
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_REGENERATE1KSPHERE
 */

void Galaxql::OnRegenerate1ksphereClick( wxCommandEvent& WXUNUSED(event) )
{
    RegenerateWorld(3);
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SQLSHRINK
 */

void Galaxql::OnSqlshrinkClick( wxCommandEvent& WXUNUSED(event) )
{
    mQuery->SetText(
        wxT("UPDATE stars SET\n")
        wxT("  x=x*0.5,\n")
        wxT("  y=y*0.5,\n")
        wxT("  z=z*0.5"));
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_HILIGHT_MOON_PLANETS
 */

void Galaxql::OnHilightMoonPlanetsClick( wxCommandEvent& WXUNUSED(event) )
{
    mQuery->SetText(
                wxT("INSERT INTO hilight\n")
                wxT("  SELECT DISTINCT starid\n")
                wxT("  FROM planets,moons\n")
                wxT("  WHERE moons.planetid=planets.planetid\n"));
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_TURNGALAXY
 */

void Galaxql::OnTurngalaxyClick( wxCommandEvent& WXUNUSED(event) )
{
    mQuery->SetText(wxT("UPDATE stars SET x=z, z=x"));
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_PLANETCOUNT
 */

void Galaxql::OnPlanetcountClick( wxCommandEvent& WXUNUSED(event) )
{
    mQuery->SetText(
        wxT("SELECT stars.starid AS starid,\n")
        wxT("    COUNT(planets.planetid) AS planet_count\n")
        wxT("FROM stars LEFT OUTER JOIN planets\n")
        wxT("    ON stars.starid=planets.starid \n")
        wxT("GROUP BY stars.starid \n"));
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MAXPLANETS
 */

void Galaxql::OnMaxplanetsClick( wxCommandEvent& WXUNUSED(event) )
{
    mQuery->SetText(
        wxT("SELECT COALESCE(MAX(planet_count), 0) AS max_planet_count\n")
        wxT("FROM (SELECT COUNT(planetid) AS planet_count\n")
        wxT("    FROM planets\n")
        wxT("    GROUP BY starid)\n"));
}

float Galaxql::min_col(Query &q, int colno)
{
    double val;
    q.mResults[1][colno].ToDouble(&val);
    int i;
    for (i = 2; i < q.mRows; i++)
    {
        double comp;
        q.mResults[i][colno].ToDouble(&comp);
        if (comp < val)
            val = comp;
    }
    return (float)val;
}

float Galaxql::max_col(Query &q, int colno)
{
    double val;
    q.mResults[1][colno].ToDouble(&val);
    int i;
    for (i = 2; i < q.mRows; i++)
    {
        double comp;
        q.mResults[i][colno].ToDouble(&comp);
        if (comp > val)
            val = comp;
    }
    return (float)val;
}

int Galaxql::sorted_col(Query &q, int colno)
{
    double v;
    double w;
    q.mResults[1][colno].ToDouble(&v);
    q.mResults[2][colno].ToDouble(&w);
    int i = 2;
    while (i < q.mRows && v <= w)
    {
        v = w;
        i++;
        q.mResults[i][colno].ToDouble(&w);
    }
    if (i >= q.mRows)
        return 1;
    return 0;
}

int Galaxql::equals_col(Query &q, int colno, int value)
{
    int i;
    for (i = 1; i < q.mRows; i++)
    {
        long comp;
        q.mResults[i][colno].ToLong(&comp,10);
        if (comp != value)
            return 0;
    }
    return 1;
}

int Galaxql::check_pairs(Query &q)
{
    int i, j;
    for (j = 0; j < q.mColumns; j++)
    for (i = 0; i < (q.mRows-1)/2; i++)
    {
        if (q.mResults[i*2+1][j].CmpNoCase(q.mResults[i*2+2][j])!=0)
            return 0;
    }
    return 1;
}

int Galaxql::compare_queries(Query &q1, Query &q2)
{
    if (q1.mRows == 0)
        return 0;
    if (q1.mColumns != q2.mColumns)
        return -2;
    if (q1.mRows != q2.mRows)
        return -1;
    int i, j;
    for (i = 0; i < q1.mColumns; i++)
        if (q1.mResults[0][i].CmpNoCase(q2.mResults[0][i]) != 0)
            return 1;
    for (i = 1; i < q1.mRows; i++)
        for (j = 0; j < q1.mColumns; j++)
        if (!(q1.mResults[i][j] == q2.mResults[i][j] || q1.mResults[i][j].CmpNoCase(q2.mResults[i][j]) == 0))
            return 2;
    return 3;
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GURU_DONE
 */

void Galaxql::OnGuruDoneClick( wxCommandEvent& event )
{
    GalaxqlApp * app = (GalaxqlApp*)wxTheApp;  

    if (mChapterSelect->IsEnabled())
    {
        mChapterSelect->Enable(0);
        int success = 0;
        switch (mChapterSelect->GetSelection()+1)
        {
        case 1:
            mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru01ok.htm"));
            success=1;
            break;
        case 2:
            {
                long val;
                mUserQuery.mResults[1][0].ToLong(&val);
                if (val == 60*60*24)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru02ok.htm"));
                    success = 1;
                }
                else
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru02no.htm"));
                }
            }
            break;
        case 3:
            if (mUserQuery.mColumns > 3)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru03no1.htm"));
            }
            else
            if (mGfxPanel->mStars == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_nostars.htm"));
                success = -1;
            }
            else
            if (mUserQuery.mRows == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_noresults.htm"));
            }
            else
            if (mUserQuery.mResults[0][0].CmpNoCase(wxT("x")) == 0 &&
                mUserQuery.mResults[0][1].CmpNoCase(wxT("y")) == 0 &&
                mUserQuery.mResults[0][2].CmpNoCase(wxT("z")) == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru03ok.htm"));
                success = 1;
            }
            else
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_notright.htm"));
            }
            break;
        case 4:
            if (mUserQuery.mColumns != 4 ||
                !(mUserQuery.mResults[0][0].CmpNoCase(wxT("starid")) == 0 &&
                  mUserQuery.mResults[0][1].CmpNoCase(wxT("x")) == 0 &&
                  mUserQuery.mResults[0][2].CmpNoCase(wxT("y")) == 0 &&
                  mUserQuery.mResults[0][3].CmpNoCase(wxT("z")) == 0))
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_wrongcolumns.htm"));
            }
            else
            if (mGfxPanel->mStars == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_nostars.htm"));
                success = -1;
            }
            else
            if (mUserQuery.mRows == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_noresults.htm"));
            }
            else
            if (mUserQuery.mRows < 5)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_notright.htm"));
            }
            else
            if (min_col(mUserQuery, 1) < 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru04no2.htm"));
            }
            else
            if (max_col(mUserQuery, 0) > 100)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru04no3.htm"));
            }
            else
            if (!sorted_col(mUserQuery, 2))
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru04no4.htm"));
            }
            else
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru04ok.htm"));
                success = 1;
            }
            break;
        case 5:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT SUM(y/x) FROM stars",
                sql_callback,
                &mGuruQuery,
                NULL);

            if (mGfxPanel->mStars == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_nostars.htm"));
                success = -1;
            }
            else
            if (mGuruQuery.mResults[1][0].CmpNoCase(mUserQuery.mResults[1][0]) != 0)
                //(atof(mGuruQuery.mResults[1][0]) != atof(mUserQuery.mResults[1][0]))
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru05no.htm"));
            }
            else
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru05ok.htm"));
                success = 1;
            }            
            break;
        case 6:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT s.starid,s.class "
                "FROM stars AS s, hilight AS h "
                "WHERE h.starid=s.starid",
                sql_callback,
                &mGuruQuery,
                NULL);

            if (mGfxPanel->mStars == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_nostars.htm"));
                success = -1;
            }
            else
            if (mGuruQuery.mRows == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru06no1.htm")); // no rows
            }
            else
            if (!equals_col(mGuruQuery,1,7))
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru06no2.htm")); // wrong star class 
            }
            else
            if (min_col(mGuruQuery,0) < 5000 || max_col(mGuruQuery,0) > 15000)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru06no3.htm")); // wrong starids
            }
            else
            if (mGuruQuery.mRows != 6)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru06no4.htm")); // too many/few results
            }
            else
            {
                success = 1;
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru06ok.htm"));
            }
            break;
        case 7:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT * from hilight",
                sql_callback,
                &mGuruQuery,
                NULL);

            if (mGfxPanel->mStars == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_nostars.htm"));
                success = -1;
            }
            else
            if (mGuruQuery.mRows == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru06no1.htm")); // no rows
            }
            else
            if (min_col(mGuruQuery,0) < 10000 || max_col(mGuruQuery,0) > 11000)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru07no.htm")); // outofrange
            }
            else
            if (mGuruQuery.mRows < 900)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_toofewstars.htm")); // not enough stars
            }
            else
            {
                success = 1;
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru07ok.htm")); 
            }
            break;
        case 8:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT MIN(starid) FROM stars",
                sql_callback,
                &mGuruQuery,
                NULL);
            {
                long val;
                mGuruQuery.mResults[1][0].ToLong(&val);

                if (mGfxPanel->mStars < 100)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru08no1.htm")); // too few stars
                    success = -1;
                }
                else
                if (val < 10000)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru08no2.htm")); // starids less than 10k
                }
                else
                {
                    mGuruQuery.mColumns=mGuruQuery.mRows=0;
                    sqlite3_exec(
                        app->db,
                        "ROLLBACK;SELECT min(starid) from stars",
                        sql_callback,
                        &mGuruQuery,
                        NULL);
                    // refresh starmap
                    delete[] mGfxPanel->mVtx;
                    delete[] mGfxPanel->mCol;
                    mGfxPanel->mVtx = NULL;
                    mGfxPanel->mCol = NULL;

                    
                    mGuruQuery.mResults[1][0].ToLong(&val);
                    if (val > 9000)
                    {
                        mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru08no3.htm")); // rollback failed
                        success = -1;
                    }
                    else
                    {
                        mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru08ok.htm")); 
                        success=1;
                    }
                }
            }
            break;
        case 9:
            // To verify that the X and Z values were swapped, we assume that the galaxy was
            // large and flat before the user transformed it.  This means that while the X 
            // values can get large, the Z values don't.
            //
            // "changed_x" is the average absolute value of the original X values for the stars
            // which should have been moved by the user.
            //
            // "unchanged_z" is the average absolute value of the original Z values for the stars
            // that shouldn't have been moved.
            //
            // The ratio of these averages should is about 1.4 in an unmodified galaxy and about
            // 17 in one that has been modified correctly.
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT changed_x / unchanged_z FROM "
                " (SELECT AVG(ABS(z)) AS changed_x   FROM stars WHERE starid >  10000 AND starid < 15000),"
                " (SELECT AVG(ABS(z)) AS unchanged_z FROM stars WHERE starid <= 10000 OR  15000 <= starid)",
                sql_callback,
                &mGuruQuery,
                NULL);
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            {
                double val;
                mGuruQuery.mResults[1][0].ToDouble(&val);

                if (mGfxPanel->mStars < 20000)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_toofewstars.htm")); // too few stars
                    success = -1;
                }
                else
                if (val < 15 || 19 < val)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_notright.htm"));
                }
                else
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru09ok.htm"));
                    success=1;
                }
            }
            break;
        case 10:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT DISTINCT starid "
                "FROM planets, moons "
                "WHERE planets.planetid = moons.planetid "
                "AND moons.orbitdistance >= 3000 "
                "AND planets.starid >= 20000 "
                "UNION ALL "
                "SELECT * FROM hilight "
                "ORDER BY starid",
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGfxPanel->mStars < 20000)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_toofewstars.htm")); // too few stars
                success = -1;
            }
            else
            if ((mGuruQuery.mRows & 1) == 0 || !check_pairs(mGuruQuery))
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru10no.htm")); // no pairs
            }
            else
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru10ok.htm"));
                success=1;
            }
            break;            
        case 11:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT stars.starid FROM stars, planets, "
                "(SELECT MAX(orbitdistance) AS os FROM planets) "
                "WHERE stars.starid=planets.starid AND planets.orbitdistance=os "
                "UNION ALL "
                "SELECT * FROM hilight "
                "ORDER BY starid",
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGfxPanel->mStars < 20000)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_toofewstars.htm")); // too few stars
                success = -1;
            }
            else
            if ((mGuruQuery.mRows & 1) == 0 || !check_pairs(mGuruQuery))
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru11no.htm")); // no pairs
            }
            else
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru11ok.htm"));
                success=1;
            }
            break;            
        case 12:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT stars.name AS starname, "
                "((class+7)*intensity)*1000000 AS startemp, "
                "planets.name AS planetname, "
                "((class+7)*intensity)*1000000-orbitdistance*50 AS planettemp "
                "FROM stars "
                "LEFT OUTER JOIN planets "
                "ON stars.starid=planets.starid "
                "WHERE stars.starid < 100",
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGfxPanel->mStars < 20000)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_toofewstars.htm")); // too few stars
                success = -1;
            }
            else
            if (mUserQuery.mColumns != 4)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru12no1.htm")); // wrong number of columns
            }
            else
            if (mUserQuery.mRows != mGuruQuery.mRows)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru12no2.htm")); // wrong number of rows
            }
            else
            {
                int c = compare_queries(mGuruQuery, mUserQuery);
                if (c == 1)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru12no3.htm")); // wrong column names
                }
                else
                if (c == 2)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru12no4.htm")); // wrong calc?
                }
                else
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru12ok.htm"));
                    success=1;
                }
            }
            break;        
        case 13:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT * FROM numbers "
                "UNION ALL "
                "SELECT 3 AS three, intensity, x "
                "FROM stars "
                "ORDER BY x",
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGfxPanel->mStars < 20000)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_toofewstars.htm")); // too few stars
                success = -1;
            }
            else
            if (mGuruQuery.mRows == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru13no3.htm")); // no view
            }
            else
            if (mGuruQuery.mColumns > 3)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru13no1.htm")); // extra columns
            }
            else
            if (!check_pairs(mGuruQuery))
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru13no2.htm")); // something else went wrong
            }
            else
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru13ok.htm"));
                success=1;
            }
            break;        
        case 14:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT * FROM colors ORDER BY color",
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGuruQuery.mRows == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru14no1.htm")); // no table / wrong columns
            }
            else
            if (mGuruQuery.mColumns > 2)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru14no2.htm")); // extra columns
            }
            else
            if (mGuruQuery.mColumns != 2 ||
                mGuruQuery.mResults[0][0].CmpNoCase(wxT("color")) != 0 ||
                mGuruQuery.mResults[0][1].CmpNoCase(wxT("description")) != 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru14no3.htm")); // wrong columns
            }
            else
            if (mGuruQuery.mRows != 15 || 
                min_col(mGuruQuery,0) != -3 || 
                max_col(mGuruQuery,0) != 10)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru14no4.htm")); // wrong number of rows
            }
            else // TODO: smarter checks ('color' values)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru14ok.htm"));
                success=1;
            }
            break;        
        case 15:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT *,id,quote FROM quotes",
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGuruQuery.mRows == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru15no1.htm")); // no table / wrong columns
            }
            else
            if (mGuruQuery.mColumns != 4)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru15no2.htm")); // extra columns
            }
            else
            if (mGuruQuery.mResults[0][0].CmpNoCase(wxT("id")) != 0 ||
                mGuruQuery.mResults[0][1].CmpNoCase(wxT("quote")) != 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru15no3.htm")); // wrong columns
            }
            else
            {
                mGuruQuery.mColumns=mGuruQuery.mRows=0;
                sqlite3_exec(
                    app->db,
                    "DELETE FROM quotes WHERE id=7357",
                    sql_callback,
                    &mGuruQuery,
                    NULL);
                mGuruQuery.mColumns=mGuruQuery.mRows=0;
                sqlite3_exec(
                    app->db,
                    "INSERT INTO quotes (id, quote) VALUES (7357, NULL)",
                    sql_callback,
                    &mGuruQuery,
                    NULL);
                mGuruQuery.mColumns=mGuruQuery.mRows=0;
                sqlite3_exec(
                    app->db,
                    "SELECT * FROM quotes WHERE id=7357",
                    sql_callback,
                    &mGuruQuery,
                    NULL);
                if (mGuruQuery.mRows != 0)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru15no4.htm")); // could insert NULL to non-null column
                }
                else 
                {
                    mGuruQuery.mColumns=mGuruQuery.mRows=0;
                    sqlite3_exec(
                        app->db,
                        "INSERT INTO quotes (id, quote) VALUES (7357, \"test1\")",
                        sql_callback,
                        &mGuruQuery,
                        NULL);
                    mGuruQuery.mColumns=mGuruQuery.mRows=0;
                    sqlite3_exec(
                        app->db,
                        "INSERT INTO quotes (id, quote) VALUES (7357, \"test2\")",
                        sql_callback,
                        &mGuruQuery,
                        NULL);
                    mGuruQuery.mColumns=mGuruQuery.mRows=0;
                    sqlite3_exec(
                        app->db,
                        "SELECT * FROM quotes WHERE id=7357",
                        sql_callback,
                        &mGuruQuery,
                        NULL);

                    if (mGuruQuery.mRows != 2)
                    {
                        mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru15no5.htm")); // could insert to same id twice
                    }
                    else
                    {
                        mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru15ok.htm"));
                        success=1;
                    }
                }
            }
            break;        
        case 16:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT moredata FROM my_table UNION ALL SELECT COUNT(moredata) FROM my_table",
                sql_callback,
                &mGuruQuery,
                NULL);
            {
                long val;
                mGuruQuery.mResults[mGuruQuery.mRows-1][0].ToLong(&val);

                if (mGuruQuery.mRows == 0)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru16no1.htm")); // no table / wrong columns
                }
                else
                if (mGuruQuery.mRows < 5)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru16no2.htm")); // too little data
                }
                else
                if (val < mGuruQuery.mRows-4)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru16no3.htm")); // too many NULLs
                }
                else
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru16ok.htm"));
                    success=1;
                }
            }
            break;        
        case 17:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;

            // Query for the correct answer, followed by the user's answer.
            // The SELECT that comes before the UNION ALL is the correct answer.
            // It creates two relations, one for total planets by star id and one for
            // total moons by star id.  It joins these two relations using an OUTER LEFT JOIN
            // so that the table contains all stars, not just ones with non-zero moons.
            // Finally, it sums the total planets with the total moons to get the total
            // orbitals.
            //
            // The SELECT that comes after the UNION ALL is the user's answer (padded with
            // a dummy value of -1 so that the relation has the same number of columns as
            // the one which comes before the UNION ALL).
            //
            // Since the user should only put one star into the hilight table, there should
            // be three rows in the query result--the header, the correct answer,
            // and the user's answer.  If the first column of the second and third row
            // match, it means that our calculated starid matches what the user came up with.
            sqlite3_exec(
                app->db,
                "SELECT * FROM"
                "  (SELECT" // total orbitals per star
                "    starid,"
                "    total_planets+COALESCE(total_moons,0) AS orbitals FROM"
                "    (SELECT" // total planets per star
                "         starid,"
                "         COUNT(planetid) AS total_planets"
                "       FROM planets"
                "       GROUP BY starid)"
                "    LEFT OUTER JOIN"
                "      (SELECT" // total moons per star
                "          planets.starid AS planets_starid,"
                "          SUM(total_moons) AS total_moons"
                "        FROM planets,"
                "          (SELECT" // total moons per planet
                "              moons.planetid AS moons_planetid,"
                "              COUNT(moons.moonid) AS total_moons"
                "            FROM planets, moons "
                "            WHERE moons.planetid=planets.planetid"
                "            GROUP BY moons.planetid)"
                "        WHERE planets.planetid = moons_planetid"
                "        GROUP BY planets.starid)"
                "    ON starid = planets_starid"
                "    ORDER BY orbitals DESC, starid ASC"
                "    LIMIT 1)"
                "UNION ALL "
                "SELECT starid, -1 AS dummy FROM hilight", // user's answer
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGuruQuery.mRows != 3 || mGuruQuery.mResults[1][0].CmpNoCase(mGuruQuery.mResults[2][0]) != 0)
                //atoi(mGuruQuery.mResults[1][0]) != atoi(mGuruQuery.mResults[2][0]))
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru17no.htm")); // wrong star, too many answers, not enough answers.
            }
            else
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru17ok.htm"));
                success=1;
            }
            break;        
        case 18:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT starid FROM planets "
                "EXCEPT "
                "SELECT starid*2 AS starid FROM planets "
                "INTERSECT "
                "SELECT starid*3 AS starid FROM planets ",
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGfxPanel->mStars < 20000)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_toofewstars.htm")); // too few stars
                success = -1;
            }
            else
            {
                int c = compare_queries(mUserQuery, mGuruQuery);
                switch (c)
                {
                case -2:
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru18no1.htm")); // wrong number of columns
                    break;
                case -1:
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru18no2.htm")); // wrong number of rows
                    break;
                case 0:
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_noresults.htm")); // no rows
                    break;
                case 1:
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_wrongcolumns.htm")); // wrong column names
                    break;
                case 2:
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru18no3.htm")); // data mismatch
                    break;
                default:
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru18ok.htm"));
                    success=1;                
                }
            }
            break;        
        case 19:
            mGuruQuery.mColumns=mGuruQuery.mRows=0;
            sqlite3_exec(
                app->db,
                "SELECT MAX(starid) FROM stars",
                sql_callback,
                &mGuruQuery,
                NULL);
            if (mGfxPanel->mStars < 20000)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru_toofewstars.htm")); // too few stars
                success = -1;
            }
            else
            {
                long maxid;
                mGuruQuery.mResults[1][0].ToLong(&maxid);
                char temp[256];
                sprintf(temp, "INSERT INTO hilight (starid) VALUES (%ld)", maxid);
                mGuruQuery.mColumns=mGuruQuery.mRows=0;
                sqlite3_exec(
                    app->db,
                    temp,
                    sql_callback,
                    &mGuruQuery,
                    NULL);
                maxid++;
                sprintf(temp, "INSERT INTO stars (starid,x,y,z,class,intensity) VALUES (%ld,5,4,3,2,1)", maxid);
                mGuruQuery.mColumns=mGuruQuery.mRows=0;
                sqlite3_exec(
                    app->db,
                    temp,
                    sql_callback,
                    &mGuruQuery,
                    NULL);
                mGuruQuery.mColumns=mGuruQuery.mRows=0;
                sqlite3_exec(
                    app->db,
                    "SELECT starid FROM hilight",
                    sql_callback,
                    &mGuruQuery,
                    NULL);
                
                long val;
                mGuruQuery.mResults[1][0].ToLong(&val);
                if (mGuruQuery.mRows != 2 || val != maxid)
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru19no.htm")); // no view
                }
                else
                {
                    mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru19ok.htm"));
                    success=1;
                }
            }
            break;        
        case 20:
            if (mGfxPanel->mGateways == 0)
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru20no.htm")); // no gateways
            }
            else
            {
                mGuruSpeaks->LoadPage(wxT("file:galaxql.dat#zip:guru20ok.htm"));
                success=1;
            }
            break;        

        default:
            success=2;
        }
        if (success>0)
        {
            if (success == 1)
                mGuruPicture->SetBitmap(mGuruHappy);
            else
                mGuruPicture->SetBitmap(mGuruEaster);
            mChapterSelect->SetSelection(mChapterSelect->GetSelection()+1);
            mGuruDone->SetLabel(wxT("Ok!"));
        }
        else
        {
            if (success == 0)
                mGuruPicture->SetBitmap(mGuruSad);
            else
                mGuruPicture->SetBitmap(mGuruBadDb);
            mGuruDone->SetLabel(wxT("I'll try again.."));
        }
    }
    else
    {
        mGuruPicture->SetBitmap(mGuruNormal);
        mChapterSelect->Enable(1);
        mGuruDone->SetLabel(wxT("Ok, I'm done.."));
        OnChapterselectSelected(event);
    }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OPEN
 */

void Galaxql::OnLoadqueryClick( wxCommandEvent& WXUNUSED(event) )
{
    wxFileDialog fd(this, 
        wxT("Load query"), 
        wxT(""), 
        wxT("query.txt"), 
        wxT("txt files (*.txt)|*.txt"), 
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fd.ShowModal() == wxID_OK)
    {
        // Open the file for reading.
        wxFileName fileName(fd.GetDirectory(), fd.GetFilename());
        wxFFile file(fileName.GetFullPath(), wxT("r"));
        if (!file.IsOpened())
        {
            // wxWidgets will display an error message.
            return;
        }

        // Read the entire file contents.
        wxString fileContents;
        if (!file.ReadAll(&fileContents))
        {
            // wxWidgets will display an error message.
            return;
        }

        // Set the file contents into the Query box.
        mQuery->SetText(fileContents);
    }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_SAVE
 */

void Galaxql::OnSavequeryClick( wxCommandEvent& WXUNUSED(event) )
{
    wxFileDialog fd(this, 
        wxT("Save query"), 
        wxT(""), 
        wxT("query.txt"), 
        wxT("txt files (*.txt)|*.txt"), 
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (fd.ShowModal() == wxID_OK)
    {
        // open the file for writing
        wxFileName fileName(fd.GetDirectory(), fd.GetFilename());
        wxFFile file(fileName.GetFullPath(), wxT("w"));
        if (!file.IsOpened())
        {
            // wxWidgets will display an error message.
            return;
        }

        // Write the file contents.
        if (!file.Write(mQuery->GetText()))
        {
            // wxWidgets will display an error message.
            return;
        }

        // Close/flush the file stream
        if (!file.Close())
        {
            // wxWidgets will display an error message.
            return;
        }
    }
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENURUNQUERY
 */

void Galaxql::OnMenurunqueryClick( wxCommandEvent& event )
{
    OnRunqueryClick(event);
}


/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHAPTERSELECT
 */

void Galaxql::OnChapterselectSelected( wxCommandEvent& WXUNUSED(event) )
{
    int selection = mChapterSelect->GetSelection()+1;
    char temp[256];
    if (selection < 21)
        sprintf(temp,"file:galaxql.dat#zip:guru%02d.htm",selection);
    else
        sprintf(temp,"file:galaxql.dat#zip:guruend.htm",selection);

    mGuruSpeaks->LoadPage(wxString(temp,wxConvUTF8));
}


/*!
 * wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK
 */

void Galaxql::OnNotebookPageChanged( wxNotebookEvent& event )
{
    if (mQuery && event.GetSelection()==1)
        mQuery->SetFocus();
    if (mGuruSpeaks && event.GetSelection()==0)
        mGuruSpeaks->SetFocus();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUGLOW
 */

void Galaxql::OnMenuglowClick( wxCommandEvent& WXUNUSED(event) )
{
    mGfxPanel->mRenderInLowQuality = 0;
    mGfxPanel->mRenderWithGlow = 1;
    wxMenuBar *mb = GetMenuBar();
    mb->Check(ID_MENUGLOW,1);
    mb->Check(ID_MENULOWQUALITY,0);
    mb->Check(ID_NORMALRENDER,0);
}



/*!
 * wxGuruSpeaksPanel type definition
 */

IMPLEMENT_DYNAMIC_CLASS( wxGuruSpeaksPanel, wxHtmlWindow )

/*!
 * wxGuruSpeaksPanel event table definition
 */

BEGIN_EVENT_TABLE( wxGuruSpeaksPanel, wxHtmlWindow )

////@begin wxGuruSpeaksPanel event table entries
EVT_CONTEXT_MENU(wxGuruSpeaksPanel::OnContextMenu)
EVT_MENU(MENU_CONTEXT_COPY, wxGuruSpeaksPanel::OnContextMenu_Copy)
EVT_MENU(MENU_CONTEXT_SELECT_ALL, wxGuruSpeaksPanel::OnContextMenu_SelectAll)
////@end wxGuruSpeaksPanel event table entries

END_EVENT_TABLE()

/*!
 * wxGuruSpeaksPanel constructors
 */

wxGuruSpeaksPanel::wxGuruSpeaksPanel( )
{
}

wxGuruSpeaksPanel::wxGuruSpeaksPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * wxGuruSpeaksPanel creator
 */

bool wxGuruSpeaksPanel::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin wxGuruSpeaksPanel member initialisation
////@end wxGuruSpeaksPanel member initialisation

////@begin wxGuruSpeaksPanel creation
    wxHtmlWindow::Create( parent, id, pos, size, style );

    CreateControls();
////@end wxGuruSpeaksPanel creation
    return true;
}

/*!
 * Control creation for wxGuruSpeaksPanel
 */

void wxGuruSpeaksPanel::CreateControls()
{    
////@begin wxGuruSpeaksPanel content construction
    // Generated by DialogBlocks, 03/06/2006 15:22:22 (Personal Edition)

    //wxGuruSpeaksPanel* itemHtmlWindow54 = this;

////@end wxGuruSpeaksPanel content construction
}

/*!
 * Should we show tooltips?
 */

bool wxGuruSpeaksPanel::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap wxGuruSpeaksPanel::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin wxGuruSpeaksPanel bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end wxGuruSpeaksPanel bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon wxGuruSpeaksPanel::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin wxGuruSpeaksPanel icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end wxGuruSpeaksPanel icon retrieval
}

void wxGuruSpeaksPanel::OnLinkClicked(const wxHtmlLinkInfo& link)
{
    GalaxqlApp * app = (GalaxqlApp*)wxTheApp;  
    Galaxql * g = (Galaxql*)app->GetTopWindow();
    g->mQuery->SetText(link.GetHref());
    g->mQuery->SetFocus();
    
}

void wxGuruSpeaksPanel::OnContextMenu(wxContextMenuEvent &event)
{
    wxMenu menu;
    menu.Append(MENU_CONTEXT_COPY, wxT("Copy"));
    menu.Append(wxID_SEPARATOR);
    menu.Append(MENU_CONTEXT_SELECT_ALL, wxT("Select All"));
    PopupMenu(&menu, ScreenToClient(event.GetPosition()));
    //wxMessageBox(wxT("Done"));
}

void wxGuruSpeaksPanel::OnContextMenu_Copy(wxCommandEvent &event)
{
    if(wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(SelectionToText()));
        wxTheClipboard->Close();
    }
    else
    {
        wxMessageBox( "Failed to copy to the clipboard.", "GalaXQL Error", wxICON_ERROR | wxCENTER );
    }
}

void wxGuruSpeaksPanel::OnContextMenu_SelectAll(wxCommandEvent &event)
{
    SelectAll();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENULOWQUALITY
 */

void Galaxql::OnMenulowqualityClick( wxCommandEvent& WXUNUSED(event) )
{
    mGfxPanel->mRenderWithGlow = 0;
    mGfxPanel->mRenderInLowQuality = 1;
    wxMenuBar *mb = GetMenuBar();
    mb->Check(ID_MENUGLOW,0);
    mb->Check(ID_MENULOWQUALITY,1);
    mb->Check(ID_NORMALRENDER,0);
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_NORMALRENDER
 */

void Galaxql::OnNormalrenderClick( wxCommandEvent& WXUNUSED(event) )
{
    mGfxPanel->mRenderWithGlow = 0;
    mGfxPanel->mRenderInLowQuality = 0;
    wxMenuBar *mb = GetMenuBar();
    mb->Check(ID_MENUGLOW,0);
    mb->Check(ID_MENULOWQUALITY,0);
    mb->Check(ID_NORMALRENDER,1);
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_RENDERGRID
 */

void Galaxql::OnRendergridClick( wxCommandEvent& WXUNUSED(event) )
{
    mGfxPanel->mRenderGrid = !mGfxPanel->mRenderGrid;
    wxMenuBar *mb = GetMenuBar();
    mb->Check(ID_RENDERGRID, mGfxPanel->mRenderGrid != 0);
}
