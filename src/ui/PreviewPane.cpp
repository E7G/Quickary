#include "PreviewPane.h"

#include "../core/AppSettings.h"
#include "../platform/ShellPreviewHost.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QLocale>
#include <QPlainTextEdit>
#include <QTextCursor>

namespace quickary {

PreviewPane::PreviewPane(QWidget* parent) : QStackedWidget(parent)
{
    shell_ = new ShellPreviewHost(this);
    image_ = new QLabel(this);
    image_->setAlignment(Qt::AlignCenter);
    image_->setMinimumWidth(280);
    text_ = new QPlainTextEdit(this);
    text_->setReadOnly(true);
    text_->setFrameShape(QFrame::NoFrame);
    info_ = new QLabel(this);
    info_->setAlignment(Qt::AlignCenter);
    info_->setWordWrap(true);
    addWidget(info_);
    addWidget(shell_);
    addWidget(image_);
    addWidget(text_);
}

PreviewPane::~PreviewPane()
{
    clearPreview();
}

void PreviewPane::clearPreview()
{
    if (shell_) shell_->clear();
    if (image_) image_->clear();
    if (text_) text_->clear();
}

void PreviewPane::preview(const QString& path)
{
    clearPreview();
    QFileInfo fi(path);
    if (!fi.exists()) {
        info_->setText(QStringLiteral("No preview available"));
        setCurrentWidget(info_);
        return;
    }

    QImageReader reader(path);
    if (reader.canRead()) {
        reader.setAutoTransform(true);
        QSize target = reader.size();
        target.scale(QSize(720, 720), Qt::KeepAspectRatio);
        reader.setScaledSize(target);
        const QImage img = reader.read();
        if (!img.isNull()) {
            image_->setPixmap(QPixmap::fromImage(img));
            setCurrentWidget(image_);
            return;
        }
    }

    static const QStringList textExt{QStringLiteral("txt"), QStringLiteral("md"), QStringLiteral("json"), QStringLiteral("xml"),
                                     QStringLiteral("yaml"), QStringLiteral("yml"), QStringLiteral("ini"), QStringLiteral("log"),
                                     QStringLiteral("cpp"), QStringLiteral("h"), QStringLiteral("hpp"), QStringLiteral("c"),
                                     QStringLiteral("py"), QStringLiteral("js"), QStringLiteral("ts"), QStringLiteral("css"),
                                     QStringLiteral("html"), QStringLiteral("toml"), QStringLiteral("csv"), QStringLiteral("ps1")};
    if (textExt.contains(fi.suffix().toCaseFolded()) && fi.size() <= 4 * 1024 * 1024) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray bytes = f.read(512 * 1024);
            text_->setPlainText(QString::fromUtf8(bytes));
            text_->moveCursor(QTextCursor::Start);
            setCurrentWidget(text_);
            return;
        }
    }

    if (AppSettings::instance().nativePreviewEnabled() && fi.isFile() && shell_->preview(path)) {
        setCurrentWidget(shell_);
        return;
    }

    info_->setText(QStringLiteral("%1\n\n%2\n%3")
        .arg(fi.fileName(), SearchResultModel::formatSize(static_cast<quint64>(fi.size())),
             QLocale().toString(fi.lastModified(), QLocale::ShortFormat)));
    setCurrentWidget(info_);
}

} // namespace quickary
