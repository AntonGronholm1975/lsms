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

QRectF Project::cropRect(int index) const
{
    if (index < 0 || index >= m_cropRects.size())
        return {};
    return m_cropRects.at(index);
}

ChromaKeySettings Project::chromaSettings(int index) const
{
    if (index < 0 || index >= m_chromaSettings.size())
        return {};
    return m_chromaSettings.at(index);
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
        m_cropRects.append(QRectF());
        m_chromaSettings.append(ChromaKeySettings{});
        m_frameDurations.append(1);
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
    if (index < m_cropRects.size())
        m_cropRects.remove(index);
    if (index < m_chromaSettings.size())
        m_chromaSettings.remove(index);
    if (index < m_frameDurations.size())
        m_frameDurations.remove(index);
    setModified(true);
    emit framesChanged();
}

void Project::duplicateFrame(int index)
{
    if (index < 0 || index >= m_imagePaths.size())
        return;
    m_imagePaths.insert(index + 1, m_imagePaths.at(index));
    m_pixmaps.insert(index + 1, m_pixmaps.at(index));
    QRectF cr = (index < m_cropRects.size()) ? m_cropRects.at(index) : QRectF();
    m_cropRects.insert(index + 1, cr);
    ChromaKeySettings ck = (index < m_chromaSettings.size()) ? m_chromaSettings.at(index) : ChromaKeySettings{};
    m_chromaSettings.insert(index + 1, ck);
    int dur = (index < m_frameDurations.size()) ? m_frameDurations.at(index) : 1;
    m_frameDurations.insert(index + 1, dur);
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
    if (from < m_cropRects.size() && to < m_cropRects.size())
        m_cropRects.move(from, to);
    if (from < m_chromaSettings.size() && to < m_chromaSettings.size())
        m_chromaSettings.move(from, to);
    if (from < m_frameDurations.size() && to < m_frameDurations.size())
        m_frameDurations.move(from, to);
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

void Project::setCropRect(int index, const QRectF& rect)
{
    if (index < 0 || index >= m_imagePaths.size())
        return;
    while (m_cropRects.size() <= index)
        m_cropRects.append(QRectF());
    m_cropRects[index] = rect;
    setModified(true);
}

void Project::setCropRectAllFrames(const QRectF& rect)
{
    m_cropRects.fill(rect, m_imagePaths.size());
    setModified(true);
}

void Project::setChromaSettings(int index, const ChromaKeySettings& s)
{
    if (index < 0 || index >= m_imagePaths.size())
        return;
    while (m_chromaSettings.size() <= index)
        m_chromaSettings.append(ChromaKeySettings{});
    m_chromaSettings[index] = s;
    setModified(true);
}

void Project::setChromaSettingsAllFrames(const ChromaKeySettings& s)
{
    m_chromaSettings.fill(s, m_imagePaths.size());
    setModified(true);
}

int Project::frameDuration(int index) const
{
    if (index < 0 || index >= m_frameDurations.size())
        return 1;
    return qMax(1, m_frameDurations.at(index));
}

void Project::setFrameDuration(int index, int ticks)
{
    if (index < 0 || index >= m_imagePaths.size())
        return;
    while (m_frameDurations.size() <= index)
        m_frameDurations.append(1);
    m_frameDurations[index] = qMax(1, ticks);
    setModified(true);
}

void Project::setFrameDurationAllFrames(int ticks)
{
    m_frameDurations.fill(qMax(1, ticks), m_imagePaths.size());
    setModified(true);
}

QString Project::audioFilePath() const { return m_audioFilePath; }

void Project::setAudioFilePath(const QString& path)
{
    if (m_audioFilePath == path) return;
    m_audioFilePath = path;
    setModified(true);
}

bool Project::cropLayerEnabled()  const { return m_cropLayerEnabled; }
bool Project::chromaLayerEnabled() const { return m_chromaLayerEnabled; }

void Project::setCropLayerEnabled(bool enabled)
{
    if (m_cropLayerEnabled == enabled) return;
    m_cropLayerEnabled = enabled;
    setModified(true);
    emit layerVisibilityChanged();
}

void Project::setChromaLayerEnabled(bool enabled)
{
    if (m_chromaLayerEnabled == enabled) return;
    m_chromaLayerEnabled = enabled;
    setModified(true);
    emit layerVisibilityChanged();
}

bool Project::save(const QString& path)
{
    QFileInfo projectFile(path);
    QDir projectDir = projectFile.absoluteDir();

    QJsonArray framesArray;
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        QJsonObject frameObj;
        frameObj["path"] = projectDir.relativeFilePath(m_imagePaths.at(i));
        if (i < m_cropRects.size() && !m_cropRects.at(i).isEmpty()) {
            const QRectF& cr = m_cropRects.at(i);
            QJsonObject cropObj;
            cropObj["x"] = cr.x();
            cropObj["y"] = cr.y();
            cropObj["w"] = cr.width();
            cropObj["h"] = cr.height();
            frameObj["crop"] = cropObj;
        }
        if (i < m_chromaSettings.size() && m_chromaSettings.at(i).enabled) {
            // chroma JSON ... (unchanged)
            const ChromaKeySettings& ck = m_chromaSettings.at(i);
            QJsonObject ckObj;
            ckObj["enabled"]       = ck.enabled;
            ckObj["keyColor"]      = ck.keyColor.name();
            ckObj["tolerance"]     = ck.tolerance;
            ckObj["feather"]       = ck.feather;
            ckObj["spillSuppress"] = ck.spillSuppress;
            ckObj["bgMode"]        = int(ck.bgMode);
            ckObj["bgColor"]       = ck.bgColor.name();
            ckObj["bgImagePath"]   = ck.bgImagePath;
            frameObj["chroma"]     = ckObj;
        }
        int dur = (i < m_frameDurations.size()) ? m_frameDurations.at(i) : 1;
        if (dur != 1)
            frameObj["duration"] = dur;
        framesArray.append(frameObj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["fps"] = m_fps;
    if (!m_audioFilePath.isEmpty())
        root["audioFile"] = m_audioFilePath;
    root["frames"] = framesArray;

    QJsonObject layers;
    layers["cropEnabled"]   = m_cropLayerEnabled;
    layers["chromaEnabled"] = m_chromaLayerEnabled;
    root["layers"] = layers;

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
        QJsonObject fobj = v.toObject();
        QString relPath = fobj["path"].toString();
        QString absPath = projectDir.absoluteFilePath(relPath);
        QPixmap px(absPath);
        if (px.isNull())
            continue;
        m_imagePaths.append(absPath);
        m_pixmaps.append(px);
        QRectF cr;
        if (fobj.contains("crop")) {
            QJsonObject c = fobj["crop"].toObject();
            cr = QRectF(c["x"].toDouble(), c["y"].toDouble(),
                        c["w"].toDouble(), c["h"].toDouble());
        }
        m_cropRects.append(cr);
        ChromaKeySettings ck;
        if (fobj.contains("chroma")) {
            QJsonObject c = fobj["chroma"].toObject();
            ck.enabled       = c["enabled"].toBool();
            ck.keyColor      = QColor(c["keyColor"].toString());
            ck.tolerance     = c["tolerance"].toInt(80);
            ck.feather       = c["feather"].toInt(15);
            ck.spillSuppress = c["spillSuppress"].toInt(30);
            ck.bgMode        = ChromaKeySettings::BgMode(c["bgMode"].toInt(0));
            ck.bgColor       = QColor(c["bgColor"].toString("#000000"));
            ck.bgImagePath   = c["bgImagePath"].toString();
        }
        m_chromaSettings.append(ck);
        m_frameDurations.append(fobj["duration"].toInt(1));
    }

    emit fpsChanged(m_fps);
    emit framesChanged();

    m_audioFilePath = root["audioFile"].toString();

    if (root.contains("layers")) {
        QJsonObject layers = root["layers"].toObject();
        m_cropLayerEnabled   = layers["cropEnabled"].toBool(true);
        m_chromaLayerEnabled = layers["chromaEnabled"].toBool(true);
    }
    emit layerVisibilityChanged();
    return true;
}

void Project::reset()
{
    m_imagePaths.clear();
    m_pixmaps.clear();
    m_cropRects.clear();
    m_chromaSettings.clear();
    m_frameDurations.clear();
    m_audioFilePath.clear();
    m_fps = 12;
    m_filePath.clear();
    m_modified = false;
    m_cropLayerEnabled   = true;
    m_chromaLayerEnabled  = true;
    emit layerVisibilityChanged();
}

void Project::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit modifiedChanged(m_modified);
}
