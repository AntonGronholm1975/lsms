#pragma once

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QVector>

class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(QObject* parent = nullptr);

    int frameCount() const;
    QPixmap pixmap(int index) const;
    QString imagePath(int index) const;
    int fps() const;
    QString filePath() const;
    bool isModified() const;

    void addImages(const QStringList& paths);
    void removeFrame(int index);
    void duplicateFrame(int index);
    void moveFrame(int from, int to);
    void setFps(int fps);

    bool save(const QString& path);
    bool load(const QString& path);
    void reset();

signals:
    void framesChanged();
    void fpsChanged(int fps);
    void modifiedChanged(bool modified);

private:
    void setModified(bool modified);

    QVector<QString> m_imagePaths;
    QVector<QPixmap> m_pixmaps;
    int m_fps = 12;
    QString m_filePath;
    bool m_modified = false;
};
