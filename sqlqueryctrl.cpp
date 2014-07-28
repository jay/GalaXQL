/////////////////////////////////////////////////////////////////////////////
// Name:        sqlqueryctrl.cpp
// Author:      David Costanzo
// Licence:     
//
// Copyright (c) 2013, David Costanzo
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

#include "sqlqueryctrl.h"

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

const int INVALID_POSITION = -1;

const int ID_FINDMATCHINGPAREN   = 11000;
const int ID_SELECTMATCHINGPAREN = 11001;

SqlQueryCtrl::SqlQueryCtrl(
    wxWindow *      parent,
    wxWindowID      id
    ) : 
    wxStyledTextCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN)
{
    // Enable word-wrapping, instead of horizontal scrolling.
    SetWrapMode(wxSTC_WRAP_WORD);

    // Hide the margin that Scintilla creates by default.
    // We don't use it for anything, so it just looks weird.
    SetMarginWidth(1, 0);

    // Configure the keyboard shortcuts
    wxAcceleratorEntry acceleratorEntries[2];

    // Ctrl+m moves to matching paren
    acceleratorEntries[0].Set(
        wxACCEL_CTRL,
        'm',
        ID_FINDMATCHINGPAREN);

    // Ctrl+Shift+m selects to matching paren
    acceleratorEntries[1].Set(
        wxACCEL_CTRL | wxACCEL_SHIFT,
        'm',
        ID_SELECTMATCHINGPAREN);

    wxAcceleratorTable acceleratorTable(
        COUNT_OF(acceleratorEntries),
        acceleratorEntries);
    SetAcceleratorTable(acceleratorTable);

    // Switch to the SQL style
    SetLexer(wxSTC_LEX_SQL);

    SetKeyWords(
        0, // keywords for wxSTC_SQL_WORD
        wxT("abort action add after all alter analyze and as asc attach ")
        wxT("autoincrement before begin between by cascade case cast check ")
        wxT("collate column commit conflict constraint create cross ")
        wxT("current_date current_time current_timestamp database default ")
        wxT("deferrable deferred delete desc detach distinct drop each else ")
        wxT("end escape except exclusive exists explain fail for foreign ")
        wxT("from full glob group having if ignore immediate in index ")
        wxT("indexed initially inner insert instead intersect into is ")
        wxT("isnull join key left like limit match natural no not notnull ")
        wxT("null of offset on or order outer plan pragma primary query ")
        wxT("raise references regexp reindex release rename replace restrict ")
        wxT("right rollback row savepoint select set table temp temporary ")
        wxT("then to transaction trigger union unique update using vacuum ")
        wxT("values view virtual when where"));

    SetKeyWords(
        4, // keywords for wxSTC_SQL_USER1
        wxT("ltrim rtrim trim min max typeof length instr substr unicode char ")
        wxT("abs sum total avg count group_concat glob like ")
        wxT("upper lower coalesce hex ifnull random randomblob nullif ")
        wxT("sqlite_version sqlite_source_id sqlite_log ")
        wxT("quote last_insert_rowid changes total_changes replace ")
        wxT("zeroblob ")
#ifndef SQLITE_OMIT_FLOATING_POINT
        wxT("round ")
#endif
#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
        wxT("sqlite_compileoption_used sqlite_compileoption_get ")
#endif
#ifdef SQLITE_SOUNDEX
        wxT("soundex ")
#endif
#ifndef SQLITE_OMIT_LOAD_EXTENSION
        wxT("load_extension ")
        wxT("unload_extension ")
#endif
        );

    const wxColor purple       (0x80,    0, 0x80);
    const wxColor blue         (0,       0, 0xFF);
    const wxColor darkgreen    (0,    0x80, 0);
    const wxColor darkred      (0x80,    0, 0);
    const wxColor red          (0xFF,    0, 0);
    const wxColor lightgrey    (0xCC, 0xCC, 0xCC);
    const wxColor lightblue    (200,   242, 255);

    // This is the list of available styles in wxSTC_LEX_SQL
    //  wxSTC_SQL_DEFAULT 
    //  wxSTC_SQL_COMMENT
    //  wxSTC_SQL_COMMENTLINE 
    //  wxSTC_SQL_COMMENTDOC 
    //  wxSTC_SQL_NUMBER 
    //  wxSTC_SQL_WORD
    //  wxSTC_SQL_STRING
    //  wxSTC_SQL_CHARACTER
    //  wxSTC_SQL_SQLPLUS
    //  wxSTC_SQL_SQLPLUS_PROMPT
    //  wxSTC_SQL_OPERATOR 
    //  wxSTC_SQL_IDENTIFIER 
    //  wxSTC_SQL_SQLPLUS_COMMENT 
    //  wxSTC_SQL_COMMENTLINEDOC 
    //  wxSTC_SQL_WORD2
    //  wxSTC_SQL_COMMENTDOCKEYWORD
    //  wxSTC_SQL_COMMENTDOCKEYWORDERROR 
    //  wxSTC_SQL_USER1 
    //  wxSTC_SQL_USER2
    //  wxSTC_SQL_USER3
    //  wxSTC_SQL_USER4
    //  wxSTC_SQL_QUOTEDIDENTIFIER

    StyleSetForeground(wxSTC_SQL_WORD,       blue);
    StyleSetForeground(wxSTC_SQL_USER1,      purple);
    StyleSetForeground(wxSTC_SQL_OPERATOR,   purple);

    StyleSetForeground(wxSTC_SQL_COMMENT,     darkgreen);
    StyleSetForeground(wxSTC_SQL_COMMENTLINE, darkgreen);
    StyleSetForeground(wxSTC_SQL_COMMENTDOC,  darkgreen);

    StyleSetForeground(wxSTC_SQL_STRING,      darkred);
    StyleSetForeground(wxSTC_SQL_CHARACTER,   darkred);

    StyleSetForeground(wxSTC_STYLE_BRACELIGHT,  darkgreen);
    StyleSetBackground(wxSTC_STYLE_BRACELIGHT,  lightblue);

    StyleSetForeground(wxSTC_STYLE_BRACEBAD,    red);
    StyleSetBackground(wxSTC_STYLE_BRACEBAD,    lightblue);
}

