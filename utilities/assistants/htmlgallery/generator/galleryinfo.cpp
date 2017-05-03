/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-04-04
 * Description : a tool to generate HTML image galleries
 *
 * Copyright (C) 2006-2010 by Aurelien Gateau <aurelien dot gateau at free dot fr>
 * Copyright (C) 2012-2017 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "galleryinfo.h"

// KDE includes

#include <kconfigbase.h>

// Local includes

#include "album.h"

namespace Digikam
{

static const char* THEME_GROUP_PREFIX = "Theme ";

GalleryInfo::GalleryInfo()
{
}

GalleryInfo::~GalleryInfo()
{
}

QString GalleryInfo::fullFormatString() const
{
    return getEnumString(QLatin1String("fullFormat"));
}

QString GalleryInfo::thumbnailFormatString() const
{
    return getEnumString(QLatin1String("thumbnailFormat"));
}

QString GalleryInfo::getThemeParameterValue(const QString& theme,
                                            const QString& parameter,
                                            const QString& defaultValue) const
{
    QString groupName  = QLatin1String(THEME_GROUP_PREFIX) + theme;
    KConfigGroup group = config()->group(groupName);

    return group.readEntry(parameter, defaultValue);
}

void GalleryInfo::setThemeParameterValue(const QString& theme,
                                         const QString& parameter,
                                         const QString& value)
{
    // FIXME: This is hackish, but config() is const :'(
    KConfig* const localConfig = const_cast<KConfig*>(config());
    QString groupName          = QLatin1String(THEME_GROUP_PREFIX) + theme;
    KConfigGroup group         = localConfig->group(groupName);
    group.writeEntry(parameter, value);
}

QString GalleryInfo::getEnumString(const QString& itemName) const
{
    // findItem is not marked const :-(
    GalleryInfo* const that               = const_cast<GalleryInfo*>(this);
    KConfigSkeletonItem* const tmp        = that->findItem(itemName);
    KConfigSkeleton::ItemEnum* const item = dynamic_cast<KConfigSkeleton::ItemEnum*>(tmp);

    Q_ASSERT(item);

    if (!item)
        return QString();

    int value                                                   = item->value();
    QList<KConfigSkeleton::ItemEnum::Choice> lst                = item->choices();
    QList<KConfigSkeleton::ItemEnum::Choice>::ConstIterator it  = lst.constBegin();
    QList<KConfigSkeleton::ItemEnum::Choice>::ConstIterator end = lst.constEnd();

    for (int pos = 0 ; it != end ; ++it, pos++)
    {
        if (pos == value)
        {
            return (*it).name;
        }
    }

    return QString();
}

} // namespace Digikam