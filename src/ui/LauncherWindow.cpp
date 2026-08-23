#include "LauncherWindow.h"

#include "SearchResultModel.h"
#include "ResultDelegate.h"
#include "PreviewPane.h"
#include "../core/SearchCoordinator.h"
#include "../core/ConfigStore.h"
#include "../platform/DialogNavigator.h"
#include "../platform/ShellActions.h"
#include "../providers/FavoritesProvider.h"

#include <QApplication>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QFileInfo>
#include <QHash>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QHideEvent>
#include <QItemSelectionModel>
#include <QUrl>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QScreen>
#include <QShortcut>
#include <QCursor>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

namespace quickary {

LauncherWindow::LauncherWindow(SearchCoordinator* coordinator, QWidget* parent)
    : QWidget(parent), coordinator_(coordinator)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowTitle(QStringLiteral("Quickary"));
    resize(760, 430);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 10);
    root->setSpacing(8);

    search_ = new QLineEdit(this);
    search_->setObjectName(QStringLiteral("searchBox"));
    search_->setPlaceholderText(QStringLiteral("Search files, apps, commands, or the web…"));
    search_->setClearButtonEnabled(true);
    search_->setMinimumHeight(52);
    root->addWidget(search_);

    splitter_ = new QSplitter(Qt::Horizontal, this);
    results_ = new QListView(splitter_);
    results_->setObjectName(QStringLiteral("results"));
    results_->setUniformItemSizes(true);
    results_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    results_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    results_->setSelectionMode(QAbstractItemView::SingleSelection);
    results_->setMouseTracking(true);
    model_ = new SearchResultModel(results_);
    results_->setModel(model_);
    results_->setItemDelegate(new ResultDelegate(results_));
    results_->setContextMenuPolicy(Qt::CustomContextMenu);

    preview_ = new PreviewPane(splitter_);
    preview_->hide();
    splitter_->addWidget(results_);
    splitter_->addWidget(preview_);
    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 2);
    root->addWidget(splitter_, 1);

    status_ = new QLabel(QStringLiteral("Enter open   Ctrl+Enter reveal   Alt+P preview   Ctrl+O actions   Esc close"), this);
    status_->setObjectName(QStringLiteral("status"));
    root->addWidget(status_);

    debounce_ = new QTimer(this);
    debounce_->setSingleShot(true);
    debounce_->setInterval(18);

    connect(search_, &QLineEdit::textChanged, this, [this] { debounce_->start(); });
    connect(debounce_, &QTimer::timeout, this, &LauncherWindow::triggerSearch);
    connect(coordinator_, &SearchCoordinator::resultsChanged, this, &LauncherWindow::onResults);
    connect(results_, &QListView::doubleClicked, this, [this] { activateCurrent(); });
    connect(results_, &QListView::customContextMenuRequested, this, [this] { showActions(); });
    connect(results_->selectionModel(), &QItemSelectionModel::currentChanged, this, [this] { updatePreview(); });
    connect(search_, &QLineEdit::returnPressed, this, &LauncherWindow::activateCurrent);

    auto shortcut = [this](const QKeySequence& seq, auto fn) {
        auto* sc = new QShortcut(seq, this);
        sc->setContext(Qt::WindowShortcut);
        connect(sc, &QShortcut::activated, this, fn);
    };
    shortcut(QKeySequence(Qt::Key_Escape), [this] { hide(); });
    shortcut(QKeySequence(QStringLiteral("Alt+P")), [this] {
        previewVisible_ = !previewVisible_;
        preview_->setVisible(previewVisible_);
        updatePreview();
    });
    shortcut(QKeySequence(QStringLiteral("Ctrl+O")), [this] { showActions(); });
    shortcut(QKeySequence(QStringLiteral("Ctrl+Enter")), [this] {
        if (const auto* item = currentItem(); item && !item->path.isEmpty()) ShellActions::revealInFolder(item->path);
    });
    shortcut(QKeySequence(Qt::Key_F2), [this] { setDeepMode(!deep_); triggerSearch(); });
    shortcut(QKeySequence(QStringLiteral("Ctrl+N")), [this] {
        const int row = qBound(0, results_->currentIndex().row() + 1, qMax(0, model_->rowCount() - 1));
        results_->setCurrentIndex(model_->index(row, 0));
    });
    shortcut(QKeySequence(QStringLiteral("Ctrl+P")), [this] {
        const int row = qBound(0, results_->currentIndex().row() - 1, qMax(0, model_->rowCount() - 1));
        results_->setCurrentIndex(model_->index(row, 0));
    });

    applyTheme();
}

