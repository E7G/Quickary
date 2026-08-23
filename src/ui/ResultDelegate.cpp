#include "ResultDelegate.h"
#include "SearchResultModel.h"

#include <QPainter>
#include <QPainterPath>

namespace quickary {

QSize ResultDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const { return {0, 58}; }

void ResultDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    QRect r = opt.rect.adjusted(6, 3, -6, -3);
    if (opt.state & QStyle::State_Selected) {
        p->setPen(Qt::NoPen);
        p->setBrush(opt.palette.highlight().color());
        p->drawRoundedRect(r, 10, 10);
    }

    const QIcon icon = qvariant_cast<QIcon>(idx.data(Qt::DecorationRole));
    const QRect iconRect(r.left() + 10, r.top() + 9, 34, 34);
    icon.paint(p, iconRect, Qt::AlignCenter);

    const int textLeft = iconRect.right() + 12;
    const QRect titleRect(textLeft, r.top() + 8, r.width() - (textLeft - r.left()) - 12, 22);
    const QRect subRect(textLeft, r.top() + 30, titleRect.width(), 18);

    QFont titleFont = opt.font;
    titleFont.setWeight(QFont::DemiBold);
    p->setFont(titleFont);
    p->setPen((opt.state & QStyle::State_Selected) ? opt.palette.highlightedText().color() : opt.palette.text().color());
    p->drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft,
                opt.fontMetrics.elidedText(idx.data(Qt::DisplayRole).toString(), Qt::ElideRight, titleRect.width()));

    QFont subFont = opt.font;
    subFont.setPointSizeF(qMax<qreal>(8.0, subFont.pointSizeF() - 1.0));
    p->setFont(subFont);
    QColor secondary = (opt.state & QStyle::State_Selected) ? opt.palette.highlightedText().color() : opt.palette.placeholderText().color();
    secondary.setAlpha(190);
    p->setPen(secondary);
    p->drawText(subRect, Qt::AlignVCenter | Qt::AlignLeft,
                opt.fontMetrics.elidedText(idx.data(SearchResultModel::SubtitleRole).toString(), Qt::ElideMiddle, subRect.width()));
    p->restore();
}

} // namespace quickary
