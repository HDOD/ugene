/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2015 UniPro <ugene@unipro.ru>
 * http://ugene.unipro.ru
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _HI_GT_COMBOBOX_H_
#define _HI_GT_COMBOBOX_H_

#include "GTGlobals.h"
#if (QT_VERSION < 0x050000) //Qt 5
#include <QtGui/QComboBox>
#else
#include <QtWidgets/QComboBox>
#endif

namespace HI {

class HI_EXPORT GTComboBox {
public:
    // fails if the comboBox is NULL, index is not in a comboBox range
    // or a comboBox's index differs from a given index in the end of method's execution
    static void setCurrentIndex(U2::U2OpStatus& os, QComboBox *comboBox, int index, bool checkVal = true, GTGlobals::UseMethod method = GTGlobals::UseKey);

    // Fails if the comboBox is NULL, combobox doesn't contain an item with text
    // or a comboBox current item's text differs from a given text in the end of method's execution
    static void setIndexWithText(U2::U2OpStatus& os, QComboBox *comboBox, const QString& text, bool checkVal = true, GTGlobals::UseMethod method = GTGlobals::UseKey);

    static QStringList getValues(U2::U2OpStatus &os, QComboBox *comboBox);
    static void checkValues(U2::U2OpStatus& os, QComboBox *comboBox, const QStringList &values);

    static void checkValuesPresence(U2::U2OpStatus& os, QComboBox *comboBox, const QStringList &values);
    static void checkCurrentValue(U2::U2OpStatus &os, QComboBox *comboBox, const QString &expectedValue);
};

}
#endif // _U2_GT_COMBOBOX_H_