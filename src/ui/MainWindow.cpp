#include "ui/MainWindow.h"
#include <QHeaderView>
#include <QDir>
#include <QStandardPaths>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QSplitter>
#include <QLabel>
#include <QToolButton>
#include <QCoreApplication>
#include <QFile>

static QString getAssetPath(const QString &name) {
    // Try executable path first (deployment friendly)
    QString path = QCoreApplication::applicationDirPath() + "/assets/" + name;
    if (QFile::exists(path)) return path;
    
    // Fallback to relative (development)
    return "assets/" + name;
}

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent), m_engine(new FileSystemEngine(this)), m_isCut(false) {
    applyModernStyle();
    setupUI();
    setupActions();
    setupShortcuts();
    
    // Icon
    setWindowIcon(QIcon(getAssetPath("logo.png")));

    goHome();
}

void MainWindow::applyModernStyle() {
    // Sharp Dark Theme
    // Main: #1A1A1A, Panels: #202020, Inputs: #2D2D2D
    // Borders: #333333 (Defining structure)
    // Radius: 2px (Sharp/Modern)
    
    this->setStyleSheet(R"(
        QMainWindow { background-color: #1A1A1A; color: #FFFFFF; }
        
        QWidget { 
            background-color: #1A1A1A; 
            color: #FFFFFF; 
            font-family: 'Inter', 'Segoe UI', sans-serif; 
            font-size: 14px; 
            font-weight: bold;
        }
        
        QLineEdit { 
            background-color: #2D2D2D; 
            border: 1px solid #333333; 
            border-radius: 2px; 
            padding: 6px; 
            color: #FFFFFF;
            selection-background-color: #005FB8;
            selection-color: #FFFFFF;
            font-weight: bold;
        }
        QLineEdit:focus { border: 1px solid #005FB8; background-color: #202020; }
        
        QTreeView { 
            background-color: #1A1A1A; 
            border: 1px solid #333333; 
            outline: none;
        }
        QTreeView::item { padding: 4px; border-radius: 2px; border: none; }
        QTreeView::item:hover { background-color: #2A2A2A; }
        QTreeView::item:selected { background-color: #005FB8; color: #FFFFFF; }
        
        QHeaderView::section {
            background-color: #202020;
            color: #FFFFFF;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #333333;
            border-right: 1px solid #333333;
            font-weight: bold;
        }
        
        QListWidget { background-color: #1A1A1A; border: 1px solid #333333; outline: none; }
        QListWidget::item { padding: 6px; border-radius: 2px; margin: 2px; }
        QListWidget::item:hover { background-color: #2A2A2A; }
        QListWidget::item:selected { background-color: #005FB8; color: #FFFFFF; }
        
        QToolBar { 
            background-color: #202020; 
            border-bottom: 1px solid #333333; 
            padding: 6px; 
            spacing: 8px;
        }
        QToolButton { 
            background-color: transparent; 
            border: 1px solid transparent;
            border-radius: 2px; 
            padding: 4px 10px;
            font-weight: bold;
        }
        QToolButton:hover { background-color: #333333; border: 1px solid #444444; }
        QToolButton:pressed { background-color: #000000; }
        QToolButton:checked { background-color: #005FB8; color: #FFFFFF; }
        
        QSplitter::handle { background-color: #333333; width: 1px; }
        
        QScrollBar:vertical {
            background-color: #1A1A1A;
            width: 12px;
            margin: 0px;
            border-left: 1px solid #333333;
        }
        QScrollBar::handle:vertical {
            background-color: #444444;
            min-height: 20px;
            border-radius: 2px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover { background-color: #666666; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");
}

class SearchResultItem : public QListWidgetItem {
public:
    SearchResultItem(const FileInfo &info, QListWidget *parent) 
        : QListWidgetItem(parent), m_info(info) {
        setText(info.absolutePath);
        setData(Qt::UserRole, info.absolutePath);
        setToolTip(info.isDir ? "Directory" : "File");
        setIcon(QIcon(getAssetPath(info.isDir ? "ic_folder.png" : "ic_file.png")));
    }
    
    bool operator<(const QListWidgetItem &other) const override {
        // Sort descending by score
        const SearchResultItem *o = static_cast<const SearchResultItem*>(&other);
        return m_info.score < o->m_info.score; // Default is ascending, we want descending sorting usually, but QListWidget sorts ascending by default. 
        // Logic: if current < other, current comes first? No. 
        // We will setSortOrder(Qt::DescendingOrder). So larger score should be "greater".
        // Therefore: return m_info.score < o->m_info.score is correct for numeric comparison.
    }
    
private:
    FileInfo m_info;
};

void MainWindow::setupUI() {
    setWindowTitle("Raefile - File Manager");
    resize(1100, 750);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); 
    mainLayout->setSpacing(0);

    // Top Bar (Nav + Path + Search)
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(10);
    
    // Up Button
    QToolButton *upBtn = new QToolButton(this);
    upBtn->setIcon(QIcon(getAssetPath("ic_arrow_up.png")));
    upBtn->setToolTip("Up");
    upBtn->setFixedSize(32, 32); // Ensure size
    upBtn->setIconSize(QSize(20, 20));
    upBtn->setStyleSheet("border: 1px solid #333333; border-radius: 2px; background-color: #2D2D2D;");
    connect(upBtn, &QToolButton::clicked, this, &MainWindow::goUp);
    
    // Refresh Button
    QToolButton *refreshBtn = new QToolButton(this);
    refreshBtn->setIcon(QIcon(getAssetPath("ic_refresh.png")));
    refreshBtn->setToolTip("Refresh");
    refreshBtn->setFixedSize(32, 32); // Ensure size
    refreshBtn->setIconSize(QSize(20, 20));
    refreshBtn->setStyleSheet("border: 1px solid #333333; border-radius: 2px; background-color: #2D2D2D;");
    connect(refreshBtn, &QToolButton::clicked, this, &MainWindow::refreshView);
    
    topLayout->addWidget(upBtn);
    topLayout->addWidget(refreshBtn);
    
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText("Path...");
    
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Global Search...");
    
    topLayout->addWidget(m_pathEdit, 7);
    topLayout->addWidget(m_searchEdit, 3);
    
    mainLayout->addLayout(topLayout);

    connect(m_pathEdit, &QLineEdit::returnPressed, [this]() {
        onDirectoryLoaded(m_pathEdit->text());
    });
    
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::startGlobalSearch);

    // Splitter for Sidebar | Content
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    
    // Sidebar Container
    QWidget *sidebarContainer = new QWidget(this);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebarContainer);
    sidebarLayout->setContentsMargins(10, 10, 10, 10);
    sidebarLayout->setSpacing(10);
    
    // Logo Removed as requested
    
    // Sidebar List
    setupSidebar();
    sidebarLayout->addWidget(m_sideBar);
    
    splitter->addWidget(sidebarContainer);

    // Right Content (Stacked -> Tree / Search)
    QWidget *rightContent = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContent);
    rightLayout->setContentsMargins(0,0,0,0);
    
    m_stackWidget = new QStackedWidget(this);
    
    // Page 0: Tree View
    m_treeView = new QTreeView(this);
    m_model = new QFileSystemModel(this);
    m_model->setRootPath("");
    m_treeView->setModel(m_model);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->setAlternatingRowColors(false);
    m_treeView->setIconSize(QSize(24, 24));
    
    // Set column widths (Name = 2x others)
    // Name (0), Size (1), Type (2), Date (3)
    m_treeView->setColumnWidth(0, 400); // Name
    m_treeView->setColumnWidth(1, 120); // Size
    m_treeView->setColumnWidth(2, 120); // Type
    m_treeView->setColumnWidth(3, 200); // Date modified
    
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(m_treeView, &QTreeView::doubleClicked, this, &MainWindow::onFileDoubleClicked);
    
    // Page 1: Search List
    m_searchList = new QListWidget(this);
    connect(m_searchList, &QListWidget::itemDoubleClicked, this, &MainWindow::onSearchResultClicked);
    
    m_stackWidget->addWidget(m_treeView);
    m_stackWidget->addWidget(m_searchList);
    
    rightLayout->addWidget(m_stackWidget);
    
    // Branding (Moved to Right Panel to fix bottom gap)
    QLabel *brandLabel = new QLabel("raef", this);
    brandLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    brandLabel->setStyleSheet("color: rgba(255, 255, 255, 0.3); font-size: 12px; margin: 4px; font-weight: bold;");
    rightLayout->addWidget(brandLabel);
    
    splitter->addWidget(rightContent);
    splitter->setStretchFactor(1, 4); // Logic: Content is wider than sidebar
    
    // Branding Label (Already created above in rightLayout)
    // Branding Label (Already created above in rightLayout)
    // mainLayout was created at the top, just use it
    mainLayout->addWidget(splitter);
    // mainLayout->addWidget(brandLabel); // Removed from here to fix gap
    
    setCentralWidget(centralWidget);
    // statusBar()->showMessage("Ready"); // Removed to avoid gap
}

void MainWindow::setupSidebar() {
    m_sideBar = new QListWidget(this);
    m_sideBar->setFixedWidth(240);
    m_sideBar->setIconSize(QSize(32, 32));
    
    auto addItem = [this](const QString &name, const QString &iconName) {
        QListWidgetItem *item = new QListWidgetItem(QIcon(getAssetPath(iconName)), name);
        m_sideBar->addItem(item);
    };
    
    addItem("Home", "ic_home.png");
    addItem("Desktop", "ic_desktop.png");
    addItem("Root", "ic_root.png");
    addItem("Documents", "ic_folder.png");
    addItem("Downloads", "ic_folder.png");
    addItem("Pictures", "ic_media.png");
    addItem("Music", "ic_media.png");
    addItem("Videos", "ic_media.png");
    
    connect(m_sideBar, &QListWidget::itemClicked, this, &MainWindow::onSideBarClicked);
}

void MainWindow::onSideBarClicked(QListWidgetItem *item) {
    if (!item) return;
    QString txt = item->text();
    QString target;
    
    if (txt == "Home") target = QDir::homePath();
    else if (txt == "Root") target = "/";
    else if (txt == "Desktop") target = QDir::home().filePath("Desktop");
    else if (txt == "Documents") target = QDir::home().filePath("Documents");
    else if (txt == "Downloads") target = QDir::home().filePath("Downloads");
    else if (txt == "Pictures") target = QDir::home().filePath("Pictures");
    else if (txt == "Music") target = QDir::home().filePath("Music");
    else if (txt == "Videos") target = QDir::home().filePath("Videos");
    else target = QDir::homePath();
    
    onDirectoryLoaded(target);
}

void MainWindow::setupActions() {
    m_toolBar = addToolBar("Main");
    m_toolBar->addAction("Home", this, &MainWindow::goHome);
    m_toolBar->addAction("Root", this, &MainWindow::goRoot);
    // Removed "Up" and "Refresh" from this toolbar as they are now in the navBar
    m_toolBar->addSeparator();
    QAction *hiddenAct = m_toolBar->addAction("Hidden Files", this, &MainWindow::toggleHiddenFiles);
    hiddenAct->setCheckable(true);
    
    m_toolBar->addSeparator();
    m_toolBar->addAction("New Folder", this, &MainWindow::createNewFolder);
    m_toolBar->addAction("New File", this, &MainWindow::createNewFile);
    // Removed File Ops from Toolbar as per request
}

void MainWindow::setupShortcuts() {
    // Basic shortcuts
    QAction *copyAct = new QAction(this);
    copyAct->setShortcut(QKeySequence::Copy);
    connect(copyAct, &QAction::triggered, this, &MainWindow::copySelected);
    addAction(copyAct);

    QAction *pasteAct = new QAction(this);
    pasteAct->setShortcut(QKeySequence::Paste);
    connect(pasteAct, &QAction::triggered, this, &MainWindow::pasteToCurrent);
    addAction(pasteAct);

    QAction *delAct = new QAction(this);
    delAct->setShortcut(QKeySequence::Delete);
    connect(delAct, &QAction::triggered, this, &MainWindow::deleteSelected);
    addAction(delAct);

    QAction *cutAct = new QAction(this);
    cutAct->setShortcut(QKeySequence::Cut);
    connect(cutAct, &QAction::triggered, this, &MainWindow::cutSelected);
    addAction(cutAct);
    
    QAction *refreshAct = new QAction(this);
    refreshAct->setShortcut(QKeySequence::Refresh);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::refreshView);
    addAction(refreshAct);
}

void MainWindow::goHome() {
    onDirectoryLoaded(QDir::homePath());
}

void MainWindow::goRoot() {
    onDirectoryLoaded("/");
}

void MainWindow::goUp() {
    QDir current(m_pathEdit->text());
    if (current.cdUp()) {
        onDirectoryLoaded(current.absolutePath());
    }
}

void MainWindow::refreshView() {
    onDirectoryLoaded(m_pathEdit->text());
}

void MainWindow::onDirectoryLoaded(const QString &path) {
    if (QDir(path).exists()) {
        m_stackWidget->setCurrentIndex(0); // Show Tree View
        m_pathEdit->setText(path);
        m_treeView->setRootIndex(m_model->index(path));
    }
}

void MainWindow::onFileDoubleClicked(const QModelIndex &index) {
    if (m_model->isDir(index)) {
        onDirectoryLoaded(m_model->filePath(index));
    }
}

void MainWindow::createNewFolder() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Folder", "Name:", QLineEdit::Normal, "New Folder", &ok);
    if (ok && !name.isEmpty()) {
        QString path = QDir(m_pathEdit->text()).filePath(name);
        if (!m_engine->createFolder(path)) {
            QMessageBox::warning(this, "Error", "Could not create folder.");
        }
    }
}

void MainWindow::createNewFile() {
    bool ok;
    QString name = QInputDialog::getText(this, "New File", "Name:", QLineEdit::Normal, "new_file.txt", &ok);
    if (ok && !name.isEmpty()) {
        QString path = QDir(m_pathEdit->text()).filePath(name);
        if (!m_engine->createFile(path)) {
            QMessageBox::warning(this, "Error", "Could not create file.");
        }
    }
}

void MainWindow::deleteSelected() {
    auto indexes = m_treeView->selectionModel()->selectedRows();
    if (indexes.isEmpty()) return;

    auto reply = QMessageBox::question(this, "Delete", "Are you sure you want to delete selected items?", 
                                     QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        for (const auto &index : indexes) {
            m_engine->remove(m_model->filePath(index), true);
        }
    }
}

void MainWindow::renameSelected() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    bool ok;
    QString oldName = m_model->fileName(index);
    QString newName = QInputDialog::getText(this, "Rename", "New Name:", QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty() && newName != oldName) {
        if (!m_engine->rename(m_model->filePath(index), newName)) {
            QMessageBox::warning(this, "Error", "Could not rename.");
        }
    }
}

void MainWindow::copySelected() {
    auto index = m_treeView->currentIndex();
    if (index.isValid()) {
        m_copyPath = m_model->filePath(index);
        m_isCut = false;
        statusBar()->showMessage("Copied: " + m_model->fileName(index));
    }
}

void MainWindow::cutSelected() {
    auto index = m_treeView->currentIndex();
    if (index.isValid()) {
        m_copyPath = m_model->filePath(index);
        m_isCut = true;
        statusBar()->showMessage("Cut: " + m_model->fileName(index));
    }
}

void MainWindow::pasteToCurrent() {
    if (m_copyPath.isEmpty()) return;
    
    QString dest = QDir(m_pathEdit->text()).filePath(QFileInfo(m_copyPath).fileName());
    bool success = false;
    if (m_isCut) {
        success = m_engine->move(m_copyPath, dest);
        m_copyPath = ""; // Clear after move
    } else {
        success = m_engine->copy(m_copyPath, dest);
    }
    
    if (!success) {
        QMessageBox::warning(this, "Error", "Paste operation failed.");
    }
}

void MainWindow::showContextMenu(const QPoint &pos) {
    QMenu menu(this);
    // Using ic_file.png as a generic icon for actions since we don't have dedicated edit icons yet, 
    // or we can stick to no icons for actions to keep it minimal, or use system icons as fallback?
    // User asked for "replace system icons with icons u like". I will use the custom ones where possible.
    // For actions, to be consistent, I'll use the file/folder icons or no icons if none match well.
    // Let's use simple QMenu without icons for actions to keep it minimal "Bold White" text style, 
    // OR we can simple text. Most "Dark" themes rely on text.
    // However, I will use ic_file.png for file ops just to show "custom" nature.
    
    menu.addAction(QIcon(getAssetPath("ic_file.png")), "Rename", this, &MainWindow::renameSelected);
    menu.addAction(QIcon(getAssetPath("ic_file.png")), "Copy", this, &MainWindow::copySelected);
    menu.addAction(QIcon(getAssetPath("ic_file.png")), "Cut", this, &MainWindow::cutSelected);
    menu.addAction(QIcon(getAssetPath("ic_file.png")), "Paste", this, &MainWindow::pasteToCurrent);
    menu.addAction(QIcon(getAssetPath("ic_file.png")), "Delete", this, &MainWindow::deleteSelected);
    menu.addSeparator();
    menu.addAction(QIcon(getAssetPath("ic_folder.png")), "New Folder", this, &MainWindow::createNewFolder);
    menu.addAction(QIcon(getAssetPath("ic_file.png")), "New File", this, &MainWindow::createNewFile);
    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void MainWindow::toggleHiddenFiles() {
    if (m_model->filter() & QDir::Hidden) {
        m_model->setFilter(m_model->filter() & ~QDir::Hidden);
    } else {
        m_model->setFilter(m_model->filter() | QDir::Hidden);
    }
}

void MainWindow::startGlobalSearch() {
    QString query = m_searchEdit->text();
    if (query.isEmpty()) return;

    m_searchList->clear();
    m_stackWidget->setCurrentIndex(1); // Switch to list view
    statusBar()->showMessage("Searching global filesystem... (This may take a while)");
    
    // Stop any existing search
    m_engine->stopSearch();
    
    // Start new search
    m_engine->searchAsync(query, [this](const FileInfo &info) {
        // UI updates must be on main thread
        QMetaObject::invokeMethod(this, [this, info]() {
            // Use custom item for sorting
            SearchResultItem *item = new SearchResultItem(info, m_searchList);
            // Sorting triggers automatically if sorting is enabled
            m_searchList->sortItems(Qt::DescendingOrder); 
        });
    });
}

void MainWindow::onSearchResultClicked(QListWidgetItem *item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    QFileInfo info(path);
    if (info.isDir()) {
        onDirectoryLoaded(path);
    } else {
        onDirectoryLoaded(info.path());
        // Ideally select the file too
        QModelIndex index = m_model->index(path);
        m_treeView->setCurrentIndex(index);
        m_treeView->scrollTo(index);
    }
}
