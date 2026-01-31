#pragma once

#include <QTreeView>

class FileListWidget : public QTreeView {
    Q_OBJECT
public:
    explicit FileListWidget(QWidget *parent = nullptr);
};
