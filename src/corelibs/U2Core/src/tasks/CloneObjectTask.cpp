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

#include <QtCore/QCoreApplication>

#include <U2Core/DocumentModel.h>
#include <U2Core/U2DbiUtils.h>
#include <U2Core/U2ObjectDbi.h>
#include <U2Core/U2SafePoints.h>

#include "CloneObjectTask.h"

namespace U2 {

CloneObjectTask::CloneObjectTask(GObject *srcObj, Document *dstDoc, const QString &dstFolder)
: Task(tr("Copy object"), TaskFlag_None), srcObj(srcObj), dstDoc(dstDoc), dstFolder(dstFolder), dstObj(NULL)
{
    CHECK_EXT(NULL != srcObj, setError(tr("Invalid source object")), );
    CHECK_EXT(NULL != dstDoc, setError(tr("Invalid destination document")), );
    setTaskName(getTaskName() + ": " + srcObj->getGObjectName());
}

CloneObjectTask::~CloneObjectTask() {
    delete dstObj;
}

void CloneObjectTask::run() {
    CHECK_EXT(!dstDoc.isNull(), setError(tr("The document has been removed")), );
    U2DbiRef dstRef = dstDoc->getDbiRef();

    DbiOperationsBlock opBlock(dstRef, stateInfo);
    Q_UNUSED(opBlock);
    CHECK_OP(stateInfo, );

    DbiConnection con(dstRef, stateInfo);
    CHECK_OP(stateInfo, );
    SAFE_POINT_EXT(NULL != con.dbi, setError(QObject::tr("Error! No DBI")), );
    U2ObjectDbi *oDbi = con.dbi->getObjectDbi();
    SAFE_POINT_EXT(NULL != oDbi, setError(QObject::tr("Error! No object DBI")), );

    CHECK_EXT(!srcObj.isNull(), setError(tr("The object has been removed")), );
    dstObj = srcObj->clone(dstDoc->getDbiRef(), stateInfo);
    CHECK_OP(stateInfo, );
    dstObj->moveToThread(QCoreApplication::instance()->thread());

    QStringList folders = oDbi->getFolders(stateInfo);
    CHECK_OP(stateInfo, );

    if (!folders.contains(dstFolder)) {
        oDbi->createFolder(dstFolder, stateInfo);
        CHECK_OP(stateInfo, );
    }

    QList<U2DataId> objList;
    objList << dstObj->getEntityRef().entityId;
    oDbi->moveObjects(objList, U2ObjectDbi::ROOT_FOLDER, dstFolder, stateInfo);
    CHECK_OP(stateInfo, );
}

GObject * CloneObjectTask::takeResult() {
    GObject *result = dstObj;
    dstObj = NULL;
    return result;
}

const QString & CloneObjectTask::getFolder() const {
    return dstFolder;
}

Document * CloneObjectTask::getDocument() const {
    return dstDoc.data();
}

} // U2