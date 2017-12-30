/*	Copyright 2012 Theo Berkau <cwx@cyberwarriorx.com>

This file is part of Yabause.

Yabause is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Yabause is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Yabause; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#include "UIDebugSCSPChan.h"
#include "CommonDialogs.h"

#include <QImageWriter>
#include <QGraphicsPixmapItem>
#include <QDebug>
#include <QIODevice>
#include <QTimer>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QPainter>

UIDebugSCSPChan::UIDebugSCSPChan(QWidget* p)
   : QDialog(p)
{
   QVBoxLayout *verticalLayout = new QVBoxLayout(this);

   scsp_debug_set_mode(1);

   scsp_debug_instrument_clear();

   envelope_colors[0] = Qt::red;
   envelope_colors[1] = Qt::green;
   envelope_colors[2] = Qt::blue;
   envelope_colors[3] = Qt::cyan;

   for (int i = 0; i < 24; i++)
   {
      checkbox[i] = new QCheckBox("null");
      checkbox[i]->setChecked(false);
      verticalLayout->addWidget(checkbox[i]);
   }

   resize(720, 480);

   timer = new QTimer(this);
   timer->setInterval(50);//20 fps
   timer->start();

   connect(timer, SIGNAL(timeout()), SLOT(update_window()));

   // retranslate widgets
   QtYabause::retranslateWidget(this);

   this->show();
}

UIDebugSCSPChan::~UIDebugSCSPChan()
{
   scsp_debug_set_mode(0);

   delete timer;
   for (int i = 0; i < 24; i++)
      delete checkbox[i];
}

void UIDebugSCSPChan::paintEvent(QPaintEvent *event)
{
   QPainter painter(this);

   painter.setRenderHint(QPainter::Antialiasing);
   
   int channel_width = 16;
   double max_height = 64;
   int spacer = 4;

   int start_x = 64;
   int start_y = 8;

   int space = 0;

   QRect rect;

   for (int i = 0; i < 32; i++)
   {
      int env = 0, state = 0;
      scsp_debug_get_envelope(i, &env, &state);

      double env_ratio = (1023.0 - env)/1023.0;
      rect = QRect(start_x + space + (i * channel_width), 8, channel_width, (int)(max_height * env_ratio));
      painter.setPen(envelope_colors[state]);
      painter.drawRect(rect);
      space += spacer;
   }
}

void UIDebugSCSPChan::update_window()
{
   QString address;

   for (int i = 0; i < 24; i++)
   {
      u32 sa = 0;
      int muted = 0;
      scsp_debug_instrument_get_data(i, &sa, &muted);
      
      address.sprintf("%05X", sa);

      checkbox[i]->setText(address);

      if (checkbox[i]->isChecked())
         scsp_debug_instrument_set_mute(sa, 1);//mute this instrument
      else
         scsp_debug_instrument_set_mute(sa, 0);
   }

   repaint();
}