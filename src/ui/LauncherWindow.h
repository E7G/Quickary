#pragma once

#include "../core/SearchTypes.h"
#include <QHash>
#include <QWidget>

class QAbstractItemView;
class QLabel;
class QLineEdit;
class QListView;
class QModelIndex;
class QSplitter;
class QStackedWidget;
class QTableView;
class QTimer;
class QToolButton;

namespace quickary {

class PreviewPane;
class ResultFilterProxy;
class SearchCoordinator;
class SearchResultModel;

class LauncherWindow final : public QWidget {
    Q_OBJECT
public:
    explicit LauncherWindow(SearchCoordinator* coordinator, QWidget* parent = nullptr);

    void summon(bool deepSearch = false);
    void typeFromExplorer(const QString& text);

public slots:
    void applySettings();

signals:
    void settingsRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void triggerSearch();
    void onResults(const QVector<SearchItem>& items, quint64 serial);
    void activateCurrent();
    void showActions();
    void updatePreview();
    void updateFacetFilter();

private:
    QAbstractItemView* activeResults() const;
    const SearchItem* currentItem() const;
    QVector<SearchItem> selectedItems() const;
    void activate(const SearchItem& item);
    void setDeepMode(bool deep);
    void applyTheme();
    void updateFacetCounts();
    void resetFacetFilter();
    void selectFirstResult();
    void moveSelection(int delta);

    SearchCoordinator* coordinator_{};
    QLineEdit* search_{};
    QListView* launcherResults_{};
    QTableView* deepResults_{};
    QStackedWidget* resultStack_{};
    SearchResultModel* model_{};
    ResultFilterProxy* proxy_{};
    PreviewPane* preview_{};
    QSplitter* splitter_{};
    QWidget* facetBar_{};
    QToolButton* allFacet_{};
    QHash<int, QToolButton*> facetButtons_;
    QLabel* status_{};
    QTimer* debounce_{};
    QString currentFolder_;
    bool deep_{false};
    bool previewVisible_{false};
};

} // namespace quickary
