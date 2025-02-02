/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef QMUPDF_PAGE_HPP
#define QMUPDF_PAGE_HPP

#include <QRect>
#include <QString>
#include <QSharedDataPointer>

class QImage;
class QSizeF;

struct fz_document;
struct fz_context;

namespace QMuPDF
{

class TextBox;

class Page
{
public:
    Page(fz_context *ctx, fz_document *doc, int num);
    Page(const Page &other);

    ~Page();
    int number() const;
    QSizeF size(const QSizeF &dpi) const;
    qreal duration() const;
    QString label() const;
    QImage render(qreal width, qreal height) const;
    QVector<TextBox *> textBoxes(const QSizeF &dpi) const;

private:
    Page();
    struct Data;
    QSharedDataPointer<Data> d;
};

class TextBox
{
public:
    TextBox(QChar c, const QRectF &bbox)
        : m_text(c), m_rect(bbox), m_end(false) { }
    QRectF rect() const
    {
        return m_rect;
    }
    QChar text() const
    {
        return m_text;
    }
    bool isAtEndOfLine() const
    {
        return m_end;
    }
    void markAtEndOfLine()
    {
        m_end = true;
    }
private:
    QChar m_text;
    QRectF m_rect;
    bool m_end : 1;
};

} // namespace QMuPDF

#endif
