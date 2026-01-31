#include "core/FileSystemEngine.h"
#include <QFileInfo>
#include <QDir>
#include <iostream>
#include <fstream>

FileSystemEngine::FileSystemEngine(QObject *parent) : QObject(parent) {}

std::future<QVector<FileInfo>> FileSystemEngine::listDirectory(const QString &path) {
    return std::async(std::launch::async, [path, this]() {
        QVector<FileInfo> results;
        try {
            fs::path p(path.toStdString());
            if (fs::exists(p) && fs::is_directory(p)) {
                for (const auto &entry : fs::directory_iterator(p)) {
                    FileInfo info;
                    info.name = QString::fromStdString(entry.path().filename().string());
                    info.absolutePath = QString::fromStdString(fs::absolute(entry.path()).string());
                    info.isDir = entry.is_directory();
                    info.isHidden = info.name.startsWith(".");
                    
                    auto status = entry.status();
                    info.permissions = getPermissionsString(status.permissions());
                    
                    if (!info.isDir) {
                        info.size = fs::file_size(entry.path());
                        info.type = "File";
                    } else {
                        info.size = 0;
                        info.type = "Folder";
                    }
                    
                    auto ftime = fs::last_write_time(entry.path());
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                    info.modified = QDateTime::fromSecsSinceEpoch(cftime);
                    
                    results.push_back(info);
                }
            }
        } catch (const std::exception &e) {
            // Error handling will be more robust in final version
        }
        return results;
    });
}

void FileSystemEngine::stopSearch() {
    m_stopSearch = true;
}

void FileSystemEngine::searchAsync(const QString &query, SearchCallback callback) {
    m_stopSearch = false;
    std::thread([this, query, callback]() {
        try {
            fs::path root("/");
            
            // Directories to exclude to prevent hangs/loops/crashes
            std::vector<std::string> excluded = {
                "/proc", "/sys", "/dev", "/run", "/tmp", "/mnt", "/media", "/var/run", "/var/lock"
            };

            auto isExcluded = [&](const fs::path& p) {
                std::string pathStr = p.string();
                for (const auto& ex : excluded) {
                    if (pathStr == ex || pathStr.rfind(ex + "/", 0) == 0) {
                        return true;
                    }
                }
                return false;
            };

            for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied);
                 it != fs::recursive_directory_iterator(); ++it) {
                
                if (m_stopSearch) break;

                try {
                    // Check exclusion before processing
                    if (isExcluded(it->path())) {
                        it.disable_recursion_pending();
                        continue;
                    }

                    const auto& entry = *it;
                    QString fileName = QString::fromStdString(entry.path().filename().string());
                    
                    if (fileName.contains(query, Qt::CaseInsensitive)) {
                        FileInfo info;
                        info.name = fileName;
                        info.absolutePath = QString::fromStdString(fs::absolute(entry.path()).string());
                        info.isDir = entry.is_directory();
                        info.isHidden = info.name.startsWith(".");
                        
                        // Heuristic Scoring
                        info.score = 0;
                        // 1. Exact match (case insensitive): +100
                        if (fileName.compare(query, Qt::CaseInsensitive) == 0) info.score += 100;
                        // 2. Starts with: +50
                        else if (fileName.startsWith(query, Qt::CaseInsensitive)) info.score += 50;
                        // 3. Contains: +10 (already guaranteed)
                        else info.score += 10;
                        
                        // 4. Penalty for depth/length: -1 per character in path (prefer shorter paths)
                        info.score -= info.absolutePath.length();
                        
                        callback(info);
                    }
                } catch (...) {
                    it.disable_recursion_pending();
                }
            }
        } catch (...) {
        }
    }).detach(); 
}

QString FileSystemEngine::getPermissionsString(fs::perms p) {
    QString res;
    res += ((p & fs::perms::owner_read) != fs::perms::none ? "r" : "-");
    res += ((p & fs::perms::owner_write) != fs::perms::none ? "w" : "-");
    res += ((p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-");
    res += ((p & fs::perms::group_read) != fs::perms::none ? "r" : "-");
    res += ((p & fs::perms::group_write) != fs::perms::none ? "w" : "-");
    res += ((p & fs::perms::group_exec) != fs::perms::none ? "x" : "-");
    res += ((p & fs::perms::others_read) != fs::perms::none ? "r" : "-");
    res += ((p & fs::perms::others_write) != fs::perms::none ? "w" : "-");
    res += ((p & fs::perms::others_exec) != fs::perms::none ? "x" : "-");
    return res;
}

bool FileSystemEngine::copy(const QString &src, const QString &dest) {
    try {
        fs::copy(src.toStdString(), dest.toStdString(), fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        return true;
    } catch (...) { return false; }
}

bool FileSystemEngine::move(const QString &src, const QString &dest) {
    try {
        fs::rename(src.toStdString(), dest.toStdString());
        return true;
    } catch (...) { return false; }
}

bool FileSystemEngine::remove(const QString &path, bool permanent) {
    try {
        if (permanent) {
            return fs::remove_all(path.toStdString()) > 0;
        } else {
            // Trash logic would go here, for MVP just permanent delete or simple remove
            return fs::remove_all(path.toStdString()) > 0;
        }
    } catch (...) { return false; }
}

bool FileSystemEngine::rename(const QString &oldPath, const QString &newName) {
    try {
        fs::path p(oldPath.toStdString());
        fs::rename(p, p.parent_path() / newName.toStdString());
        return true;
    } catch (...) { return false; }
}

bool FileSystemEngine::createFolder(const QString &path) {
    try {
        return fs::create_directory(path.toStdString());
    } catch (...) { return false; }
}

bool FileSystemEngine::createFile(const QString &path) {
    try {
        std::ofstream ofs(path.toStdString());
        return ofs.good();
    } catch (...) { return false; }
}

QString FileSystemEngine::homePath() {
    return QDir::homePath();
}

QString FileSystemEngine::rootPath() {
    return "/";
}
