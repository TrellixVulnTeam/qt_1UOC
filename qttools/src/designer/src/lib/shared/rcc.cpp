// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/* Note: This is a copy of qtbase/src/tools/rcc/rcc.cpp. */

#include "rcc_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qlocale.h>
#include <QtCore/qstack.h>
#include <QtCore/qxmlstream.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

enum {
    CONSTANT_USENAMESPACE = 1,
    CONSTANT_COMPRESSLEVEL_DEFAULT = -1,
    CONSTANT_COMPRESSTHRESHOLD_DEFAULT = 70
};


#define writeString(s) write(s, sizeof(s))

void RCCResourceLibrary::write(const char *str, int len)
{
    --len; // trailing \0 on string literals...
    int n = m_out.size();
    m_out.resize(n + len);
    memcpy(m_out.data() + n, str, len);
}

void RCCResourceLibrary::writeByteArray(const QByteArray &other)
{
    m_out.append(other);
}

static inline QString msgOpenReadFailed(const QString &fname, const QString &why)
{
    return QString::fromUtf8("Unable to open %1 for reading: %2\n").arg(fname).arg(why);
}


///////////////////////////////////////////////////////////
//
// RCCFileInfo
//
///////////////////////////////////////////////////////////

class RCCFileInfo
{
public:
    enum Flags
    {
        NoFlags = 0x00,
        Compressed = 0x01,
        Directory = 0x02
    };

    RCCFileInfo(const QString &name = QString(), const QFileInfo &fileInfo = QFileInfo(),
                QLocale::Language language = QLocale::C,
                QLocale::Territory territory = QLocale::AnyTerritory,
                uint flags = NoFlags,
                int compressLevel = CONSTANT_COMPRESSLEVEL_DEFAULT,
                int compressThreshold = CONSTANT_COMPRESSTHRESHOLD_DEFAULT);
    ~RCCFileInfo();

    QString resourceName() const;

public:
    qint64 writeDataBlob(RCCResourceLibrary &lib, qint64 offset, QString *errorMessage);
    qint64 writeDataName(RCCResourceLibrary &, qint64 offset);
    void writeDataInfo(RCCResourceLibrary &lib);

    int m_flags;
    QString m_name;
    QLocale::Language m_language;
    QLocale::Territory m_territory;
    QFileInfo m_fileInfo;
    RCCFileInfo *m_parent;
    QHash<QString, RCCFileInfo*> m_children;
    int m_compressLevel;
    int m_compressThreshold;

    qint64 m_nameOffset;
    qint64 m_dataOffset;
    qint64 m_childOffset;
};

RCCFileInfo::RCCFileInfo(const QString &name, const QFileInfo &fileInfo,
    QLocale::Language language, QLocale::Territory territory, uint flags,
    int compressLevel, int compressThreshold)
{
    m_name = name;
    m_fileInfo = fileInfo;
    m_language = language;
    m_territory = territory;
    m_flags = flags;
    m_parent = nullptr;
    m_nameOffset = 0;
    m_dataOffset = 0;
    m_childOffset = 0;
    m_compressLevel = compressLevel;
    m_compressThreshold = compressThreshold;
}

RCCFileInfo::~RCCFileInfo()
{
    qDeleteAll(m_children);
}

QString RCCFileInfo::resourceName() const
{
    QString resource = m_name;
    for (RCCFileInfo *p = m_parent; p; p = p->m_parent)
        resource = resource.prepend(p->m_name + QLatin1Char('/'));
    return QLatin1Char(':') + resource;
}

