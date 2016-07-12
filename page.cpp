/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "page.hpp"

extern "C" {
#include <mupdf/fitz.h>
}

#include <QImage>
#include <QSharedData>

namespace QMuPDF
{

QRectF convert_fz_rect(const fz_rect &rect, const QSizeF &dpi)
{
    return QRectF(QPointF(rect.x0, rect.y0) * dpi.width() / 72.,
                  QPointF(rect.x1, rect.y1) * dpi.height() / 72.);
}

QImage convert_fz_pixmap(fz_context *ctx, fz_pixmap *image)
{
    const int w = fz_pixmap_width(ctx, image);
    const int h = fz_pixmap_height(ctx, image);
    QImage img(w, h, QImage::Format_ARGB32);
    unsigned char *data = fz_pixmap_samples(ctx, image);
    unsigned int *imgdata = (unsigned int *)img.bits();
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            *imgdata = qRgba(data[0], data[1], data[2], data[3]);
            data = data + 4;
            imgdata++;
        }
    }
    return img;
}

struct Page::Data : public QSharedData
{
    Data(): pageNum(-1), doc(0), page(0) { }
    int pageNum;
    fz_context *ctx;
    fz_document *doc;
    fz_page *page;
};

Page::Page()
    : d(new Data)
{
}

Page::~Page()
{
    fz_drop_page(d->ctx, d->page);
}

Page::Page(fz_context_s *ctx, fz_document_s *doc, int num) :
    d(new Page::Data)
{
    Q_ASSERT(doc && ctx);
    d->page = fz_load_page(ctx, doc, num);
    d->pageNum = num;
    d->doc = doc;
    d->ctx = ctx;
}

Page::Page(const Page &other) :
    d(other.d)
{
}

int Page::number() const
{
    return d->pageNum;
}

QSizeF Page::size(const QSizeF &dpi) const
{
    fz_rect rect;
    fz_bound_page(d->ctx, d->page, &rect);
    // MuPDF always assumes 72dpi
    return QSizeF((rect.x1 - rect.x0) * dpi.width() / 72.,
                  (rect.y1 - rect.y0) * dpi.height() / 72.);
}

qreal Page::duration() const
{
    float val;
    (void)fz_page_presentation(d->ctx, d->page, &val);
    return val < 0.1 ? -1 : val;
}

QImage Page::render(qreal width, qreal height) const
{
    const QSizeF s = size(QSizeF(72, 72));

    fz_matrix ctm;
    fz_scale(&ctm, width / s.width(), height / s.height());

    fz_cookie cookie = { 0, 0, 0, 0, 0, 0 };
    fz_colorspace *csp = fz_device_rgb(d->ctx);
    fz_pixmap *image = fz_new_pixmap(d->ctx, csp, width, height);
    fz_clear_pixmap_with_value(d->ctx, image, 0xff);
    fz_device *device = fz_new_draw_device(d->ctx, image);
    fz_run_page(d->ctx, d->page, device, &ctm, &cookie);
    fz_drop_device(d->ctx, device);

    QImage img;
    if (!cookie.errors) {
        img = convert_fz_pixmap(d->ctx, image);
    }
    fz_drop_pixmap(d->ctx, image);
    return img;
}

QVector<TextBox *> Page::textBoxes(const QSizeF &dpi) const
{
    fz_cookie cookie = { 0, 0, 0, 0, 0, 0 };
    fz_stext_page *page = fz_new_stext_page(d->ctx);
    fz_stext_sheet *sheet = fz_new_stext_sheet(d->ctx);
    fz_device *device = fz_new_stext_device(d->ctx, sheet, page);
    fz_run_page(d->ctx, d->page, device, &fz_identity, &cookie);
    fz_drop_device(d->ctx, device);
    if (cookie.errors) {
        fz_drop_stext_page(d->ctx, page);
        fz_drop_stext_sheet(d->ctx, sheet);
        return QVector<TextBox *>();
    }

    QVector<TextBox *> boxes;

    for (int i_block = 0; i_block < page->len; ++i_block) {
        if (page->blocks[i_block].type != FZ_PAGE_BLOCK_TEXT) {
            continue;
        }
        fz_stext_block &block = *page->blocks[i_block].u.text;
        for (int i_line = 0; i_line < block.len; ++i_line) {
            fz_stext_line &line = block.lines[i_line];
            bool hasText = false;
            for (fz_stext_span *s = line.first_span; s; s = s->next) {
                fz_stext_span &span = *s;
                for (int i_char = 0; i_char < span.len; ++i_char) {
                    fz_rect bbox; fz_stext_char_bbox(d->ctx, &bbox, s, i_char);
                    const int text = span.text[i_char].c;
                    TextBox *box = new TextBox(text, convert_fz_rect(bbox, dpi));
                    boxes.append(box);
                    hasText = true;
                }
            }
            if (hasText) {
                boxes.back()->markAtEndOfLine();
            }
        }
    }

    fz_drop_stext_page(d->ctx, page);
    fz_drop_stext_sheet(d->ctx, sheet);

    return boxes;
}

}
