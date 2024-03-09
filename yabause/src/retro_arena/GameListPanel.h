/*
    nanogui/imagepanel.h -- Image panel widget which shows a number of
    square-shaped icons

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/
/** \file */

#pragma once

#include <nanogui/widget.h>
#include "GameInfo.h"

NAMESPACE_BEGIN(nanogui)

/**
 * \class GameListPanel GameListPanel.h nanogui/GameListPanel.h
 *
 * \brief Image panel widget which shows a number of square-shaped icons.
 */
  class NANOGUI_EXPORT GameListPanel : public Widget {
  public:
    typedef std::vector<shared_ptr<GameInfo>> GameInfos;
  public:
    GameListPanel(Widget *parent);

    void setGameInfos(const GameInfos &data);
    const GameInfos& getGameInfos() const { return mGameInfos; }

    std::function<void(int)> callback() const { return mCallback; }
    void setCallback(const std::function<void(int)> &callback) { mCallback = callback; }

    virtual bool mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;
    virtual bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) override;
    virtual Vector2i preferredSize(NVGcontext *ctx) const override;
    virtual void draw(NVGcontext* ctx) override;

    GameListPanel& withGameInfos(const GameInfos& data) { setGameInfos(data); return *this; }
  protected:
    Vector2i gridSize() const;
    int indexForPosition(const Vector2i &p) const;
  protected:
    GameInfos mGameInfos;
    std::function<void(int)> mCallback;
    int mThumbSize;
    int mSpacing;
    int mMargin;
    int mMouseIndex;
};

NAMESPACE_END(nanogui)
