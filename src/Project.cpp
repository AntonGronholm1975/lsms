#include "Project.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Project::Project(QObject* parent)
    : QObject(parent)
{
}

int Project::frameCount() const
{
    return m_imagePaths.size();
}

QPixmap Project::pixmap(int index) const
{
    if (index < 0 || index >= m_pixmaps.size())
        return {};
    return m_pixmaps.at(index);
}

QString Project::imagePath(int index) const
{
    if (index < 0 || index >= m_imagePaths.size())
        return {};
    return m_imagePaths.at(index);
}

int Project::fps() const
{
    return m_fps;
}

QString Project::filePath() const
{
    return m_filePath;
}

bool Project::isModified() const
{
    return m_modified;
}

void Project::addImages(const QStringList& paths)
{
    bool added = false;
    for (const QString& path : paths) {
        QPixmap px(path);
        if (px.isNull())
            continue;
        m_imagePaths.append(path);
        m_pixmaps.append(px);
        added = true;
    }
    if (added) {
        setModified(true);
        emit framesChanged();
    }
}

void Project::removeFrame(int index)
{
    if (index < 0 || index >= m_imagePaths.size())
        return;
    m_imagePaths.remove(index);
    m_pixmaps.remove(index);
    setModified(true);
    emit framesChanged();
}

void Project::duplicateFrame(int index)
{
    if (index < 0 || index >= m_imagePaths.size())
        return;
    m_imagePaths.insert(index + 1, m_imagePaths.at(index));
    m_pixmaps.insert(index + 1, m_pixmaps.at(index));
    setModified(true);
    emit framesChanged();
}

void Project::moveFrame(int from, int to)
{
    if (from == to)
        return;
    if (from < 0 || from >= m_imagePaths.size())
        return;
    if (to < 0 || to >= m_imagePaths.size())
        return;
    m_imagePaths.move(from, to);
    m_pixmaps.move(from, to);
    setModified(true);
    emit framesChanged();
}

void Project::setFps(int fps)
{
    if (m_fps == fps)
        return;
    m_fps = fps;
    setModified(true);
    emit fpsChanged(m_fps);
}

bool Project::save(const QString& path)
{
    QFileInfo projectFile(path);
    QDir projectDir = projectFile.absoluteDir();

    QJsonArray framesArray;
    for (const QString& imgPath : m_imagePaths) {
        QJsonObject frameObj;
        frameObj["path"] = projectDir.relativeFilePath(imgPath);
        framesArray.append(frameObj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["fps"] = m_fps;
    root["frames"] = framesArray;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(QJsonDocument(root).toJson());
    m_filePath = path;
    setModified(false);
    return true;
}

bool Project::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (err.error != QJsonParseError::NoError)
        return false;

    QJsonObject root = doc.object();
    QFileInfo projectFile(path);
    QDir projectDir = projectFile.absoluteDir();

    reset();

    m_fps = root["fps"].toInt(12);
    m_filePath = path;

    for (const QJsonValue& v : root["frames"].toArray()) {
        QString relPath = v.toObject()["path"].toString();
        QString absPath = projectDir.absoluteFilePath(relPath);
        QPixmap px(absPath);
        if (px.isNull())
            continue;
        m_imagePaths.append(absPath);
        m_pixmaps.append(px);
    }

    emit fpsChanged(m_fps);
    emit framesChanged();
    return true;
}

void Project::reset()
{
    m_imagePaths.clear();
    m_pixmaps.clear();
    m_fps = 12;
    m_filePath.clear();
    m_modified = false;
}

void Project::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit modifiedChanged(m_modified);
}
