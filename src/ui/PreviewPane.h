#pragma once

#include <QStackedWidget>

class QLabel;
class QPlainTextEdit;

namespace quickary {

class ShellPreviewHost;

class PreviewPane final : public QStackedWidget {
    Q_OBJECT
public:
    explicit PreviewPane(QWidget* parent = nullptr);
    ~PreviewPane() override;

    void preview(const QString& path);
    void clearPreview();

private:
    ShellPreviewHost* shell_{};
    QLabel* image_{};
    QPlainTextEdit* text_{};
    QLabel* info_{};
};

} // namespace quickary
