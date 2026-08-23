#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSpinBox;

namespace quickary {

class SettingsDialog final : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

signals:
    void settingsApplied();

private slots:
    void apply();

private:
    void load();

    QComboBox* theme_{};
    QCheckBox* startup_{};
    QCheckBox* explorerTyping_{};
    QCheckBox* previewDefault_{};
    QCheckBox* nativePreview_{};
    QCheckBox* closeAfterActivation_{};
    QSpinBox* launcherLimit_{};
    QSpinBox* deepLimit_{};
};

} // namespace quickary
