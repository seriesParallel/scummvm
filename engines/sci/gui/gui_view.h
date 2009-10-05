/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef SCI_GUI_VIEW_H
#define SCI_GUI_VIEW_H

namespace Sci {

struct sciViewCelInfo {
	int16 width, height;
	char displaceX;
	byte displaceY;
	byte clearKey;
	uint16 offsetEGA;
	uint16 offsetRLE;
	uint16 offsetLiteral;
	byte *rawBitmap;
};

struct sciViewLoopInfo {
	bool mirrorFlag;
	uint16 celCount;
	sciViewCelInfo *cel;
};

class SciGuiView {
public:
	SciGuiView(OSystem *system, EngineState *state, SciGuiGfx *gfx, SciGuiScreen *screen, GuiResourceId resourceId);
	~SciGuiView();

	// TODO: Remove gfx reference after putting palette things into SciGuiScreen

	GuiResourceId getResourceId();
	int16 getWidth(GuiViewLoopNo loopNo, GuiViewCelNo celNo);
	int16 getHeight(GuiViewLoopNo loopNo, GuiViewCelNo celNo);
	sciViewCelInfo *getCelInfo(GuiViewLoopNo loopNo, GuiViewCelNo celNo);
	sciViewLoopInfo *getLoopInfo(GuiViewLoopNo loopNo);
	void getCelRect(GuiViewLoopNo loopNo, GuiViewCelNo celNo, int16 x, int16 y, int16 z, Common::Rect *outRect);
	byte *getBitmap(GuiViewLoopNo loopNo, GuiViewCelNo celNo);
	void draw(Common::Rect rect, Common::Rect clipRect, GuiViewLoopNo loopNo, GuiViewCelNo celNo, byte priority, uint16 paletteNo);

private:
	void initData(GuiResourceId resourceId);
	void unpackCel(GuiViewLoopNo loopNo, GuiViewCelNo celNo, byte *outPtr, uint16 pixelCount);

	OSystem *_system;
	EngineState *_s;
	SciGuiGfx *_gfx;
	SciGuiScreen *_screen;

	GuiResourceId _resourceId;
	byte *_resourceData;

	uint16 _loopCount;
	sciViewLoopInfo *_loop;
	bool _embeddedPal;
	GuiPalette _palette;
	const byte *_EGAMapping; // simple translation map for all 16 colors
};

} // End of namespace Sci

#endif
