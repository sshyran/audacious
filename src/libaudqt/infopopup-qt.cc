/*
 * infopopup-qt.cc
 * Copyright 2018 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/tuple.h>

#include "libaudqt.h"
#include "libaudqt-internal.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>

namespace audqt {

class InfoPopup : public PopupWidget
{
public:
    InfoPopup (const String & filename, const Tuple & tuple, bool under_mouse);

private:
    void add_field (int row, const char * field, const char * value);
    void add_fields (const Tuple & tuple);
    void art_ready (const char * filename);
    void finish_loading ();

    void paintEvent (QPaintEvent *) override;

    HookReceiver<InfoPopup, const char *> art_ready_hook
        {"art ready", this, & InfoPopup::art_ready};

    QHBoxLayout m_hbox;
    QGridLayout m_grid;
    String m_filename;
    bool m_queued = false;
};

InfoPopup::InfoPopup (const String & filename, const Tuple & tuple, bool under_mouse) :
    PopupWidget (under_mouse),
    m_filename (filename)
{
    setWindowFlags (Qt::ToolTip);

    m_hbox.setMargin (sizes.TwoPt);
    m_hbox.setSpacing (sizes.FourPt);
    setLayout (& m_hbox);

    m_grid.setMargin (0);
    m_grid.setHorizontalSpacing (sizes.FourPt);
    m_grid.setVerticalSpacing (0);
    m_hbox.addLayout (& m_grid);

    add_fields (tuple);
    finish_loading ();
}

void InfoPopup::add_fields (const Tuple & tuple)
{
    String title = tuple.get_str (Tuple::Title);
    String artist = tuple.get_str (Tuple::Artist);
    String album = tuple.get_str (Tuple::Album);
    String genre = tuple.get_str (Tuple::Genre);

    int year = tuple.get_int (Tuple::Year);
    int track = tuple.get_int (Tuple::Track);
    int length = tuple.get_int (Tuple::Length);
    int row = 0;

    if (title)
        add_field (row ++, _("Title"), title);
    if (artist)
        add_field (row ++, _("Artist"), artist);
    if (album)
        add_field (row ++, _("Album"), album);
    if (genre)
        add_field (row ++, _("Genre"), title);
    if (year > 0)
        add_field (row ++, _("Year"), int_to_str (year));
    if (track > 0)
        add_field (row ++, _("Track"), int_to_str (track));
    if (length > 0)
        add_field (row ++, _("Length"), str_format_time (length));
}

void InfoPopup::add_field (int row, const char * field, const char * value)
{
    auto header = new QLabel (this);
    header->setTextFormat (Qt::RichText);
    header->setText (QString ("<i><font color=\"#a0a0a0\">%1</font></i>").arg (field));
    m_grid.addWidget (header, row, 0, Qt::AlignRight);

    auto label = new QLabel (this);
    header->setTextFormat (Qt::RichText);
    auto html = QString (value).toHtmlEscaped ();
    label->setText (QString ("<font color=\"#ffffff\">%1</font>").arg (html));
    m_grid.addWidget (label, row, 1, Qt::AlignLeft);
}

void InfoPopup::art_ready (const char * filename)
{
    if (m_queued && strcmp (filename, m_filename) == 0)
        finish_loading ();
}

void InfoPopup::finish_loading ()
{
    QImage image = art_request (m_filename, & m_queued);

    if (! image.isNull ())
    {
        auto label = new QLabel (this);
        label->setPixmap (art_scale (image, sizes.OneInch, sizes.OneInch));
        m_hbox.insertWidget (0, label);
    }

    if (! m_queued)
        show ();
}

void InfoPopup::paintEvent (QPaintEvent *)
{
    QLinearGradient grad (0, 0, 0, height ());
    grad.setStops ({
        {0, QColor (64, 64, 64)},
        {0.499, QColor (38, 38, 38)},
        {0.5, QColor (26, 26, 26)},
        {1, QColor (0, 0, 0)}
    });

    QPainter p (this);
    p.fillRect (rect (), grad);
}

static InfoPopup * s_infopopup;

static void infopopup_show (const String & filename, const Tuple & tuple, bool under_mouse)
{
    delete s_infopopup;
    s_infopopup = new InfoPopup (filename, tuple, under_mouse);

    QObject::connect (s_infopopup, & QObject::destroyed, [] () {
        s_infopopup = nullptr;
    });
}

EXPORT void infopopup_show (Playlist playlist, int entry, bool under_mouse)
{
    String filename = playlist.entry_filename (entry);
    Tuple tuple = playlist.entry_tuple (entry);

    if (filename && tuple.valid ())
        infopopup_show (filename, tuple, under_mouse);
}

EXPORT void infopopup_show_current (bool under_mouse)
{
    auto playlist = Playlist::playing_playlist ();
    if (playlist == Playlist ())
        playlist = Playlist::active_playlist ();

    int position = playlist.get_position ();
    if (position >= 0)
        infopopup_show (playlist, position, under_mouse);
}

EXPORT void infopopup_hide ()
{
    delete s_infopopup;
}

} // namespace audqt
