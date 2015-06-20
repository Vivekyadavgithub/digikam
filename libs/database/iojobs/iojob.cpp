/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2015-06-15
 * Description : IO Jobs for file systems jobs
 *
 * Copyright (C) 2015 by Mohamed Anwer <m dot anwer at gmx dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "iojob.h"

// Qt includes

#include <QFile>
#include <QDir>

// KDE includes

#include "klocalizedstring.h"

// Local includes

#include "digikam_debug.h"
#include "digikam_export.h"
#include "albummanager.h"
#include "albumdb.h"
#include "album.h"
#include "framework/databaseaccess.h"
#include "databaseparameters.h"
#include "item/imageinfo.h"

namespace Digikam
{

IOJob::IOJob()
{
}

// --------------------------------------------

CopyJob::CopyJob(const PAlbum *dest, CopyJob::OperationType type)
{
    m_dest = dest;
    m_type = type;
}

// --------------------------------------------

CopyFileJob::CopyFileJob(const QUrl &src, const PAlbum *dest, CopyJob::OperationType type)
    : CopyJob(dest, type)
{
    m_srcFile = src;
}

void CopyFileJob::run()
{
    // TODO
    emit signalDone();
}

// --------------------------------------------

CopyAlbumJob::CopyAlbumJob(const PAlbum *src, const PAlbum *dest, CopyJob::OperationType type)
    : CopyJob(dest, type)
{
    m_srcAlbum = src;
}

void CopyAlbumJob::run()
{
    // TODO
    emit signalDone();
}

// --------------------------------------------

DeleteJob::DeleteJob(bool useTrash)
{
    m_useTrash = useTrash;
}

// --------------------------------------------

DeleteFileJob::DeleteFileJob(const ImageInfo &srcToDelete, bool useTrash)
    : DeleteJob(useTrash)
{
    m_srcToDelete = srcToDelete;
}

void DeleteFileJob::run()
{
    qCDebug(DIGIKAM_IOJOB_LOG) << "DELETING: " << m_srcToDelete.fileUrl();

    PAlbum *album = AlbumManager::instance()->findPAlbum(m_srcToDelete.albumId());

    if (!album)
    {
        error(i18n("Source album %1 not found in database", m_srcToDelete.fileUrl().adjusted(QUrl::RemoveFilename).path()));
        emit signalDone();
        return;
    }

    if(m_useTrash)
    {
        // TODO
    }
    else
    {

    }

    emit signalDone();
}

// --------------------------------------------

DeleteAlbumJob::DeleteAlbumJob(const PAlbum *album, bool useTrash)
    : DeleteJob(useTrash)
{
    m_albumToDelete = album;
}

void DeleteAlbumJob::run()
{
    qCDebug(DIGIKAM_IOJOB_LOG) << "DELETING PALBUM: " << m_albumToDelete->albumPath();

    PAlbum *album = AlbumManager::instance()->findPAlbum(m_albumToDelete->id());

    if (!album)
    {
        emit error(i18n("Source album %1 not found in database", m_albumToDelete->albumPath()));
        emit signalDone();
        return;
    }

    QDir albumFolder(m_albumToDelete->albumPath());

    if(!albumFolder.exists())
    {
        emit error(i18n("Folder %1 does not exist anymore", m_albumToDelete->title()));
        emit signalDone();
        return;
    }

    if(!m_useTrash)
    {
        // TODO: Trash Implementation
    }
    else
    {
        if(!albumFolder.removeRecursively())
        {
            emit error(i18n("Album %1 could not be deleted", m_albumToDelete->title()));
        }
    }

    emit signalDone();
}

} // namespace Digikam