static bool IsParen(int ch)
{
    return ch == '(' || ch == ')';
}

void
SqlQueryCtrl::FindMatchingParen(
    int & currentParenPosition,
    int & matchingParenPosition
    )
{
    currentParenPosition  = INVALID_POSITION;
    matchingParenPosition = INVALID_POSITION;

    int currentPosition = GetCurrentPos();
    int currentChar     = GetCharAt(currentPosition);
    if (IsParen(currentChar))
    {
        // we're close enough to a paren to try to match it
        int matchingPosition = BraceMatch(currentPosition);
        if (matchingPosition != INVALID_POSITION)
        {
            // found a match

            // If the paren after the caret is an open paren, then
            // the matching paren will be after it.  In this case, 
            // we want to jump just after the matching paren's position
            // to remain on the outside of the parens.
            // Likewise, if the paren after the caret is a close paren,
            // then the matching paren will be before it.  In this case,
            // we also want to jump after the matching paren's position
            // so that we can remain on the inside of the parens.
            currentParenPosition  = currentPosition;
            matchingParenPosition = matchingPosition + 1;
        }
        else
        {
            // If paren after the caret has no matching paren,
            // then mark it as a bad paren, instead of trying to
            // match the one before the caret.
            currentParenPosition  = currentPosition;
            matchingParenPosition = INVALID_POSITION;
        }
    }
    else
    {
        // we're not over a paren, so try the position just before the caret
        if (currentPosition != 0)
        {
            int previousChar = GetCharAt(currentPosition - 1);
            if (IsParen(previousChar))
            {
                // we're close enough to a paren to try to match it
                int matchingPosition = BraceMatch(currentPosition - 1);
                if (matchingPosition != INVALID_POSITION)
                {
                    // found a match

                    // If the paren before the caret is an open paren, then
                    // the matching paren will be after it.  In this case, 
                    // we want to jump to the matching paren's position to
                    // remain on the inside of the parens.
                    // Likewise, if the paren before the caret is a close paren,
                    // then the matching paren will be before it.  In this case,
                    // we also want to jump after the matching paren's position
                    // so that we can remain on the outside of the parens.
                    currentParenPosition  = currentPosition - 1;
                    matchingParenPosition = matchingPosition;
                }
                else
                {
                    currentParenPosition  = currentPosition - 1;
                    matchingParenPosition = INVALID_POSITION;
                }
            }
        }
    }
}