void RCCFileInfo::writeDataInfo(RCCResourceLibrary &lib)
{
    const bool text = (lib.m_format == RCCResourceLibrary::C_Code);
    //some info
    if (text) {
        if (m_language != QLocale::C) {
            lib.writeString("  // ");
            lib.writeByteArray(resourceName().toLocal8Bit());
            lib.writeString(" [");
            lib.writeByteArray(QByteArray::number(m_territory));
            lib.writeString("::");
            lib.writeByteArray(QByteArray::number(m_language));
            lib.writeString("[\n  ");
        } else {
            lib.writeString("  // ");
            lib.writeByteArray(resourceName().toLocal8Bit());
            lib.writeString("\n  ");
        }
    }

    //pointer data
    if (m_flags & RCCFileInfo::Directory) {
        // name offset
        lib.writeNumber4(m_nameOffset);

        // flags
        lib.writeNumber2(m_flags);

        // child count
        lib.writeNumber4(m_children.size());

        // first child offset
        lib.writeNumber4(m_childOffset);
    } else {
        // name offset
        lib.writeNumber4(m_nameOffset);

        // flags
        lib.writeNumber2(m_flags);

        // locale
        lib.writeNumber2(m_territory);
        lib.writeNumber2(m_language);

        //data offset
        lib.writeNumber4(m_dataOffset);
    }
    if (text)
        lib.writeChar('\n');
}

qint64 RCCFileInfo::writeDataBlob(RCCResourceLibrary &lib, qint64 offset,
    QString *errorMessage)
{
    const bool text = (lib.m_format == RCCResourceLibrary::C_Code);

    //capture the offset
    m_dataOffset = offset;

    //find the data to be written
    QFile file(m_fileInfo.absoluteFilePath());
    if (!file.open(QFile::ReadOnly)) {
        *errorMessage = msgOpenReadFailed(m_fileInfo.absoluteFilePath(), file.errorString());
        return 0;
    }
    QByteArray data = file.readAll();

#ifndef QT_NO_COMPRESS
    // Check if compression is useful for this file
    if (m_compressLevel != 0 && data.size() != 0) {
        QByteArray compressed =
            qCompress(reinterpret_cast<uchar *>(data.data()), data.size(), m_compressLevel);

        int compressRatio = int(100.0 * (data.size() - compressed.size()) / data.size());
        if (compressRatio >= m_compressThreshold) {
            data = compressed;
            m_flags |= Compressed;
        }
    }
#endif // QT_NO_COMPRESS

    // some info
    if (text) {
        lib.writeString("  // ");
        lib.writeByteArray(m_fileInfo.absoluteFilePath().toLocal8Bit());
        lib.writeString("\n  ");
    }

    // write the length

    lib.writeNumber4(data.size());
    if (text)
        lib.writeString("\n  ");
    offset += 4;

    // write the payload
    const char *p = data.constData();
    if (text) {
        for (int i = data.size(), j = 0; --i >= 0; --j) {
            lib.writeHex(*p++);
            if (j == 0) {
                lib.writeString("\n  ");
                j = 16;
            }
        }
    } else {
        for (int i = data.size(); --i >= 0; )
           lib.writeChar(*p++);
    }
    offset += data.size();

    // done
    if (text)
        lib.writeString("\n  ");
    return offset;
}

qint64 RCCFileInfo::writeDataName(RCCResourceLibrary &lib, qint64 offset)
{
    const bool text = (lib.m_format == RCCResourceLibrary::C_Code);

    // capture the offset
    m_nameOffset = offset;

    // some info
    if (text) {
        lib.writeString("  // ");
        lib.writeByteArray(m_name.toLocal8Bit());
        lib.writeString("\n  ");
    }

    // write the length
    lib.writeNumber2(m_name.length());
    if (text)
        lib.writeString("\n  ");
    offset += 2;

    // write the hash
    lib.writeNumber4(qt_hash(m_name));
    if (text)
        lib.writeString("\n  ");
    offset += 4;

    // write the m_name
    const QChar *unicode = m_name.unicode();
    for (int i = 0; i < m_name.length(); ++i) {
        lib.writeNumber2(unicode[i].unicode());
        if (text && i % 16 == 0)
            lib.writeString("\n  ");
    }
    offset += m_name.length()*2;

    // done
    if (text)
        lib.writeString("\n  ");
    return offset;
}


