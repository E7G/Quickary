#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QShowEvent;
class QSpinBox;

namespace quickary {

class SettingsDialog final : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

signals:
    void settingsApplied();

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void apply();

private:
    void load();
    void refreshApiSummary();

    QComboBox* theme_{};
    QCheckBox* startup_{};
    QCheckBox* explorerTyping_{};
    QCheckBox* previewDefault_{};
    QCheckBox* nativePreview_{};
    QCheckBox* closeAfterActivation_{};
    QSpinBox* launcherLimit_{};
    QSpinBox* deepLimit_{};
    QCheckBox* httpApiEnabled_{};
    QSpinBox* httpApiPort_{};
    QLineEdit* httpApiToken_{};
    QLineEdit* httpApiEndpoint_{};
    QPushButton* regenerateToken_{};
};

} // namespace quickary
