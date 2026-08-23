#include "LauncherWindow.h"

#include "PreviewPane.h"
#include "ResultDelegate.h"
#include "ResultFilterProxy.h"
#include "SearchResultModel.h"
#include "../core/AppSettings.h"
#include "../core/ConfigStore.h"
#include "../core/SearchCoordinator.h"
#include "../platform/DialogNavigator.h"
#include "../platform/ShellActions.h"
#include "../providers/FavoritesProvider.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHeaderView>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPalette>
#include <QScreen>
#include <QSet>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

namespace quickary {

LauncherWindow::LauncherWindow(SearchCoordinator* coordinator, QWidget* parent)
    : QWidget(parent), coordinator_(coordinator)
{
    setObjectName(QStringLiteral("QuickaryPanel"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowTitle(QStringLiteral("Quickary"));
    resize(760, 430);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 10);
    root->setSpacing(8);

    search_ = new QLineEdit(this);
    search_->setObjectName(QStringLiteral("searchBox"));
    search_->setPlaceholderText(QStringLiteral("Search files, apps, commands, content, or the web…"));
    search_->setClearButtonEnabled(true);
    search_->setMinimumHeight(52);
    root->addWidget(search_);

    splitter_ = new QSplitter(Qt::Horizontal, this);
    auto* resultPane = new QWidget(splitter_);
    resultPane->setObjectName(QStringLiteral("resultPane"));
    auto* resultLayout = new QVBoxLayout(resultPane);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(6);

    facetBar_ = new QWidget(resultPane);
    facetBar_->setObjectName(QStringLiteral("facetBar"));
    auto* facets = new QHBoxLayout(facetBar_);
    facets->setContentsMargins(4, 0, 4, 0);
    facets->setSpacing(6);
    auto* typeLabel = new QLabel(QStringLiteral("Type"), facetBar_);
    typeLabel->setObjectName(QStringLiteral("facetLabel"));
    facets->addWidget(typeLabel);

    allFacet_ = new QToolButton(facetBar_);
    allFacet_->setCheckable(true);
    allFacet_->setChecked(true);
    allFacet_->setProperty("baseText", QStringLiteral("All"));
    allFacet_->setText(QStringLiteral("All"));
    facets->addWidget(allFacet_);

    const struct FacetSpec { ItemKind kind; const char* label; } facetSpecs[] = {
        {ItemKind::File, "Files"},
        {ItemKind::Folder, "Folders"},
        {ItemKind::Application, "Apps"},
        {ItemKind::Favorite, "Favorites"},
        {ItemKind::Command, "Commands"},
        {ItemKind::Web, "Web"},
    };
    for (const auto& spec : facetSpecs) {
        auto* button = new QToolButton(facetBar_);
        button->setCheckable(true);
        button->setProperty("baseText", QLatin1String(spec.label));
        button->setText(QLatin1String(spec.label));
        facetButtons_.insert(static_cast<int>(spec.kind), button);
        facets->addWidget(button);
        connect(button, &QToolButton::toggled, this, &LauncherWindow::updateFacetFilter);
    }
    facets->addStretch(1);
    resultLayout->addWidget(facetBar_);

    model_ = new SearchResultModel(this);
    proxy_ = new ResultFilterProxy(this);
    proxy_->setSourceModel(model_);

    resultStack_ = new QStackedWidget(resultPane);
    launcherResults_ = new QListView(resultStack_);
    launcherResults_->setObjectName(QStringLiteral("results"));
    launcherResults_->setUniformItemSizes(true);
    launcherResults_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    launcherResults_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    launcherResults_->setSelectionMode(QAbstractItemView::SingleSelection);
    launcherResults_->setMouseTracking(true);
    launcherResults_->setModel(proxy_);
    launcherResults_->setItemDelegate(new ResultDelegate(launcherResults_));
    launcherResults_->setContextMenuPolicy(Qt::CustomContextMenu);

    deepResults_ = new QTableView(resultStack_);
    deepResults_->setObjectName(QStringLiteral("deepResults"));
    deepResults_->setModel(proxy_);
    deepResults_->setSelectionBehavior(QAbstractItemView::SelectRows);
    deepResults_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    deepResults_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    deepResults_->setShowGrid(false);
    deepResults_->setAlternatingRowColors(true);
    deepResults_->setContextMenuPolicy(Qt::CustomContextMenu);
    deepResults_->setSortingEnabled(true);
    proxy_->sort(-1);
    deepResults_->verticalHeader()->setVisible(false);
    deepResults_->horizontalHeader()->setHighlightSections(false);
    deepResults_->horizontalHeader()->setStretchLastSection(true);
    deepResults_->setColumnWidth(SearchResultModel::NameColumn, 260);
    deepResults_->setColumnWidth(SearchResultModel::PathColumn, 420);
    deepResults_->setColumnWidth(SearchResultModel::TypeColumn, 110);
    deepResults_->setColumnWidth(SearchResultModel::SizeColumn, 100);
    deepResults_->setColumnWidth(SearchResultModel::ModifiedColumn, 170);

    resultStack_->addWidget(launcherResults_);
    resultStack_->addWidget(deepResults_);
    resultLayout->addWidget(resultStack_, 1);

    preview_ = new PreviewPane(splitter_);
    preview_->hide();
    splitter_->addWidget(resultPane);
    splitter_->addWidget(preview_);
    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 2);
    root->addWidget(splitter_, 1);

    status_ = new QLabel(this);
    status_->setObjectName(QStringLiteral("status"));
    root->addWidget(status_);

    debounce_ = new QTimer(this);
    debounce_->setSingleShot(true);
    debounce_->setInterval(18);

    connect(search_, &QLineEdit::textChanged, this, [this] { debounce_->start(); });
    connect(debounce_, &QTimer::timeout, this, &LauncherWindow::triggerSearch);
    connect(coordinator_, &SearchCoordinator::resultsChanged, this, &LauncherWindow::onResults);

    const auto connectView = [this](QAbstractItemView* view) {
        connect(view, &QAbstractItemView::doubleClicked, this, [this] { activateCurrent(); });
        connect(view, &QWidget::customContextMenuRequested, this, [this] { showActions(); });
        connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this] { updatePreview(); });
    };
    connectView(launcherResults_);
    connectView(deepResults_);
    connect(search_, &QLineEdit::returnPressed, this, &LauncherWindow::activateCurrent);

    connect(allFacet_, &QToolButton::clicked, this, [this] {
        {
            QSignalBlocker block(allFacet_);
            allFacet_->setChecked(true);
        }
        for (auto* button : facetButtons_) {
            QSignalBlocker block(button);
            button->setChecked(false);
        }
        proxy_->setKindFilters({});
        selectFirstResult();
    });

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
    shortcut(QKeySequence(QStringLiteral("Ctrl+,")), [this] { emit settingsRequested(); });
    shortcut(QKeySequence(QStringLiteral("Ctrl+Enter")), [this] {
        if (const auto* item = currentItem(); item && !item->path.isEmpty()) ShellActions::revealInFolder(item->path);
    });
    shortcut(QKeySequence(Qt::Key_F2), [this] { setDeepMode(!deep_); triggerSearch(); });
    shortcut(QKeySequence(QStringLiteral("Ctrl+N")), [this] { moveSelection(1); });
    shortcut(QKeySequence(QStringLiteral("Ctrl+P")), [this] { moveSelection(-1); });

    connect(&AppSettings::instance(), &AppSettings::changed, this, &LauncherWindow::applySettings);
    setDeepMode(false);
    applyTheme();
}