///////////////////////////////////////////////////////////
//
// RCCResourceLibrary
//
///////////////////////////////////////////////////////////

RCCResourceLibrary::Strings::Strings() :
   TAG_RCC(QLatin1String("RCC")),
   TAG_RESOURCE(QLatin1String("qresource")),
   TAG_FILE(QLatin1String("file")),
   ATTRIBUTE_LANG(QLatin1String("lang")),
   ATTRIBUTE_PREFIX(QLatin1String("prefix")),
   ATTRIBUTE_ALIAS(QLatin1String("alias")),
   ATTRIBUTE_THRESHOLD(QLatin1String("threshold")),
   ATTRIBUTE_COMPRESS(QLatin1String("compress"))
{
}

RCCResourceLibrary::RCCResourceLibrary()
  : m_root(nullptr),
    m_format(C_Code),
    m_verbose(false),
    m_compressLevel(CONSTANT_COMPRESSLEVEL_DEFAULT),
    m_compressThreshold(CONSTANT_COMPRESSTHRESHOLD_DEFAULT),
    m_treeOffset(0),
    m_namesOffset(0),
    m_dataOffset(0),
    m_useNameSpace(CONSTANT_USENAMESPACE),
    m_errorDevice(0)
{
    m_out.reserve(30 * 1000 * 1000);
}

RCCResourceLibrary::~RCCResourceLibrary()
{
    delete m_root;
}

enum RCCXmlTag {
    RccTag,
    ResourceTag,
    FileTag
};

