/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2006, 2007 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef TextRun_h
#define TextRun_h

#include "PlatformString.h"

#include <fribidi.h>
#include <fribidi-char-sets.h>

namespace WebCore {

class RenderObject;
class SVGPaintServer;

class TextRun {
public:
    TextRun(const UChar* c, int len, bool allowTabs = false, int xpos = 0, int padding = 0, bool rtl = false, bool directionalOverride = false,
              bool applyRunRounding = true, bool applyWordRounding = true)
        : m_characters(c)
        , m_shaped_characters(NULL)
        , m_len(len)
        , m_xpos(xpos)
        , m_padding(padding)
        , m_allowTabs(allowTabs)
        , m_rtl(rtl)
        , m_directionalOverride(directionalOverride)
        , m_applyRunRounding(applyRunRounding)
        , m_applyWordRounding(applyWordRounding)
        , m_disableSpacing(false)
#if ENABLE(SVG_FONTS)
        , m_referencingRenderObject(0)
        , m_activePaintServer(0)
#endif
    {
        doShapeCharacters();
    }

    TextRun(const String& s, bool allowTabs = false, int xpos = 0, int padding = 0, bool rtl = false, bool directionalOverride = false,
              bool applyRunRounding = true, bool applyWordRounding = true)
        : m_characters(s.characters())
        , m_shaped_characters(NULL)
        , m_len(s.length())
        , m_xpos(xpos)
        , m_padding(padding)
        , m_allowTabs(allowTabs)
        , m_rtl(rtl)
        , m_directionalOverride(directionalOverride)
        , m_applyRunRounding(applyRunRounding)
        , m_applyWordRounding(applyWordRounding)
        , m_disableSpacing(false)
#if ENABLE(SVG_FONTS)
        , m_referencingRenderObject(0)
        , m_activePaintServer(0)
#endif
    {
        doShapeCharacters();
    }

    UChar operator[](int i) const { return m_characters[i]; }
    const UChar* data(int i) const { return &m_characters[i]; }

    const UChar* characters() const { return m_characters; }
    int length() const { return m_len; }

    void setText(const UChar* c, int len) { 
        m_characters = c; m_len = len; 
        doShapeCharacters();
    }

    bool allowTabs() const { return m_allowTabs; }
    int xPos() const { return m_xpos; }
    int padding() const { return m_padding; }
    bool rtl() const { return m_rtl; }
    bool ltr() const { return !m_rtl; }
    bool directionalOverride() const { return m_directionalOverride; }
    bool applyRunRounding() const { return m_applyRunRounding; }
    bool applyWordRounding() const { return m_applyWordRounding; }
    bool spacingDisabled() const { return m_disableSpacing; }

    void disableSpacing() { m_disableSpacing = true; }
    void disableRoundingHacks() { m_applyRunRounding = m_applyWordRounding = false; }
    void setRTL(bool b) { m_rtl = b; }
    void setDirectionalOverride(bool override) { m_directionalOverride = override; }

#if ENABLE(SVG_FONTS)
    RenderObject* referencingRenderObject() const { return m_referencingRenderObject; }
    void setReferencingRenderObject(RenderObject* object) { m_referencingRenderObject = object; }

    SVGPaintServer* activePaintServer() const { return m_activePaintServer; }
    void setActivePaintServer(SVGPaintServer* object) { m_activePaintServer = object; }
#endif

    void doShapeCharacters() {
        FriBidiCharType *btypes;
        FriBidiLevel *embedding_levels;
        FriBidiJoiningType *jtypes;
        FriBidiParType pbase;
        FriBidiChar *fribidi_characters;
        int32_t len;

        if (m_rtl)
            pbase = FRIBIDI_PAR_RTL;
        else
            pbase = FRIBIDI_PAR_LTR;

        len = (m_len + 1) << 2;
        fribidi_characters = (FriBidiChar *)malloc(len);
        UErrorCode pErrorCode = U_ZERO_ERROR;
        u_strToUTF32((UChar32 *)fribidi_characters, len, &len, m_characters, m_len, &pErrorCode);

        btypes = (FriBidiCharType *)malloc(sizeof(FriBidiCharType) * m_len);
        embedding_levels = (FriBidiLevel *)malloc(sizeof(FriBidiLevel) * m_len);
        jtypes = (FriBidiJoiningType *)malloc(sizeof(FriBidiJoiningType) * m_len);

        fribidi_get_bidi_types(fribidi_characters, m_len, btypes);
        fribidi_get_par_embedding_levels(btypes, m_len, &pbase, embedding_levels);

        fribidi_get_joining_types((FriBidiChar *) fribidi_characters, m_len, jtypes);
        fribidi_join_arabic(btypes, m_len, embedding_levels, jtypes);
        fribidi_shape(FRIBIDI_FLAGS_DEFAULT | FRIBIDI_FLAGS_ARABIC, embedding_levels, m_len, jtypes, fribidi_characters);

        fribidi_reorder_line(FRIBIDI_FLAG_SHAPE_MIRRORING | FRIBIDI_FLAG_REMOVE_SPECIALS, \
                             btypes, m_len, 0, pbase, embedding_levels, fribidi_characters, NULL);

        m_shaped_characters = (UChar *)malloc(sizeof(UChar) * (m_len+1));
        u_strFromUTF32(m_shaped_characters, sizeof(UChar) * (m_len+1), NULL, (UChar32 *)fribidi_characters, len, &pErrorCode);
        m_shaped_characters[m_len] = '\0';

        free(jtypes);
        free(btypes);
        free(embedding_levels);
        free(fribidi_characters);
    }


    const UChar* shapedCharacters() const { return m_shaped_characters; }



private:
    const UChar* m_characters;
    UChar* m_shaped_characters;
    int m_len;

    int m_xpos;
    int m_padding;
    bool m_allowTabs;
    bool m_rtl;
    bool m_directionalOverride;
    bool m_applyRunRounding;
    bool m_applyWordRounding;
    bool m_disableSpacing;

#if ENABLE(SVG_FONTS)
    RenderObject* m_referencingRenderObject;
    SVGPaintServer* m_activePaintServer;
#endif
};

}

#endif