// Moves the caret to the matching paren if the caret is
// currently adject to a paren.
void SqlQueryCtrl::FindMatchingParen()
{
    int currentParenPosition;
    int matchingParenPosition;
    FindMatchingParen(currentParenPosition, matchingParenPosition);
    if (matchingParenPosition != INVALID_POSITION)
    {
        GotoPos(matchingParenPosition);
    }
}

// Moves the caret to the matching paren, if the caret is currently
// adject to a paren.  Also selects the entire text within the parens.
void SqlQueryCtrl::SelectMatchingParen()
{
    int currentParenPosition;
    int matchingParenPosition;
    FindMatchingParen(currentParenPosition, matchingParenPosition);
    if (matchingParenPosition != INVALID_POSITION && currentParenPosition != INVALID_POSITION)
    {
        // Found a match.  Jump to it.
        SetAnchor(GetCurrentPos());
        SetCurrentPos(matchingParenPosition);
        ScrollCaret();
    }
}


// Sends SCI_SCROLLCARET because wxStyledTextCtrl doesn't export this wrapper.
void SqlQueryCtrl::ScrollCaret()
{
    const int SCI_SCROLLCARET = 2169;
    SendMsg(SCI_SCROLLCARET);
}

void SqlQueryCtrl::OnUpdateUi(wxStyledTextEvent& WXUNUSED(event))
{
    int currentParenPosition;
    int matchingParenPosition;
    FindMatchingParen(currentParenPosition, matchingParenPosition);
    if (currentParenPosition != INVALID_POSITION)
    {
        // We're close enough to a paren to try to match it
        if (matchingParenPosition != INVALID_POSITION)
        {
            // found a match
            if (currentParenPosition == GetCurrentPos())
            {
                BraceHighlight(currentParenPosition, matchingParenPosition - 1);
            }
            else
            {
                BraceHighlight(currentParenPosition, matchingParenPosition);
            }
        }
        else
        {
            // didn't find a match
            BraceBadLight(currentParenPosition);
        }
    }
    else
    {
        // We're not adacent to a paren, so remove the paren highlighting.
        BraceBadLight(INVALID_POSITION);
        BraceHighlight(INVALID_POSITION, INVALID_POSITION);
    }
}

void SqlQueryCtrl::OnFindMatchingParen(wxCommandEvent& WXUNUSED(event))
{
    FindMatchingParen();
}

void SqlQueryCtrl::OnSelectMatchingParen(wxCommandEvent& WXUNUSED(event))
{
    SelectMatchingParen();
}

void SqlQueryCtrl::OnKeyDown(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();

    switch (keyCode)
    {
    case WXK_TAB:
        // Use Tab and Shift+Tab as a navigational key events
        if (event.ShiftDown())
        {
            // Shift+Tab means navigate backward.
            Navigate(wxNavigationKeyEvent::IsBackward);
        }
        else
        {
            // Tab means navigate forward.
            Navigate(wxNavigationKeyEvent::IsForward);
        }
        break;

    default:
        // Use the default behavior for every other keystroke
        event.Skip();
        break;
    }
}

void SqlQueryCtrl::OnMouseWheel(wxMouseEvent& event)
{
    if (!event.ControlDown())
    {
        // Use the default behavior for every mouse wheel event
        // when the Ctrl key isn't pressed.  This blocks zooming.
        // While zooming is a nice feature, GalaXQL doesn't have
        // enough UI controls to let the user discover how to undo
        // the zoom if they did it accidently (for example, if their
        // finger got too close to the right side of a track pad on a
        // laptop).
        event.Skip();
    }
}

BEGIN_EVENT_TABLE(SqlQueryCtrl, wxStyledTextCtrl)
    EVT_STC_UPDATEUI(wxID_ANY,       SqlQueryCtrl::OnUpdateUi)
    EVT_MENU(ID_FINDMATCHINGPAREN,   SqlQueryCtrl::OnFindMatchingParen)
    EVT_MENU(ID_SELECTMATCHINGPAREN, SqlQueryCtrl::OnSelectMatchingParen)
    EVT_KEY_DOWN(SqlQueryCtrl::OnKeyDown)
    EVT_MOUSEWHEEL(SqlQueryCtrl::OnMouseWheel)
END_EVENT_TABLE()
