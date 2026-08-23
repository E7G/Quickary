#pragma once

#include <QWidget>
#include <memory>

namespace quickary {

class ShellPreviewHost final : public QWidget {
    Q_OBJECT
public:
    explicit ShellPreviewHost(QWidget* parent = nullptr);
    ~ShellPreviewHost() override;

    bool preview(const QString& path);
    void clear();
    bool isPreviewing() const;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace quickary
