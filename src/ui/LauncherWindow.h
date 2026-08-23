#pragma once

#include "../core/SearchTypes.h"
#include <QWidget>

class QLineEdit;
class QListView;
class QLabel;
class QTimer;
class QSplitter;

namespace quickary {

class SearchCoordinator;
class SearchResultModel;
class PreviewPane;

class LauncherWindow final : public QWidget {
    Q_OBJECT
public:
    explicit LauncherWindow(SearchCoordinator* coordinator, QWidget* parent = nullptr);

    void summon(bool deepSearch = false);
    void typeFromExplorer(const QString& text);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void triggerSearch();
    void onResults(const QVector<SearchItem>& items, quint64 serial);
    void activateCurrent();
    void showActions();
    void updatePreview();

private:
    const SearchItem* currentItem() const;
    QVector<SearchItem> selectedItems() const;
    void activate(const SearchItem& item);
    void setDeepMode(bool deep);
    void applyTheme();

    SearchCoordinator* coordinator_{};
    QLineEdit* search_{};
    QListView* results_{};
    SearchResultModel* model_{};
    PreviewPane* preview_{};
    QSplitter* splitter_{};
    QLabel* status_{};
    QTimer* debounce_{};
    QString currentFolder_;
    bool deep_{false};
    bool previewVisible_{false};
};

} // namespace quickary
