/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CEGUI_FORMATTED_LIST_BOX_H_
#define CEGUI_FORMATTED_LIST_BOX_H_

//Code adapted from http://www.cegui.org.uk/phpBB2/viewtopic.php?f=10&t=4322
#include "CEGUI.h"

namespace CEGUI {
///! A ListboxItem based class that can do horizontal text formatiing.
class FormattedListboxTextItem : public ListboxTextItem {
public:
    ///! Constructor
    FormattedListboxTextItem(const String& text,
                    const HorizontalTextFormatting format = HTF_LEFT_ALIGNED,
                    const uint item_id = 0,
                    void* const item_data = 0,
                    const bool disabled = false,
                    const bool auto_delete = true);

    ///! Destructor.
    ~FormattedListboxTextItem();

    ///! Return the current formatting set.
    HorizontalTextFormatting getFormatting() const;
    /**!
        Set the formatting.  You should call Listbox::handleUpdatedItemData
        after setting the formatting in order to update the listbox.  We do not
        do it automatically since you may wish to batch changes to multiple
        items and multiple calls to handleUpdatedItemData is wasteful.
    */
    void setFormatting(const HorizontalTextFormatting fmt);

    // overriden functions.
    Size getPixelSize(void) const;
    void draw(GeometryBuffer& buffer, const Rect& targetRect,
                float alpha, const Rect* clipper) const;

protected:
    ///! Helper to create a FormattedRenderedString of an appropriate type.
    void setupStringFormatter() const;
    ///! Current formatting set
    HorizontalTextFormatting d_formatting;
    ///! Class that renders RenderedString with some formatting.
    mutable FormattedRenderedString* d_formattedRenderedString;
    ///! Tracks target area for rendering so we can reformat when needed
    mutable Size d_formattingAreaSize;
};

}

#endif