#include "PreviewPane.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTextStream>
#include <QTextCursor>
#include <QLocale>

namespace quickary {

PreviewPane::PreviewPane(QWidget* parent) : QStackedWidget(parent)
{
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
    addWidget(image_);
    addWidget(text_);
}

void PreviewPane::preview(const QString& path)
{
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
        target.scale(QSize(520, 520), Qt::KeepAspectRatio);
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
                                     QStringLiteral("py"), QStringLiteral("js"), QStringLiteral("ts"), QStringLiteral("css"), QStringLiteral("html")};
    if (textExt.contains(fi.suffix().toCaseFolded()) && fi.size() <= 1024 * 1024) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray bytes = f.read(256 * 1024);
            text_->setPlainText(QString::fromUtf8(bytes));
            text_->moveCursor(QTextCursor::Start);
            setCurrentWidget(text_);
            return;
        }
    }

    info_->setText(QStringLiteral("%1\n\n%2 bytes\n%3")
        .arg(fi.fileName()).arg(fi.size()).arg(QLocale().toString(fi.lastModified(), QLocale::ShortFormat)));
    setCurrentWidget(info_);
}

} // namespace quickary
