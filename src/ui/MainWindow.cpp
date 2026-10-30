#include "MainWindow.h"

#include "core/Database.h"
#include "ui/AttendancePage.h"
#include "ui/CalendarPage.h"
#include "ui/ExercisesPage.h"
#include "ui/Platform.h"
#include "ui/SessionsPage.h"
#include "ui/SquadPage.h"
#include "ui/TacticsPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(Database &db, QWidget *parent)
    : QMainWindow(parent)
    , m_db(db)
    , m_compact(Platform::compactLayout())
{
    setWindowTitle(tr("FootCoach"));
    m_pages = new QStackedWidget;
    auto *central = new QWidget;

    if (m_compact) {
        // Téléphone : une barre en haut (bouton « Menu » + titre de la page) et la page en dessous.
        resize(390, 800);   // sans effet sur Android (plein écran), utile pour l'essai sur PC
        m_menu = new QMenu(this);
        m_menuButton = new QPushButton(tr("Menu"));
        m_menuButton->setMenu(m_menu);
        m_title = new QLabel;
        m_title->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 18px;"));

        auto *bar = new QHBoxLayout;
        bar->setContentsMargins(8, 8, 8, 4);
        bar->addWidget(m_menuButton);
        bar->addWidget(m_title, 1);

        auto *layout = new QVBoxLayout(central);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addLayout(bar);
        layout->addWidget(m_pages, 1);
    } else {
        resize(1100, 700);
        m_nav = new QListWidget;
        m_nav->setFixedWidth(180);
        auto *layout = new QHBoxLayout(central);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_nav);
        layout->addWidget(m_pages, 1);
    }
    setCentralWidget(central);

    addPage(tr("Effectif"), new SquadPage(m_db.handle()));
    addPage(tr("Calendrier"), new CalendarPage(m_db.handle()));
    addPage(tr("Présences"), new AttendancePage(m_db.handle()));
    addPage(tr("Exercices"), new ExercisesPage(m_db.handle()));
    addPage(tr("Séances"), new SessionsPage(m_db.handle()));
    addPage(tr("Tactique"), new TacticsPage(m_db.handle()));

    if (m_nav) {
        connect(m_nav, &QListWidget::currentRowChanged, this, &MainWindow::showPage);
        m_nav->setCurrentRow(0);
        statusBar()->showMessage(tr("Base : %1  |  schéma v%2")
                                     .arg(m_db.path())
                                     .arg(m_db.schemaVersion()));
    } else {
        showPage(0);
        statusBar()->hide();   // pas de place pour le chemin de la base sur un téléphone
    }
}

void MainWindow::addPage(const QString &title, QWidget *page)
{
    const int index = m_pages->addWidget(page);
    if (m_nav)
        m_nav->addItem(title);
    if (m_menu) {
        QAction *action = m_menu->addAction(title);
        connect(action, &QAction::triggered, this, [this, index]() { showPage(index); });
    }
}

void MainWindow::showPage(int index)
{
    m_pages->setCurrentIndex(index);
    if (m_title && m_menu && index >= 0 && index < m_menu->actions().size())
        m_title->setText(m_menu->actions().at(index)->text());
}
