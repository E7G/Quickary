#pragma once

#include <QStackedWidget>

class QLabel;
class QPlainTextEdit;

namespace quickary {

class PreviewPane final : public QStackedWidget {
    Q_OBJECT
public:
    explicit PreviewPane(QWidget* parent = nullptr);
    void preview(const QString& path);

private:
    QLabel* image_{};
    QPlainTextEdit* text_{};
    QLabel* info_{};
};

} // namespace quickary
