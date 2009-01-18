/***************************************************************************
 *   Copyright (C) 2003-2008 Mark Kretschmann <kretschmann@kde.org>        *
 *             (C) 2008 Dan Meltzer <hydrogen@notyetimplemented.com        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef COLLECTIONSCANNER_H
#define COLLECTIONSCANNER_H

#include "metadata/tfile_helper.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <QDBusInterface>
#include <QHash>
#include <QStringList>

#include <KApplication>
#include <KStandardDirs>

//Taglib includes..
#include <audioproperties.h>
#include <fileref.h>

typedef QHash<QString, QString> AttributeHash;
namespace TagLib
{
    namespace MPEG
    {
        class File;
    }
}

/**
 * @class CollectionScanner
 * @short Scans directories and builds the Collection
 */

class CollectionScanner : public KApplication
{
    Q_OBJECT

public:
    CollectionScanner( const QStringList& folders,
                       const QString &amarokPid,
                       const QString &collectionId,
                       bool recursive,
                       bool incremental,
                       bool importPlaylists,
                       bool restart );

    ~CollectionScanner();
    int newInstance() { return 0; }

private slots:
    void doJob();

private:
    enum FileType
    {
        mp3,
        ogg,
        flac,
        mp4
    };

    inline QString saveLocation() const { return KGlobal::dirs()->saveLocation( "data", QString("amarok/"), true ); }

    void readDir( const QString& dir, QStringList& entries );
    void scanFiles( const QStringList& entries );
    
    /**
     * Get the render() of a tag from TagLib, if possible, to assist in AFT UID generation.
     * @fileref TagLib reference to the file.
     * @return TagLib::ByteVector that contains the tag's render, if possible, and empty otherwise.
     */
    static const QString readEmbeddedUniqueId( const TagLib::FileRef &fileref );

    /**
     * Get the render() of a tag from TagLib, if possible, to assist in AFT UID generation.
     * @fileref TagLib reference to the file.
     * @return TagLib::ByteVector that contains the tag's render, if possible, and empty otherwise.
     */
    static const TagLib::ByteVector generatedUniqueIdHelper( const TagLib::FileRef &fileref );

    /**
     * Get the render() of a tag from TagLib, if possible, to assist in AFT UID generation.
     * @path Path of the file.
     * @return QString contains the uniqueID, an MD5 hash (but not a hash of the entire file!).
     */
    static const QString readUniqueId( const QString &path );

    /**
     * Converts the peak value from the standard representation (position on the digital scale
     * after decoding and adjusting the gain) to a decibel value.
     *
     * @param scaleVal the peak in digital scale form
     */
    static qreal peakToDecibels( qreal scaleVal );

    /**
     * Converts the peak value from the standard representation (position on the digital scale
     * after decoding and adjusting the gain) to a decibel value.
     *
     * @param scaleVal the peak in digital scale form as a TagLib::String
     */
    static void addPeakFromScale( const TagLib::String &scaleVal, const QString &key, AttributeHash *attributes );

    /**
     * Strips any trailing " dB" from the input and checks that it is a valid real number
     * before adding it to the attributes hash.
     *
     * @param input the value read from the tags
     * @param key the key to store it in ("trackgain" or "albumgain")
     * @param attributes used to store the tags
     */
    static void addGain( const TagLib::String &input, const QString &key, AttributeHash *attributes );

    /**
     * Reads the replay gain tags (if any) from an MP3 file.
     *
     * The tags will be stored in the following keys:
     * - trackgain
     * - trackpeakgain
     * - albumgain
     * - albumpeakgain
     *
     * @param file the MP3 file
     * @param attributes used to store the tags
     */
    static void readMP3ReplayGainTags( TagLib::MPEG::File *file, AttributeHash *attributes );

    /**
     * Reads the peak value in the format it is stored in RVA2 frames.
     */
    static qreal readRVA2PeakValue( const TagLib::ByteVector &data, int bits, bool *ok = 0 );

    /**
     * Read metadata tags of a given file.
     * @track Track for the file.
     * @return QMap containing tags, or empty QMap on failure.
     */
    AttributeHash readTags( const QString &path, TagLib::AudioProperties::ReadStyle readStyle = TagLib::AudioProperties::Fast );

    /**
     * Helper method for writing XML elements to stdout.
     * @name Name of the element.
     * @attributes Key/value map of attributes.
     */
    void writeElement( const QString& name, const AttributeHash& attributes );

    /**
     * @return the LOWERCASE file extension without the preceding '.', r "" if there is none
     */
    inline QString extension( const QString &fileName )
    {
        return fileName.contains( '.' ) ? fileName.mid( fileName.lastIndexOf( '.' ) + 1 ).toLower() : "";
    }

    /**
     * @return the last directory in @param fileName
     */
    inline QString directory( const QString &fileName )
    {
        return fileName.section( '/', 0, -2 );
    }

    const bool    m_importPlaylists;
    QStringList   m_folders;
    const bool    m_recursively;
    const bool    m_incremental;
    const bool    m_restart;
    const QString m_logfile;
    QStringList   m_scannedFolders;
    QDBusInterface *m_amarokCollectionInterface;

    // Disable copy constructor and assignment
    CollectionScanner( const CollectionScanner& );
    CollectionScanner& operator= ( const CollectionScanner& );

};


#endif // COLLECTIONSCANNER_H