bool RCCResourceLibrary::interpretResourceFile(QIODevice *inputDevice,
    const QString &fname, QString currentPath, bool ignoreErrors)
{
    Q_ASSERT(m_errorDevice);
    const QChar slash = QLatin1Char('/');
    if (!currentPath.isEmpty() && !currentPath.endsWith(slash))
        currentPath += slash;

    QXmlStreamReader reader(inputDevice);
    QStack<RCCXmlTag> tokens;

    QString prefix;
    QLocale::Language language = QLocale::c().language();
    QLocale::Territory territory = QLocale::c().territory();
    QString alias;
    int compressLevel = m_compressLevel;
    int compressThreshold = m_compressThreshold;

    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType t = reader.readNext();
        switch (t) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == m_strings.TAG_RCC) {
                if (!tokens.isEmpty())
                    reader.raiseError(QLatin1String("expected <RCC> tag"));
                else
                    tokens.push(RccTag);
            } else if (reader.name() == m_strings.TAG_RESOURCE) {
                if (tokens.isEmpty() || tokens.top() != RccTag) {
                    reader.raiseError(QLatin1String("unexpected <RESOURCE> tag"));
                } else {
                    tokens.push(ResourceTag);

                    QXmlStreamAttributes attributes = reader.attributes();
                    language = QLocale::c().language();
                    territory = QLocale::c().territory();

                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_LANG)) {
                        QString attribute = attributes.value(m_strings.ATTRIBUTE_LANG).toString();
                        QLocale lang = QLocale(attribute);
                        language = lang.language();
                        if (2 == attribute.length()) {
                            // Language only
                            territory = QLocale::AnyTerritory;
                        } else {
                            territory = lang.territory();
                        }
                    }

                    prefix.clear();
                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_PREFIX))
                        prefix = attributes.value(m_strings.ATTRIBUTE_PREFIX).toString();
                    if (!prefix.startsWith(slash))
                        prefix.prepend(slash);
                    if (!prefix.endsWith(slash))
                        prefix += slash;
                }
            } else if (reader.name() == m_strings.TAG_FILE) {
                if (tokens.isEmpty() || tokens.top() != ResourceTag) {
                    reader.raiseError(QLatin1String("unexpected <FILE> tag"));
                } else {
                    tokens.push(FileTag);

                    QXmlStreamAttributes attributes = reader.attributes();
                    alias.clear();
                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_ALIAS))
                        alias = attributes.value(m_strings.ATTRIBUTE_ALIAS).toString();

                    compressLevel = m_compressLevel;
                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_COMPRESS))
                        compressLevel = attributes.value(m_strings.ATTRIBUTE_COMPRESS).toString().toInt();

                    compressThreshold = m_compressThreshold;
                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_THRESHOLD))
                        compressThreshold = attributes.value(m_strings.ATTRIBUTE_THRESHOLD).toString().toInt();

                    // Special case for -no-compress. Overrides all other settings.
                    if (m_compressLevel == -2)
                        compressLevel = 0;
                }
            } else {
                reader.raiseError(QString(QLatin1String("unexpected tag: %1")).arg(reader.name().toString()));
            }
            break;

        case QXmlStreamReader::EndElement:
            if (reader.name() == m_strings.TAG_RCC) {
                if (!tokens.isEmpty() && tokens.top() == RccTag)
                    tokens.pop();
                else
                    reader.raiseError(QLatin1String("unexpected closing tag"));
            } else if (reader.name() == m_strings.TAG_RESOURCE) {
                if (!tokens.isEmpty() && tokens.top() == ResourceTag)
                    tokens.pop();
                else
                    reader.raiseError(QLatin1String("unexpected closing tag"));
            } else if (reader.name() == m_strings.TAG_FILE) {
                if (!tokens.isEmpty() && tokens.top() == FileTag)
                    tokens.pop();
                else
                    reader.raiseError(QLatin1String("unexpected closing tag"));
            }
            break;

        case QXmlStreamReader::Characters:
            if (reader.isWhitespace())
                break;
            if (tokens.isEmpty() || tokens.top() != FileTag) {
                reader.raiseError(QLatin1String("unexpected text"));
            } else {
                QString fileName = reader.text().toString();
                if (fileName.isEmpty()) {
                    const QString msg = QString::fromLatin1("RCC: Warning: Null node in XML of '%1'\n").arg(fname);
                    m_errorDevice->write(msg.toUtf8());
                }

                if (alias.isNull())
                    alias = fileName;

                alias = QDir::cleanPath(alias);
                while (alias.startsWith(QLatin1String("../")))
                    alias.remove(0, 3);
                alias = QDir::cleanPath(m_resourceRoot) + prefix + alias;

                QString absFileName = fileName;
                if (QDir::isRelativePath(absFileName))
                    absFileName.prepend(currentPath);
                QFileInfo file(absFileName);
                if (!file.exists()) {
                    m_failedResources.push_back(absFileName);
                    const QString msg = QString::fromLatin1("RCC: Error in '%1': Cannot find file '%2'\n").arg(fname).arg(fileName);
                    m_errorDevice->write(msg.toUtf8());
                    if (ignoreErrors)
                        continue;
                    else
                        return false;
                } else if (file.isFile()) {
                    const bool arc =
                        addFile(alias,
                                RCCFileInfo(alias.section(slash, -1),
                                            file,
                                            language,
                                            territory,
                                            RCCFileInfo::NoFlags,
                                            compressLevel,
                                            compressThreshold)
                                );
                    if (!arc)
                        m_failedResources.push_back(absFileName);
                } else {
                    QDir dir;
                    if (file.isDir()) {
                        dir.setPath(file.filePath());
                    } else {
                        dir.setPath(file.path());
                        dir.setNameFilters(QStringList(file.fileName()));
                        if (alias.endsWith(file.fileName()))
                            alias = alias.left(alias.length()-file.fileName().length());
                    }
                    if (!alias.endsWith(slash))
                        alias += slash;
                    QDirIterator it(dir, QDirIterator::FollowSymlinks|QDirIterator::Subdirectories);
                    while (it.hasNext()) {
                        it.next();
                        QFileInfo child(it.fileInfo());
                        if (child.fileName() != QLatin1String(".") && child.fileName() != QLatin1String("..")) {
                            const bool arc =
                                addFile(alias + child.fileName(),
                                        RCCFileInfo(child.fileName(),
                                                    child,
                                                    language,
                                                    territory,
                                                    RCCFileInfo::NoFlags,
                                                    compressLevel,
                                                    compressThreshold)
                                        );
                            if (!arc)
                                m_failedResources.push_back(child.fileName());
                        }
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    if (reader.hasError()) {
        if (ignoreErrors)
            return true;
        int errorLine = reader.lineNumber();
        int errorColumn = reader.columnNumber();
        QString errorMessage = reader.errorString();
        QString msg = QString::fromLatin1("RCC Parse Error: '%1' Line: %2 Column: %3 [%4]\n").arg(fname).arg(errorLine).arg(errorColumn).arg(errorMessage);
        m_errorDevice->write(msg.toUtf8());
        return false;
    }

    if (m_root == nullptr) {
        const QString msg = QString::fromUtf8("RCC: Warning: No resources in '%1'.\n").arg(fname);
        m_errorDevice->write(msg.toUtf8());
        if (!ignoreErrors && m_format == Binary) {
            // create dummy entry, otherwise loading with QResource will crash
            m_root = new RCCFileInfo(QString(), QFileInfo(),
                    QLocale::C, QLocale::AnyTerritory, RCCFileInfo::Directory);
        }
    }

    return true;
}

bool RCCResourceLibrary::addFile(const QString &alias, const RCCFileInfo &file)
{
    Q_ASSERT(m_errorDevice);
    if (file.m_fileInfo.size() > 0xffffffff) {
        const QString msg = QString::fromUtf8("File too big: %1\n").arg(file.m_fileInfo.absoluteFilePath());
        m_errorDevice->write(msg.toUtf8());
        return false;
    }
    if (!m_root)
        m_root = new RCCFileInfo(QString(), QFileInfo(), QLocale::C, QLocale::AnyTerritory, RCCFileInfo::Directory);

    RCCFileInfo *parent = m_root;
    const QStringList nodes = alias.split(QLatin1Char('/'));
    for (int i = 1; i < nodes.size()-1; ++i) {
        const QString node = nodes.at(i);
        if (node.isEmpty())
            continue;
        if (!parent->m_children.contains(node)) {
            RCCFileInfo *s = new RCCFileInfo(node, QFileInfo(), QLocale::C, QLocale::AnyTerritory, RCCFileInfo::Directory);
            s->m_parent = parent;
            parent->m_children.insert(node, s);
            parent = s;
        } else {
            parent = *parent->m_children.constFind(node);
        }
    }

    const QString filename = nodes.at(nodes.size()-1);
    RCCFileInfo *s = new RCCFileInfo(file);
    s->m_parent = parent;
    if (parent->m_children.contains(filename)) {
        for (const QString &fileName : qAsConst(m_fileNames)) {
            qWarning("%s: Warning: potential duplicate alias detected: '%s'",
                     qPrintable(fileName), qPrintable(filename));
        }
    }
    parent->m_children.insert(filename, s);
    return true;
}

void RCCResourceLibrary::reset()
{
     if (m_root) {
        delete m_root;
        m_root = nullptr;
    }
    m_errorDevice = nullptr;
    m_failedResources.clear();
}


bool RCCResourceLibrary::readFiles(bool ignoreErrors, QIODevice &errorDevice)
{
    reset();
    m_errorDevice = &errorDevice;
    //read in data
    if (m_verbose) {
        const QString msg = QString::fromUtf8("Processing %1 files [%2]\n")
            .arg(m_fileNames.size()).arg(static_cast<int>(ignoreErrors));
        m_errorDevice->write(msg.toUtf8());
    }
    for (int i = 0; i < m_fileNames.size(); ++i) {
        QFile fileIn;
        QString fname = m_fileNames.at(i);
        QString pwd;
        if (fname == QLatin1String("-")) {
            fname = QLatin1String("(stdin)");
            pwd = QDir::currentPath();
            fileIn.setFileName(fname);
            if (!fileIn.open(stdin, QIODevice::ReadOnly)) {
                m_errorDevice->write(msgOpenReadFailed(fname, fileIn.errorString()).toUtf8());
                return false;
            }
        } else {
            pwd = QFileInfo(fname).path();
            fileIn.setFileName(fname);
            if (!fileIn.open(QIODevice::ReadOnly)) {
                m_errorDevice->write(msgOpenReadFailed(fname, fileIn.errorString()).toUtf8());
                return false;
            }
        }
        if (m_verbose) {
            const QString msg = QString::fromUtf8("Interpreting %1\n").arg(fname);
            m_errorDevice->write(msg.toUtf8());
        }

        if (!interpretResourceFile(&fileIn, fname, pwd, ignoreErrors))
            return false;
    }
    return true;
}

QStringList RCCResourceLibrary::dataFiles() const
{
    QStringList ret;
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return ret;
    pending.push(m_root);
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            ret.append(child->m_fileInfo.filePath());
        }
    }
    return ret;
}

