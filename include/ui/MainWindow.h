#pragma once

#include <QMainWindow>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QListWidget>
#include <QStackedWidget>
#include "core/FileSystemEngine.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onDirectoryLoaded(const QString &path);
    void onFileDoubleClicked(const QModelIndex &index);
    void onSearchResultClicked(QListWidgetItem *item);
    void onSideBarClicked(QListWidgetItem *item);
    void startGlobalSearch();
    void showContextMenu(const QPoint &pos);
    
    // Navigation
    void goHome();
    void goRoot();
    void goUp();
    void refreshView();
    void toggleHiddenFiles();
    
    // File Actions
    void createNewFolder();
    void createNewFile();
    void deleteSelected();
    void renameSelected();
    void copySelected();
    void cutSelected();
    void pasteToCurrent();

private:
    void setupUI();
    void setupSidebar();
    void setupActions();
    void setupShortcuts();
    void applyModernStyle();
    
    FileSystemEngine *m_engine;
    
    QStackedWidget *m_stackWidget;
    QTreeView *m_treeView;
    QListWidget *m_searchList;
    QListWidget *m_sideBar;
    
    QFileSystemModel *m_model;
    QLineEdit *m_pathEdit;
    QLineEdit *m_searchEdit;
    QToolBar *m_toolBar;
    
    QString m_copyPath;
    bool m_isCut;
};
