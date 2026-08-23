#include "SettingsDialog.h"

#include "../core/AppSettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace quickary {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Quickary Settings"));
    setModal(false);
    resize(520, 430);

    auto* root = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    root->addWidget(tabs, 1);

    auto* general = new QWidget(tabs);
    auto* generalLayout = new QVBoxLayout(general);
    auto* behaviorGroup = new QGroupBox(QStringLiteral("Behavior"), general);
    auto* behavior = new QVBoxLayout(behaviorGroup);
    startup_ = new QCheckBox(QStringLiteral("Start Quickary when I sign in to Windows"), behaviorGroup);
    explorerTyping_ = new QCheckBox(QStringLiteral("Type in Explorer to open Deep Search"), behaviorGroup);
    previewDefault_ = new QCheckBox(QStringLiteral("Show preview pane by default"), behaviorGroup);
    nativePreview_ = new QCheckBox(QStringLiteral("Use installed Windows preview handlers for Office, PDF, and other formats"), behaviorGroup);
    closeAfterActivation_ = new QCheckBox(QStringLiteral("Close Launcher after opening an item"), behaviorGroup);
    behavior->addWidget(startup_);
    behavior->addWidget(explorerTyping_);
    behavior->addWidget(previewDefault_);
    behavior->addWidget(nativePreview_);
    behavior->addWidget(closeAfterActivation_);
    generalLayout->addWidget(behaviorGroup);
    generalLayout->addStretch(1);
    tabs->addTab(general, QStringLiteral("General"));

    auto* search = new QWidget(tabs);
    auto* searchLayout = new QVBoxLayout(search);
    auto* limitsGroup = new QGroupBox(QStringLiteral("Result limits"), search);
    auto* limits = new QFormLayout(limitsGroup);
    launcherLimit_ = new QSpinBox(limitsGroup);
    launcherLimit_->setRange(16, 256);
    launcherLimit_->setSingleStep(16);
    deepLimit_ = new QSpinBox(limitsGroup);
    deepLimit_->setRange(100, 2000);
    deepLimit_->setSingleStep(100);
    limits->addRow(QStringLiteral("Launcher:"), launcherLimit_);
    limits->addRow(QStringLiteral("Deep Search:"), deepLimit_);
    searchLayout->addWidget(limitsGroup);

    auto* note = new QLabel(QStringLiteral(
        "Network/NAS locations are searched through Everything's Folder Index. Add your shares in Everything → Tools → Options → Indexes → Folders; Quickary will then search them through the same low-latency IPC path."), search);
    note->setWordWrap(true);
    note->setTextInteractionFlags(Qt::TextSelectableByMouse);
    searchLayout->addWidget(note);
    searchLayout->addStretch(1);
    tabs->addTab(search, QStringLiteral("Search"));

    auto* appearance = new QWidget(tabs);
    auto* appearanceLayout = new QFormLayout(appearance);
    theme_ = new QComboBox(appearance);
    theme_->addItem(QStringLiteral("Follow Windows"), static_cast<int>(ThemeMode::System));
    theme_->addItem(QStringLiteral("Dark"), static_cast<int>(ThemeMode::Dark));
    theme_->addItem(QStringLiteral("Light"), static_cast<int>(ThemeMode::Light));
    appearanceLayout->addRow(QStringLiteral("Theme:"), theme_);
    tabs->addTab(appearance, QStringLiteral("Appearance"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] { apply(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::apply);

    load();
}

void SettingsDialog::load()
{
    auto& settings = AppSettings::instance();
    const int themeIndex = theme_->findData(static_cast<int>(settings.themeMode()));
    if (themeIndex >= 0) theme_->setCurrentIndex(themeIndex);
    startup_->setChecked(settings.launchAtStartup());
    explorerTyping_->setChecked(settings.explorerTypeToSearch());
    previewDefault_->setChecked(settings.previewByDefault());
    nativePreview_->setChecked(settings.nativePreviewEnabled());
    closeAfterActivation_->setChecked(settings.closeAfterActivation());
    launcherLimit_->setValue(settings.launcherResultLimit());
    deepLimit_->setValue(settings.deepSearchResultLimit());
}

void SettingsDialog::apply()
{
    auto& settings = AppSettings::instance();
    settings.setThemeMode(static_cast<ThemeMode>(theme_->currentData().toInt()));
    settings.setLaunchAtStartup(startup_->isChecked());
    settings.setExplorerTypeToSearch(explorerTyping_->isChecked());
    settings.setPreviewByDefault(previewDefault_->isChecked());
    settings.setNativePreviewEnabled(nativePreview_->isChecked());
    settings.setCloseAfterActivation(closeAfterActivation_->isChecked());
    settings.setLauncherResultLimit(launcherLimit_->value());
    settings.setDeepSearchResultLimit(deepLimit_->value());
    settings.sync();
    emit settingsApplied();
}

} // namespace quickary
