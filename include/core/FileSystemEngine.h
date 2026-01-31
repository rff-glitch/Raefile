#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QObject>
#include <filesystem>
#include <future>

namespace fs = std::filesystem;

struct FileInfo {
    QString name;
    long long size;
    QString type;
    QString permissions;
    QDateTime modified;
    bool isDir;
    bool isHidden;
    QString absolutePath;
    int score; // Relevance score for search
};

class FileSystemEngine : public QObject {
    Q_OBJECT
public:
    explicit FileSystemEngine(QObject *parent = nullptr);
    
    // Async directory listing
    std::future<QVector<FileInfo>> listDirectory(const QString &path);

    // Global Search
    using SearchCallback = std::function<void(const FileInfo&)>;
    void searchAsync(const QString &query, SearchCallback callback);
    void stopSearch();
    
    // File operations
    bool copy(const QString &src, const QString &dest);
    bool move(const QString &src, const QString &dest);
    bool remove(const QString &path, bool permanent = false);
    bool rename(const QString &oldPath, const QString &newName);
    bool createFolder(const QString &path);
    bool createFile(const QString &path);
    
    // Helper to get user home
    static QString homePath();
    static QString rootPath();

private:
    QString getPermissionsString(fs::perms p);
    std::atomic<bool> m_stopSearch{false};
};
