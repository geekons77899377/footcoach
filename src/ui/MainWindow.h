#pragma once

#include <QMainWindow>

class Database;
class QLabel;
class QListWidget;
class QMenu;
class QPushButton;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Database &db, QWidget *parent = nullptr);

private:
    void addPage(const QString &title, QWidget *page);
    void showPage(int index);

    Database &m_db;
    bool m_compact = false;
    QListWidget *m_nav = nullptr;          // disposition PC : liste à gauche
    QPushButton *m_menuButton = nullptr;   // disposition téléphone : bouton « Menu » en haut
    QLabel *m_title = nullptr;
    QMenu *m_menu = nullptr;
    QStackedWidget *m_pages = nullptr;
};