// Determine map of resource identifier (':/newPrefix/images/p1.png') to file via recursion
static void resourceDataFileMapRecursion(const RCCFileInfo *m_root, const QString &path, RCCResourceLibrary::ResourceDataFileMap &m)
{
    const QChar slash = QLatin1Char('/');
    for (auto  it = m_root->m_children.constBegin(), cend = m_root->m_children.constEnd(); it != cend; ++it) {
        const RCCFileInfo *child = it.value();
        QString childName = path;
        childName += slash;
        childName += child->m_name;
        if (child->m_flags & RCCFileInfo::Directory) {
            resourceDataFileMapRecursion(child, childName, m);
        } else {
            m.insert(childName, child->m_fileInfo.filePath());
        }
    }
}

RCCResourceLibrary::ResourceDataFileMap RCCResourceLibrary::resourceDataFileMap() const
{
    ResourceDataFileMap rc;
    if (m_root)
        resourceDataFileMapRecursion(m_root, QString(QLatin1Char(':')),  rc);
    return rc;
}

bool RCCResourceLibrary::output(QIODevice &outDevice, QIODevice &errorDevice)
{
    m_errorDevice = &errorDevice;
    //write out
    if (m_verbose)
        m_errorDevice->write("Outputting code\n");
    if (!writeHeader()) {
        m_errorDevice->write("Could not write header\n");
        return false;
    }
    if (m_root) {
        if (!writeDataBlobs()) {
            m_errorDevice->write("Could not write data blobs.\n");
            return false;
        }
        if (!writeDataNames()) {
            m_errorDevice->write("Could not write file names\n");
            return false;
        }
        if (!writeDataStructure()) {
            m_errorDevice->write("Could not write data tree\n");
            return false;
        }
    }
    if (!writeInitializer()) {
        m_errorDevice->write("Could not write footer\n");
        return false;
    }
    outDevice.write(m_out.constData(), m_out.size());
    return true;
}