QAbstractItemView* LauncherWindow::activeResults() const
{
    return deep_ ? static_cast<QAbstractItemView*>(deepResults_)
                 : static_cast<QAbstractItemView*>(launcherResults_);
}

void LauncherWindow::applyTheme()
{
    ThemeMode mode = AppSettings::instance().themeMode();
    if (mode == ThemeMode::System) {
        const QColor window = QApplication::palette().color(QPalette::Window);
        mode = window.lightness() < 128 ? ThemeMode::Dark : ThemeMode::Light;
    }

    if (mode == ThemeMode::Light) {
        setStyleSheet(QStringLiteral(R"(
            QWidget#QuickaryPanel { background: #f7f8fa; color: #191b20; border: 1px solid #d7dbe2; font-family: "Segoe UI", "Microsoft YaHei UI"; font-size: 10.5pt; }
            QWidget#resultPane, QWidget#facetBar { background: transparent; }
            QLineEdit#searchBox { background: #ffffff; color: #191b20; border: 1px solid #cbd1da; border-radius: 12px; padding: 0 16px; font-size: 15pt; selection-background-color: #4169e1; }
            QLineEdit#searchBox:focus { border: 1px solid #5577e8; }
            QListView#results, QTableView#deepResults { background: transparent; color: #202329; border: 0; outline: 0; selection-background-color: #dfe7ff; selection-color: #10131a; alternate-background-color: #f1f3f6; }
            QHeaderView::section { background: #eef1f5; color: #505762; border: 0; border-bottom: 1px solid #d7dbe2; padding: 6px; }
            QToolButton { background: #eef1f5; border: 1px solid #d7dbe2; border-radius: 8px; padding: 4px 9px; }
            QToolButton:checked { background: #dfe7ff; border-color: #7f98eb; }
            QLabel#facetLabel, QLabel#status { color: #66707d; padding: 2px 6px; font-size: 9pt; }
            QSplitter::handle { background: #d7dbe2; width: 1px; }
            QPlainTextEdit { background: #ffffff; color: #202329; border: 1px solid #e0e3e8; border-radius: 8px; padding: 8px; }
        )"));
    } else {
        setStyleSheet(QStringLiteral(R"(
            QWidget#QuickaryPanel { background: #17191d; color: #f2f3f5; border: 1px solid #2a2e34; font-family: "Segoe UI", "Microsoft YaHei UI"; font-size: 10.5pt; }
            QWidget#resultPane, QWidget#facetBar { background: transparent; }
            QLineEdit#searchBox { background: #22262c; color: #f2f3f5; border: 1px solid #343a42; border-radius: 12px; padding: 0 16px; font-size: 15pt; selection-background-color: #5d78ff; }
            QLineEdit#searchBox:focus { border: 1px solid #6f86ff; }
            QListView#results, QTableView#deepResults { background: transparent; color: #f2f3f5; border: 0; outline: 0; selection-background-color: #344264; selection-color: #ffffff; alternate-background-color: #1d2025; }
            QHeaderView::section { background: #20242a; color: #aeb6c2; border: 0; border-bottom: 1px solid #30353d; padding: 6px; }
            QToolButton { background: #22262c; color: #cbd1d9; border: 1px solid #343a42; border-radius: 8px; padding: 4px 9px; }
            QToolButton:checked { background: #344264; color: #ffffff; border-color: #6079bd; }
            QLabel#facetLabel, QLabel#status { color: #8f98a5; padding: 2px 6px; font-size: 9pt; }
            QSplitter::handle { background: #252a30; width: 1px; }
            QPlainTextEdit { background: #111317; color: #dce1e8; border-radius: 8px; padding: 8px; }
        )"));
    }
}

void LauncherWindow::applySettings()
{
    applyTheme();
    if (isVisible()) {
        if (AppSettings::instance().previewByDefault() && !previewVisible_) {
            previewVisible_ = true;
            preview_->show();
        }
        updatePreview();
        triggerSearch();
    }
}

void LauncherWindow::summon(bool deepSearch)
{
    const bool wasVisible = isVisible();
    currentFolder_ = DialogNavigator::activeExplorerFolder();
    setDeepMode(deepSearch);
    if (!wasVisible) search_->clear();

    previewVisible_ = AppSettings::instance().previewByDefault();
    preview_->setVisible(previewVisible_);

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

void LauncherWindow::resetFacetFilter()
{
    for (auto* button : facetButtons_) {
        QSignalBlocker block(button);
        button->setChecked(false);
    }
    {
        QSignalBlocker block(allFacet_);
        allFacet_->setChecked(true);
    }
    proxy_->setKindFilters({});
}

void LauncherWindow::setDeepMode(bool deep)
{
    deep_ = deep;
    resultStack_->setCurrentWidget(deep ? static_cast<QWidget*>(deepResults_) : static_cast<QWidget*>(launcherResults_));
    facetBar_->setVisible(deep);
    resize(deep ? 1180 : 760, deep ? 720 : 430);
    if (!deep) resetFacetFilter();
    if (deep) {
        status_->setText(QStringLiteral("Deep Search · click column headers to sort · combine type facets · Alt+P preview · Ctrl+, settings"));
    } else {
        status_->setText(QStringLiteral("Launcher · Enter open · Ctrl+Enter reveal · Alt+P preview · Ctrl+O actions · Ctrl+, settings"));
    }
    selectFirstResult();
}

void LauncherWindow::triggerSearch()
{
    coordinator_->search(search_->text(), currentFolder_, deep_);
}

void LauncherWindow::updateFacetCounts()
{
    QHash<int, int> counts;
    for (const auto& item : model_->items()) ++counts[static_cast<int>(item.kind)];
    allFacet_->setText(QStringLiteral("%1 %2").arg(allFacet_->property("baseText").toString()).arg(model_->rowCount()));
    for (auto it = facetButtons_.cbegin(); it != facetButtons_.cend(); ++it) {
        const QString base = it.value()->property("baseText").toString();
        it.value()->setText(QStringLiteral("%1 %2").arg(base).arg(counts.value(it.key())));
    }
}

void LauncherWindow::onResults(const QVector<SearchItem>& items, quint64)
{
    model_->setItems(items);
    updateFacetCounts();
    selectFirstResult();
    if (deep_) {
        status_->setText(QStringLiteral("Deep Search · %1 shown · sort by Name/Path/Type/Size/Modified · combine type facets · Alt+P preview")
                         .arg(proxy_->rowCount()));
    } else {
        status_->setText(QStringLiteral("Launcher · %1 results · Enter open · Ctrl+Enter reveal · Alt+P preview · Ctrl+O actions")
                         .arg(proxy_->rowCount()));
    }
}

void LauncherWindow::updateFacetFilter()
{
    QSet<int> kinds;
    for (auto it = facetButtons_.cbegin(); it != facetButtons_.cend(); ++it) {
        if (it.value()->isChecked()) kinds.insert(it.key());
    }
    {
        QSignalBlocker block(allFacet_);
        allFacet_->setChecked(kinds.isEmpty());
    }
    proxy_->setKindFilters(kinds);
    selectFirstResult();
    if (deep_) {
        status_->setText(QStringLiteral("Deep Search · %1 matching current facets · click headers to sort · Alt+P preview")
                         .arg(proxy_->rowCount()));
    }
}

void LauncherWindow::selectFirstResult()
{
    QAbstractItemView* view = activeResults();
    if (!view || proxy_->rowCount() <= 0) return;
    view->setCurrentIndex(proxy_->index(0, SearchResultModel::NameColumn));
}

void LauncherWindow::moveSelection(int delta)
{
    if (proxy_->rowCount() <= 0) return;
    QAbstractItemView* view = activeResults();
    int row = view->currentIndex().row();
    if (row < 0) row = 0;
    row = qBound(0, row + delta, proxy_->rowCount() - 1);
    view->setCurrentIndex(proxy_->index(row, SearchResultModel::NameColumn));
    view->scrollTo(proxy_->index(row, SearchResultModel::NameColumn));
}

const SearchItem* LauncherWindow::currentItem() const
{
    const QAbstractItemView* view = activeResults();
    if (!view) return nullptr;
    QModelIndex proxyIndex = view->currentIndex();
    if (!proxyIndex.isValid()) return nullptr;
    proxyIndex = proxy_->index(proxyIndex.row(), SearchResultModel::NameColumn);
    const QModelIndex source = proxy_->mapToSource(proxyIndex);
    return model_->itemAt(source.row());
}

QVector<SearchItem> LauncherWindow::selectedItems() const
{
    QVector<SearchItem> selected;
    QSet<int> sourceRows;
    const QAbstractItemView* view = activeResults();
    if (!view || !view->selectionModel()) return selected;

    QModelIndexList indexes;
    if (deep_) indexes = view->selectionModel()->selectedRows(SearchResultModel::NameColumn);
    else indexes = view->selectionModel()->selectedIndexes();

    for (QModelIndex proxyIndex : indexes) {
        if (!proxyIndex.isValid()) continue;
        proxyIndex = proxy_->index(proxyIndex.row(), SearchResultModel::NameColumn);
        const QModelIndex source = proxy_->mapToSource(proxyIndex);
        if (!source.isValid() || sourceRows.contains(source.row())) continue;
        sourceRows.insert(source.row());
        if (const auto* item = model_->itemAt(source.row())) selected.push_back(*item);
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
    if (handled && !deep_ && AppSettings::instance().closeAfterActivation()) hide();
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
    QAction* nativeMenu = nullptr;
    QAction* copyPath = nullptr;
    QAction* copy = nullptr;
    QAction* cut = nullptr;
    QAction* pin = nullptr;
    QAction* trash = nullptr;
    if (!paths.isEmpty()) {
        if (!batch) {
            reveal = menu.addAction(QStringLiteral("Open containing folder"));
            nativeMenu = menu.addAction(QStringLiteral("Windows context menu…"));
        }
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
            QAction* actionItem = menu.addAction(action.name);
            customActions.insert(actionItem, action);
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
    } else if (chosen == nativeMenu) {
        ShellActions::showNativeContextMenu(primary.path, this);
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
    if (const auto* item = currentItem(); item && !item->path.isEmpty()) preview_->preview(item->path);
    else preview_->clearPreview();
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
        moveSelection(e->key() == Qt::Key_N ? 1 : -1);
        return;
    }
    QWidget::keyPressEvent(e);
}

void LauncherWindow::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    previewVisible_ = false;
    preview_->hide();
    preview_->clearPreview();
}

} // namespace quickary
