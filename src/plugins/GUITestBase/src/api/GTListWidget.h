/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2014 UniPro <ugene@unipro.ru>
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


#ifndef _U2_GT_LISTWIDGET_H_
#define _U2_GT_LISTWIDGET_H_

#include "api/GTGlobals.h"

class QListWidget;

namespace U2{

class GTListWidget
{
public:
    static void click(U2OpStatus &os, QListWidget *listWidget, const QString &text);
    static bool isItemChecked(U2OpStatus &os, QListWidget *listWidget, const QString &text);
    static void checkItem(U2OpStatus &os, QListWidget *listWidget, const QString &text, bool newState);
    static void checkAllItems(U2OpStatus &os, QListWidget *listWidget, bool newState);
    static void checkOnlyItems(U2OpStatus &os, QListWidget *listWidget, const QStringList &itemTexts, bool newState = true);
    static QStringList getItems(U2OpStatus &os, QListWidget *listWidget);
};

}

#endif // GTLISTWIDGET_H