void RCCResourceLibrary::writeHex(quint8 tmp)
{
    const char digits[] = "0123456789abcdef";
    writeChar('0');
    writeChar('x');
    if (tmp < 16) {
        writeChar(digits[tmp]);
    } else {
        writeChar(digits[tmp >> 4]);
        writeChar(digits[tmp & 0xf]);
    }
    writeChar(',');
}

void RCCResourceLibrary::writeNumber2(quint16 number)
{
    if (m_format == RCCResourceLibrary::Binary) {
        writeChar(number >> 8);
        writeChar(number);
    } else {
        writeHex(number >> 8);
        writeHex(number);
    }
}

void RCCResourceLibrary::writeNumber4(quint32 number)
{
    if (m_format == RCCResourceLibrary::Binary) {
        writeChar(number >> 24);
        writeChar(number >> 16);
        writeChar(number >> 8);
        writeChar(number);
    } else {
        writeHex(number >> 24);
        writeHex(number >> 16);
        writeHex(number >> 8);
        writeHex(number);
    }
}

bool RCCResourceLibrary::writeHeader()
{
    if (m_format == C_Code) {
        writeString("/****************************************************************************\n");
        writeString("** Resource object code\n");
        writeString("**\n");
        writeString("** Created: ");
        writeByteArray(QDateTime::currentDateTime().toString().toLatin1());
        writeString("\n**      by: The Resource Compiler for Qt version ");
        writeByteArray(QT_VERSION_STR);
        writeString("\n**\n");
        writeString("** WARNING! All changes made in this file will be lost!\n");
        writeString( "*****************************************************************************/\n\n");
        writeString("#include <QtCore/qglobal.h>\n\n");
    } else if (m_format == Binary) {
        writeString("qres");
        writeNumber4(0);
        writeNumber4(0);
        writeNumber4(0);
        writeNumber4(0);
    }
    return true;
}

