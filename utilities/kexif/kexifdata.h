#ifndef KEXIFDATA_H
#define KEXIFDATA_H

extern "C" {
#include <libexif/exif-data.h>
}

#include <qstring.h>
#include <qimage.h>
#include <qvaluevector.h>

class KExifIfd;

class KExifData {

public:

    enum {
        NOEXIF=0,
        NOTHUMBNAIL,
        ERROR,
        SUCCESS };

    enum ImageOrientation {
        NORMAL=1, 
        HFLIP=2, 
        ROT_180=3, 
        VFlip=4, 
        ROT_90_HFLIP=5, 
        ROT_90=6, 
        ROT_90_VFLIP=7, 
        ROT_270=8
    };

    KExifData();
    ~KExifData();

    int readFromFile(const QString& filename);
    int readFromData(char* data, int size);
    int getThumbnail(QImage& thumb);
    QString getUserComment();
    ImageOrientation getImageOrientation();

    QValueVector<KExifIfd> ifdVector;

    void saveFile(const QString& filename);
    void KExifData::saveExifComment(QString& filename, QString& comment);

private:

    ExifData *mExifData;
    QString   mExifByteOrder;
    QString   mUserComment;
    QImage mThumbnail;

};

#endif
