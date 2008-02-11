/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-30-08
 * Description : a progress dialog for digiKam
 * 
 * Copyright (C) 2006-2008 by Gilles Caulier <caulier dot gilles at gmail dot com>
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

#ifndef DPROGRESSDLG_H
#define DPROGRESSDLG_H

// Qt includes.

#include <QPixmap>

// KDE includes.

#include <kdialog.h>

// Local includes.

#include "digikam_export.h"

class QProgressBar;

namespace Digikam
{

class DProgressDlgPriv;

class DIGIKAM_EXPORT DProgressDlg : public KDialog
{
Q_OBJECT

 public:

    DProgressDlg(QWidget *parent=0, const QString &caption=QString());
    ~DProgressDlg();

    void setLabel(const QString &text);
    void setTitle(const QString &text);
    void setActionListVSBarVisible(bool visible);
    void showCancelButton(bool show);
    void setAllowCancel(bool allowCancel);
    bool wasCancelled() const;
    bool allowCancel() const;

    int  value();

 public slots:

    void setMaximum(int max);
    void incrementMaximum(int added);
    void advance(int offset);
    void setValue(int value);

    void setButtonText(const QString &text);
    void addedAction(const QPixmap& pix, const QString &text);
    void reset();

protected slots:

    void slotCancel();

 private:

    DProgressDlgPriv* d;
};

}  // NameSpace Digikam

#endif  // DPROGRESSDLG_H