void LauncherWindow::applyTheme()
{
    setStyleSheet(QStringLiteral(R"(
        QWidget { background: #17191d; color: #f2f3f5; font-family: "Segoe UI", "Microsoft YaHei UI"; font-size: 10.5pt; }
        QWidget#QuickaryPanel { border: 1px solid #2a2e34; }
        QLineEdit#searchBox { background: #22262c; border: 1px solid #343a42; border-radius: 12px; padding: 0 16px; font-size: 15pt; selection-background-color: #5d78ff; }
        QLineEdit#searchBox:focus { border: 1px solid #6f86ff; }
        QListView#results { background: transparent; border: 0; outline: 0; padding: 2px; }
        QLabel#status { color: #8f98a5; padding: 2px 6px; font-size: 9pt; }
        QSplitter::handle { background: #252a30; width: 1px; }
        QPlainTextEdit { background: #111317; color: #dce1e8; border-radius: 8px; padding: 8px; }
    )"));
}

void LauncherWindow::summon(bool deepSearch)
{
    const bool wasVisible = isVisible();
    currentFolder_ = DialogNavigator::activeExplorerFolder();
    setDeepMode(deepSearch);
    if (!wasVisible) search_->clear();

    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect area = screen->availableGeometry();
    move(area.center().x() - width() / 2, area.top() + qMax(80, area.height() / 6));
    show();
    raise();
    activateWindow();
    search_->setFocus(Qt::ShortcutFocusReason);
    search_->selectAll();
    triggerSearch();
}

void LauncherWindow::typeFromExplorer(const QString& text)
{
    if (!isVisible()) {
        summon(true);
        search_->setText(text);
    } else {
        search_->insert(text);
    }
}

void LauncherWindow::setDeepMode(bool deep)
{
    deep_ = deep;
    resize(deep ? 1060 : 760, deep ? 690 : 430);
    results_->setSelectionMode(deep ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
    status_->setText(deep
        ? QStringLiteral("Deep Search · multi-select enabled · filters: folder: file: doc: pic: video: audio: ext:pdf date:week size:>10mb")
        : QStringLiteral("Launcher · Enter open   Ctrl+Enter reveal   Alt+P preview   Ctrl+O actions   Esc close"));
}

void LauncherWindow::triggerSearch()
{
    coordinator_->search(search_->text(), currentFolder_, deep_);
}

void LauncherWindow::onResults(const QVector<SearchItem>& items, quint64)
{
    model_->setItems(items);
    if (!items.isEmpty()) results_->setCurrentIndex(model_->index(0, 0));
}

const SearchItem* LauncherWindow::currentItem() const
{
    return model_->itemAt(results_->currentIndex().row());
}

QVector<SearchItem> LauncherWindow::selectedItems() const
{
    QVector<SearchItem> selected;
    const QModelIndexList indexes = results_->selectionModel()->selectedIndexes();
    selected.reserve(indexes.size());
    for (const QModelIndex& index : indexes) {
        if (const auto* item = model_->itemAt(index.row())) selected.push_back(*item);
    }
    if (selected.isEmpty()) {
        if (const auto* item = currentItem()) selected.push_back(*item);
    }
    return selected;
}

void LauncherWindow::activateCurrent()
{
    if (const auto* item = currentItem()) activate(*item);
}

void LauncherWindow::activate(const SearchItem& item)
{
    coordinator_->recordActivation(item);
    bool handled = false;
    if (item.kind == ItemKind::Web) {
        handled = QDesktopServices::openUrl(QUrl(item.meta.value(QStringLiteral("url")).toString()));
    } else if (item.kind == ItemKind::Command) {
        handled = ShellActions::executeCommand(item, this);
    } else if ((item.kind == ItemKind::Folder || item.kind == ItemKind::Favorite)
               && DialogNavigator::jumpActiveFileDialogTo(item.path)) {
        handled = true;
    } else if (!item.path.isEmpty()) {
        handled = ShellActions::openPath(item.path);
    }
    if (handled && !deep_) hide();
}

void LauncherWindow::showActions()
{
    const QVector<SearchItem> selected = selectedItems();
    if (selected.isEmpty()) return;
    const SearchItem& primary = selected.front();

    QStringList paths;
    for (const auto& item : selected) {
        if (!item.path.isEmpty()) paths << item.path;
    }
    const bool batch = selected.size() > 1;

    QMenu menu(this);
    QAction* open = menu.addAction(batch ? QStringLiteral("Open selected (%1)").arg(selected.size()) : QStringLiteral("Open"));
    QAction* reveal = nullptr;
    QAction* copyPath = nullptr;
    QAction* copy = nullptr;
    QAction* cut = nullptr;
    QAction* pin = nullptr;
    QAction* trash = nullptr;
    if (!paths.isEmpty()) {
        if (!batch) reveal = menu.addAction(QStringLiteral("Open containing folder"));
        copyPath = menu.addAction(batch ? QStringLiteral("Copy paths (%1)").arg(paths.size()) : QStringLiteral("Copy path"));
        copy = menu.addAction(batch ? QStringLiteral("Copy selected (%1)").arg(paths.size()) : QStringLiteral("Copy"));
        cut = menu.addAction(batch ? QStringLiteral("Cut selected (%1)").arg(paths.size()) : QStringLiteral("Cut"));
        if (!batch) pin = menu.addAction(FavoritesProvider::isFavorite(primary.path) ? QStringLiteral("Unpin favorite") : QStringLiteral("Pin favorite"));
    }

    QHash<QAction*, CustomAction> customActions;
    if (!paths.isEmpty()) {
        const QString suffix = QFileInfo(primary.path).suffix().toCaseFolded();
        for (const auto& action : ConfigStore::instance().actions()) {
            bool applies = action.extensions.isEmpty() || action.extensions.contains(QStringLiteral("*"));
            if (!applies) {
                for (const QString& ext : action.extensions) {
                    if (ext.compare(suffix, Qt::CaseInsensitive) == 0) { applies = true; break; }
                    if (ext == QStringLiteral("<folder>") && QFileInfo(primary.path).isDir()) { applies = true; break; }
                }
            }
            if (!applies) continue;
            QAction* a = menu.addAction(action.name);
            customActions.insert(a, action);
        }
        menu.addSeparator();
        trash = menu.addAction(batch ? QStringLiteral("Move selected to Recycle Bin (%1)").arg(paths.size())
                                     : QStringLiteral("Move to Recycle Bin"));
    }
    QAction* chosen = menu.exec(QCursor::pos());
    if (!chosen) return;

    if (chosen == open) {
        if (!batch) activate(primary);
        else for (const auto& item : selected) { if (!item.path.isEmpty()) ShellActions::openPath(item.path); }
    } else if (chosen == reveal) {
        ShellActions::revealInFolder(primary.path);
    } else if (chosen == copyPath) {
        ShellActions::copyPaths(paths);
    } else if (chosen == copy) {
        ShellActions::copyFiles(paths, false);
    } else if (chosen == cut) {
        ShellActions::copyFiles(paths, true);
    } else if (chosen == pin) {
        FavoritesProvider::setFavorite(primary.path, !FavoritesProvider::isFavorite(primary.path));
        triggerSearch();
    } else if (customActions.contains(chosen)) {
        const CustomAction action = customActions.value(chosen);
        for (const QString& path : paths) {
            ShellActions::executeTemplate(action.program, action.arguments, path, currentFolder_, search_->text(), action.admin);
        }
    } else if (chosen == trash) {
        for (const QString& path : paths) ShellActions::recycle(path);
        triggerSearch();
    }
}

void LauncherWindow::updatePreview()
{
    if (!previewVisible_) return;
    if (const auto* item = currentItem()) preview_->preview(item->path);
}

void LauncherWindow::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) { hide(); return; }
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (e->modifiers() & Qt::ControlModifier) {
            if (const auto* item = currentItem(); item && !item->path.isEmpty()) ShellActions::revealInFolder(item->path);
        } else activateCurrent();
        return;
    }
    if ((e->modifiers() & Qt::AltModifier) && e->key() == Qt::Key_P) {
        previewVisible_ = !previewVisible_;
        preview_->setVisible(previewVisible_);
        updatePreview();
        return;
    }
    if ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_O) { showActions(); return; }
    if (e->key() == Qt::Key_Right && !(e->modifiers() & Qt::ControlModifier)) { showActions(); return; }
    if (e->key() == Qt::Key_F2) { setDeepMode(!deep_); triggerSearch(); return; }
    if ((e->modifiers() & Qt::ControlModifier) && (e->key() == Qt::Key_N || e->key() == Qt::Key_P)) {
        int row = results_->currentIndex().row();
        row += (e->key() == Qt::Key_N) ? 1 : -1;
        row = qBound(0, row, qMax(0, model_->rowCount() - 1));
        results_->setCurrentIndex(model_->index(row, 0));
        return;
    }
    QWidget::keyPressEvent(e);
}

void LauncherWindow::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    previewVisible_ = false;
    preview_->hide();
}

} // namespace quickary