bool RCCResourceLibrary::writeDataBlobs()
{
    Q_ASSERT(m_errorDevice);
    if (m_format == C_Code)
        writeString("static const unsigned char qt_resource_data[] = {\n");
    else if (m_format == Binary)
        m_dataOffset = m_out.size();
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return false;

    pending.push(m_root);
    qint64 offset = 0;
    QString errorMessage;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            else {
                offset = child->writeDataBlob(*this, offset, &errorMessage);
                if (offset == 0) {
                    m_errorDevice->write(errorMessage.toUtf8());
                    return false;
                }
            }
        }
    }
    if (m_format == C_Code)
        writeString("\n};\n\n");
    return true;
}

bool RCCResourceLibrary::writeDataNames()
{
    if (m_format == C_Code)
        writeString("static const unsigned char qt_resource_name[] = {\n");
    else if (m_format == Binary)
        m_namesOffset = m_out.size();

    QHash<QString, int> names;
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return false;

    pending.push(m_root);
    qint64 offset = 0;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            if (names.contains(child->m_name)) {
                child->m_nameOffset = names.value(child->m_name);
            } else {
                names.insert(child->m_name, offset);
                offset = child->writeDataName(*this, offset);
            }
        }
    }
    if (m_format == C_Code)
        writeString("\n};\n\n");
    return true;
}

static bool qt_rcc_compare_hash(const RCCFileInfo *left, const RCCFileInfo *right)
{
    return qt_hash(left->m_name) < qt_hash(right->m_name);
}

bool RCCResourceLibrary::writeDataStructure()
{
    if (m_format == C_Code)
        writeString("static const unsigned char qt_resource_struct[] = {\n");
    else if (m_format == Binary)
        m_treeOffset = m_out.size();
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return false;

    //calculate the child offsets (flat)
    pending.push(m_root);
    int offset = 1;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        file->m_childOffset = offset;

        //sort by hash value for binary lookup
        auto children = file->m_children.values();
        std::sort(children.begin(), children.end(), qt_rcc_compare_hash);

        //write out the actual data now
        for (RCCFileInfo *child : children) {
            ++offset;
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }

    //write out the structure (ie iterate again!)
    pending.push(m_root);
    m_root->writeDataInfo(*this);
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();

        //sort by hash value for binary lookup
        auto children = file->m_children.values();
        std::sort(children.begin(), children.end(), qt_rcc_compare_hash);

        //write out the actual data now
        for (RCCFileInfo *child : children) {
            child->writeDataInfo(*this);
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }
    if (m_format == C_Code)
        writeString("\n};\n\n");

    return true;
}

void RCCResourceLibrary::writeMangleNamespaceFunction(const QByteArray &name)
{
    if (m_useNameSpace) {
        writeString("QT_MANGLE_NAMESPACE(");
        writeByteArray(name);
        writeChar(')');
    } else {
        writeByteArray(name);
    }
}

void RCCResourceLibrary::writeAddNamespaceFunction(const QByteArray &name)
{
    if (m_useNameSpace) {
        writeString("QT_PREPEND_NAMESPACE(");
        writeByteArray(name);
        writeChar(')');
    } else {
        writeByteArray(name);
    }
}

static bool unacceptableChar(QChar qc)
{
    if (qc.isDigit())
        return false;
    if (!qc.isLetter())
        return true;
    auto c = qc.toLower().toLatin1();
    return c < 'a' || c > 'z';
}

bool RCCResourceLibrary::writeInitializer()
{
    if (m_format == C_Code) {
        //write("\nQT_BEGIN_NAMESPACE\n");
        QString initName = m_initName;
        if (!initName.isEmpty()) {
            initName.prepend(QLatin1Char('_'));
            std::replace_if(initName.begin(), initName.end(),
                            unacceptableChar, QLatin1Char('_'));
        }

        //init
        if (m_useNameSpace)
            writeString("QT_BEGIN_NAMESPACE\n\n");
        if (m_root) {
            writeString("extern Q_CORE_EXPORT bool qRegisterResourceData\n    "
                "(int, const unsigned char *, "
                "const unsigned char *, const unsigned char *);\n\n");
            writeString("extern Q_CORE_EXPORT bool qUnregisterResourceData\n    "
                "(int, const unsigned char *, "
                "const unsigned char *, const unsigned char *);\n\n");
        }
        if (m_useNameSpace)
            writeString("QT_END_NAMESPACE\n\n\n");
        QString initResources = QLatin1String("qInitResources");
        initResources += initName;
        writeString("int ");
        writeMangleNamespaceFunction(initResources.toLatin1());
        writeString("()\n{\n");

        if (m_root) {
            writeString("    ");
            writeAddNamespaceFunction("qRegisterResourceData");
            writeString("\n        (0x01, qt_resource_struct, "
                       "qt_resource_name, qt_resource_data);\n");
        }
        writeString("    return 1;\n");
        writeString("}\n\n");
        writeString("Q_CONSTRUCTOR_FUNCTION(");
        writeMangleNamespaceFunction(initResources.toLatin1());
        writeString(")\n\n");

        //cleanup
        QString cleanResources = QLatin1String("qCleanupResources");
        cleanResources += initName;
        writeString("int ");
        writeMangleNamespaceFunction(cleanResources.toLatin1());
        writeString("()\n{\n");
        if (m_root) {
            writeString("    ");
            writeAddNamespaceFunction("qUnregisterResourceData");
            writeString("\n       (0x01, qt_resource_struct, "
                      "qt_resource_name, qt_resource_data);\n");
        }
        writeString("    return 1;\n");
        writeString("}\n\n");
        writeString("Q_DESTRUCTOR_FUNCTION(");
        writeMangleNamespaceFunction(cleanResources.toLatin1());
        writeString(")\n\n");
    } else if (m_format == Binary) {
        int i = 4;
        char *p = m_out.data();
        p[i++] = 0; // 0x01
        p[i++] = 0;
        p[i++] = 0;
        p[i++] = 1;

        p[i++] = (m_treeOffset >> 24) & 0xff;
        p[i++] = (m_treeOffset >> 16) & 0xff;
        p[i++] = (m_treeOffset >>  8) & 0xff;
        p[i++] = (m_treeOffset >>  0) & 0xff;

        p[i++] = (m_dataOffset >> 24) & 0xff;
        p[i++] = (m_dataOffset >> 16) & 0xff;
        p[i++] = (m_dataOffset >>  8) & 0xff;
        p[i++] = (m_dataOffset >>  0) & 0xff;

        p[i++] = (m_namesOffset >> 24) & 0xff;
        p[i++] = (m_namesOffset >> 16) & 0xff;
        p[i++] = (m_namesOffset >>  8) & 0xff;
        p[i++] = (m_namesOffset >>  0) & 0xff;
    }
    return true;
}

QT_END_NAMESPACE
