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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "audio/audiostream.h"
#include "audio/decoders/wave.h"
#include "common/config-manager.h"
#include "common/file.h"
#include "common/system.h"
#include "graphics/managed_surface.h"
#include "graphics/palette.h"
#include "image/bmp.h"
#include "image/image_decoder.h"

#include "cryomni3d/mouse_boxes.h"
#include "cryomni3d/font_manager.h"

#include "cryomni3d/versailles/engine.h"

namespace CryOmni3D {
namespace Versailles {

bool CryOmni3DEngine_Versailles::showSubtitles() const {
	return ConfMan.getBool("subtitles");
}

void CryOmni3DEngine_Versailles::drawMenuTitle(Graphics::ManagedSurface *surface, byte color) {
	int offY;

	int oldFont = _fontManager.getCurrentFont();
	_fontManager.setSurface(surface);
	_fontManager.setForeColor(color);
	_fontManager.setCurrentFont(1);
	offY = _fontManager.getFontMaxHeight();
	_fontManager.displayStr(144, 160 - offY, _messages[23]);
	_fontManager.setCurrentFont(3);
	offY = _fontManager.getFontMaxHeight();
	_fontManager.displayStr(305, 160 - offY, _messages[24]);

	surface->vLine(100, 146, 172, color);
	surface->hLine(100, 172, 168, color); // minus 1 because hLine draws inclusive

	_fontManager.setCurrentFont(oldFont);
}

unsigned int CryOmni3DEngine_Versailles::displayOptions() {
	Common::Array<int> menuEntries;
	menuEntries.push_back(26);
	menuEntries.push_back(27);
	menuEntries.push_back(28);
	menuEntries.push_back(29);
	menuEntries.push_back(48);
	menuEntries.push_back(30);
	menuEntries.push_back(32);
#if 0
	// Music on HDD setting
	menuEntries.push_back(34);
#endif
	menuEntries.push_back(25);
	menuEntries.push_back(-42);
	menuEntries.push_back(43);
	menuEntries.push_back(40);
	// 1 is for volume box
	MouseBoxes boxes(menuEntries.size() + 1);

	bool end = false;

	int drawState = 1;

	unsigned int volumeCursorMiddleY = _sprites.getCursor(102).getHeight() / 2;
	unsigned int volume = CLIP(ConfMan.getInt("sfx_volume"), 0, 256);
	unsigned int soundVolumeY = ((283 * (256 - volume)) >> 8) + 101;
	byte volumeForeColor = 243;

	Graphics::ManagedSurface optionsSurface;
	Image::ImageDecoder *imageDecoder = loadHLZ("option.hlz");
	const Graphics::Surface *bgFrame = imageDecoder->getSurface();

	optionsSurface.create(bgFrame->w, bgFrame->h, bgFrame->format);

	setCursor(181);
	g_system->showMouse(true);

	unsigned int hoveredBox = -1;
	unsigned int selectedBox;
	int selectedMsg = 0;
	unsigned int volumeBox;
	bool resetScreen = true;
	bool forceEvents = true;

	while (!g_engine->shouldQuit() && !end) {
		if (resetScreen) {
			setPalette(imageDecoder->getPalette(), imageDecoder->getPaletteStartIndex(),
			           imageDecoder->getPaletteColorCount());
			// _cursorPalette has only 248 colors as 8 last colors are for translucency
			setPalette(_cursorPalette + 240 * 3, 240, 8);

			_fontManager.setCurrentFont(3);
			_fontManager.setTransparentBackground(true);
			_fontManager.setForeColor(243);
			_fontManager.setLineHeight(14);
			_fontManager.setSpaceWidth(0);
			_fontManager.setCharSpacing(1);
			_fontManager.setSurface(&optionsSurface);
			resetScreen = false;
		}
		if (drawState > 0) {
			if (drawState == 1) {
				optionsSurface.blitFrom(*bgFrame);
			}
			drawMenuTitle(&optionsSurface, 243);
			_fontManager.setForeColor(volumeForeColor);
			_fontManager.displayStr(550, 407, _messages[39]);
			optionsSurface.vLine(544, 402, 429, volumeForeColor);
			optionsSurface.hLine(544, 429, 613, volumeForeColor); // minus 1 because hLine draws inclusive

			boxes.reset();
			unsigned int boxId = 0;
			unsigned int top = 195;
			unsigned int bottom;
			unsigned int width;

			for (Common::Array<int>::iterator it = menuEntries.begin(); it != menuEntries.end(); it++) {
				if (*it == 30 && !ConfMan.getBool("subtitles")) {
					*it = 31;
				} else if (*it == 32 && (ConfMan.getBool("mute") ||
				                         ConfMan.getBool("music_mute"))) {
					*it = 33;
				}
#if 0
				else if (*it == 34) {
					// What to do with music on HDD setting?
				}
#endif
				else if (*it == 26 && !_isPlaying) {
					*it = -26;
				} else if (*it == 29 && !_isPlaying) {
					*it = -29;
				} else if (*it == -42 && canVisit()) {
					*it = 42;
				} else if (*it == 48) {
					unsigned int omni3D_speed = ConfMan.getInt("omni3d_speed");
					switch (omni3D_speed) {
					case 1:
						*it = 51;
						break;
					case 2:
						*it = 52;
						break;
					case 3:
						*it = 49;
						break;
					case 4:
						*it = 50;
						break;
					}
				}

				if (*it > 0) {
					int msgId = *it;
					bottom = top;
					top += 24;

					// Patch on the fly the text displayed
					if (_isVisiting) {
						if (msgId == 26) {
							msgId = 44;
						} else if (msgId == 29) {
							msgId = 45;
						}
					}

					width = _fontManager.getStrWidth(_messages[msgId]);
					//Common::Rect rct(144, top - 39, width + 144, bottom);
					//optionsSurface.frameRect(rct, 0);
					boxes.setupBox(boxId, 144, top - 39, width + 144, bottom);
					if (boxId == hoveredBox) {
						_fontManager.setForeColor(240);
					} else {
						_fontManager.setForeColor(243);
					}
					_fontManager.displayStr(144, top - 39, _messages[msgId]);
				}
				boxId++;
			}

			volumeBox = boxId;
			boxes.setupBox(boxId, 525, 101, 570, 401);
			optionsSurface.transBlitFrom(_sprites.getSurface(102), Common::Point(553, soundVolumeY),
			                             _sprites.getKeyColor(102));

			g_system->copyRectToScreen(optionsSurface.getPixels(), optionsSurface.pitch, 0, 0, optionsSurface.w,
			                           optionsSurface.h);
			drawState = 0;
		}
		g_system->updateScreen();

		if (pollEvents() || forceEvents) { // always call pollEvents
			forceEvents = false;
			Common::Point mouse = getMousePos();
			unsigned int boxId = 0;
			Common::Array<int>::iterator it;
			for (it = menuEntries.begin(); it != menuEntries.end(); it++) {
				if (boxes.hitTest(boxId, mouse)) {
					if (hoveredBox != boxId) {
						hoveredBox = boxId;
						drawState = 2;
					}
					// We met a hit, no need to look further
					break;
				}
				boxId++;
			}
			if (it != menuEntries.end()) {
				if (getDragStatus() == 2) {
					selectedMsg = *it;
					selectedBox = hoveredBox;
				}
			} else {
				// no menu selected, check volume
				if (boxes.hitTest(volumeBox, mouse)) {
					if (volumeForeColor != 240) {
						volumeForeColor = 240;
						drawState = 1;
					}
					if (getCurrentMouseButton() == 1) {
						if (soundVolumeY != getMousePos().y - volumeCursorMiddleY) {
							soundVolumeY = CLIP(getMousePos().y - volumeCursorMiddleY, 101u, 384u);
							drawState = 1;
							volume = CLIP(((384 - soundVolumeY) << 8) / 283, 0u, 256u);
							// Global setting
							ConfMan.setInt("music_volume", volume);
							ConfMan.setInt("speech_volume", volume);
							ConfMan.setInt("sfx_volume", volume);
							syncSoundSettings();
						}
					} else if (getDragStatus() == 2 &&
					           !_mixer->hasActiveChannelOfType(Audio::Mixer::kMusicSoundType) &&
					           _mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType) > 0) {
						// Finished dragging
						_mixer->stopID(SoundIds::kOrgue);
						do {
							Common::File *audioFile = new Common::File();
							if (!audioFile->open("ORGUE.WAV")) {
								warning("Failed to open sound file %s", "ORGUE.WAV");
								delete audioFile;
								break;
							}

							Audio::SeekableAudioStream *audioDecoder = Audio::makeWAVStream(audioFile, DisposeAfterUse::YES);
							// We lost ownership of the audioFile just set it to nullptr and don't use it
							audioFile = nullptr;
							if (!audioDecoder) {
								break;
							}

							_mixer->playStream(Audio::Mixer::kSFXSoundType, nullptr, audioDecoder, SoundIds::kOrgue);
							// We lost ownership of the audioDecoder just set it to nullptr and don't use it
							audioDecoder = nullptr;
						} while (false);
					}
				} else {
					if (hoveredBox != -1u) {
						hoveredBox = -1;
						drawState = 2;
					}
					if (volumeForeColor != 243) {
						volumeForeColor = 243;
						drawState = 1;
					}
				}
			}
			if (getNextKey().keycode == Common::KEYCODE_ESCAPE && _isPlaying) {
				selectedMsg = 26;
			}
			if (selectedMsg == 27 || selectedMsg == 28 || selectedMsg == 40 || selectedMsg == 42) {
				// New game, Load game, Quit, Visit
				if (!_isPlaying || _isVisiting) {
					end = true;
				} else {
					end = displayYesNoBox(optionsSurface, Common::Rect(235, 420, 505, 465), 57);
				}
				drawState = 1;
				if (end) {
					_isPlaying = false;
				} else {
					selectedMsg = 0;
				}
			}
			if (selectedMsg == 25) {
				// Documentation area
				_docManager.handleDocArea();
				drawState = 1;
				resetScreen = true;
				forceEvents = true;
				waitMouseRelease();
				selectedMsg = 0;
			} else if (selectedMsg == 26) {
				// Continue game
				end = true;
			} else if (selectedMsg == 28) {
				Common::String saveName;
				bool wasVisiting = _isVisiting;
				_isVisiting = false;
				unsigned int saveNumber = displayFilePicker(bgFrame, false, saveName);
				if (saveNumber == -1u) {
					_isVisiting = wasVisiting;
					drawState = 1;
					selectedMsg = 0;
				} else {
					_loadedSave = saveNumber;
					_isPlaying = false;
					end = true;
				}
				waitMouseRelease();
			} else if (selectedMsg == 42) {
				Common::String saveName;
				bool wasVisiting = _isVisiting;
				_isVisiting = true;
				unsigned int saveNumber = displayFilePicker(bgFrame, false, saveName);
				if (saveNumber == -1u) {
					_isVisiting = wasVisiting;
					drawState = 1;
					selectedMsg = 0;
				} else {
					_loadedSave = saveNumber;
					_isPlaying = false;
					end = true;
				}
				waitMouseRelease();
			} else if (selectedMsg == 29) {
				Common::String saveName;
				unsigned int saveNumber = displayFilePicker(bgFrame, true, saveName);
				if (saveNumber != -1u) {
					saveGame(_isVisiting, saveNumber, saveName);
				}
				drawState = 1;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 30) {
				ConfMan.setBool("subtitles", false);
				drawState = 1;
				menuEntries[selectedBox] = 31;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 31) {
				ConfMan.setBool("subtitles", true);
				drawState = 1;
				menuEntries[selectedBox] = 30;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 32) {
				ConfMan.setBool("music_mute", true);
				syncSoundSettings();
				drawState = 1;
				menuEntries[selectedBox] = 33;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 33) {
				ConfMan.setBool("mute", false);
				ConfMan.setBool("music_mute", false);
				syncSoundSettings();
				drawState = 1;
				menuEntries[selectedBox] = 32;
				selectedMsg = 0;
				waitMouseRelease();
			}
#if 0
			// Music on disk settings
			else if (selectedMsg == 35) {
				drawState = 1;
				menuEntries[selectedBox] = 34;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 34) {
				drawState = 1;
				menuEntries[selectedBox] = 36;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 36) {
				drawState = 1;
				menuEntries[selectedBox] = 35;
				selectedMsg = 0;
				waitMouseRelease();
			}
#endif
			else if (selectedMsg == 39) {
				// Volume
				selectedMsg = 0;
			} else if (selectedMsg == 47) {
				// Unknown
				selectedMsg = 0;
			} else if (selectedMsg == 48) {
				ConfMan.setInt("omni3d_speed", 1);
				drawState = 1;
				menuEntries[selectedBox] = 51;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 51) {
				ConfMan.setInt("omni3d_speed", 2);
				drawState = 1;
				menuEntries[selectedBox] = 52;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 52) {
				ConfMan.setInt("omni3d_speed", 3);
				drawState = 1;
				menuEntries[selectedBox] = 49;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 49) {
				ConfMan.setInt("omni3d_speed", 4);
				drawState = 1;
				menuEntries[selectedBox] = 50;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 50) {
				ConfMan.setInt("omni3d_speed", 0);
				drawState = 1;
				menuEntries[selectedBox] = 48;
				selectedMsg = 0;
				waitMouseRelease();
			} else if (selectedMsg == 43) {
				displayCredits();
				drawState = 1;
				resetScreen = true;
				forceEvents = true;
				selectedMsg = 0;
				waitMouseRelease();
			}
		}
	}

	g_system->showMouse(false);

	if (selectedMsg == 42) {
		_abortCommand = AbortLoadGame;
		// For return value
		selectedMsg = 28;
	} else if (selectedMsg == 28) {
		_abortCommand = AbortLoadGame;
	} else if (selectedMsg == 40) {
		_abortCommand = AbortQuit;
	} else if (selectedMsg == 27) {
		_abortCommand = AbortNewGame;
		_isVisiting = false;
	} else if (g_engine->shouldQuit()) {
		// Fake a quit
		selectedMsg = 40;
		_abortCommand = AbortQuit;
	}

	ConfMan.flushToDisk();
	syncOmni3DSettings();
	musicUpdate();

	delete imageDecoder;
	return selectedMsg;
}

unsigned int CryOmni3DEngine_Versailles::displayYesNoBox(Graphics::ManagedSurface &surface,
        const Common::Rect &position, unsigned int msg_id) {
	unsigned int confirmWidth = _fontManager.getStrWidth(_messages[53]);
	unsigned int cancelWidth = _fontManager.getStrWidth(_messages[54]);
	unsigned int oldFont = _fontManager.getCurrentFont();

	_fontManager.setSurface(&surface);
	_fontManager.setForeColor(240);
	_fontManager.setLineHeight(20);
	surface.frameRect(position, 243);

	_fontManager.setupBlock(Common::Rect(position.left + 5, position.top + 5, position.right - 5,
	                                     position.bottom - 5));
	_fontManager.setCurrentFont(5);
	_fontManager.displayBlockText(_messages[msg_id]);
	_fontManager.setCurrentFont(3);

	MouseBoxes boxes(2);
	boxes.setupBox(1, position.left + 5, position.bottom - 15, position.left + confirmWidth,
	               position.bottom, &_messages[53]);
	boxes.setupBox(0, position.right - cancelWidth - 5, position.bottom - 15, position.right,
	               position.bottom, &_messages[54]);

	bool end = false;
	bool redraw = true;
	unsigned int result = -1u;

	while (!end || redraw) {
		if (redraw) {
			for (unsigned int boxId = 0; boxId < 2; boxId++) {
				if (boxId == result) {
					_fontManager.setForeColor(240);
				} else {
					_fontManager.setForeColor(243);
				}
				boxes.display(boxId, _fontManager);
			}
			redraw = false;

			g_system->copyRectToScreen(surface.getPixels(), surface.pitch, 0, 0, surface.w, surface.h);
		}
		g_system->updateScreen();

		if (pollEvents()) {
			Common::Point mouse = getMousePos();
			unsigned int hit_result = -1u;
			if (boxes.hitTest(1, mouse)) {
				hit_result = 1;
			} else if (boxes.hitTest(0, mouse)) {
				hit_result = 0;
			}
			if (!end && hit_result != result) {
				result = hit_result;
				redraw = true;
			}
			if ((getCurrentMouseButton() == 1) && (result != -1u)) {
				end = true;
			}
			Common::KeyCode keyPressed = getNextKey().keycode;
			if (keyPressed == Common::KEYCODE_ESCAPE) {
				result = 0;
				redraw = true;
				end = true;
			} else if (keyPressed == Common::KEYCODE_RETURN) {
				result = 1;
				redraw = true;
				end = true;
			}
		}
	}
	_fontManager.setCurrentFont(oldFont);
	return result;
}

unsigned int CryOmni3DEngine_Versailles::displayFilePicker(const Graphics::Surface *bgFrame,
        bool saveMode, Common::String &saveName) {
	Graphics::ManagedSurface surface(bgFrame->w, bgFrame->h, bgFrame->format);
	surface.blitFrom(*bgFrame);

	drawMenuTitle(&surface, 243);

	int subtitleId;
	if (_isVisiting) {
		subtitleId = saveMode ? 45 : 46;
	} else {
		subtitleId = saveMode ? 29 : 28;
	}
	_fontManager.displayStr(164, 214, _messages[subtitleId]);

	// Draw an empty screen before we list saves
	g_system->showMouse(false);
	g_system->copyRectToScreen(surface.getPixels(), surface.pitch, 0, 0, surface.w, surface.h);
	g_system->updateScreen();

	Common::Array<Common::String> savesList;
	getSavesList(_isVisiting, savesList);
	Common::String saveNameBackup;

	g_system->showMouse(true);

	MouseBoxes boxes(10); // 6 files + Yes/No/Up/Down buttons

	// Yes/No buttons
	const Common::String &okMsg = _messages[53];
	unsigned int okWidth = _fontManager.getStrWidth(okMsg);
	boxes.setupBox(6, 246, 430, 246 + okWidth, 450, &okMsg);
	const Common::String &cancelMsg = _messages[54];
	unsigned int cancelWidth = _fontManager.getStrWidth(cancelMsg);
	boxes.setupBox(7, 146, 430, 146 + cancelWidth, 450, &cancelMsg);

	// Up/Down buttons
	boxes.setupBox(8, 428, 320, 448, 340);
	boxes.setupBox(9, 428, 360, 448, 380);
	surface.transBlitFrom(_sprites.getSurface(162), Common::Point(428, 320), _sprites.getKeyColor(162));
	surface.transBlitFrom(_sprites.getSurface(185), Common::Point(428, 360), _sprites.getKeyColor(185));

	setCursor(181);

	unsigned int fileListOffset = 0; // TODO: store in config

	unsigned int boxHovered = -1;
	unsigned int boxSelected = -1;

	bool textCursorState = false;
	unsigned int textCursorNextState = 0;
	unsigned int textCursorPos = -1;

	bool autoRepeatInhibit = false;
	unsigned int autoRepeatDelay = 250;
	unsigned int autoRepeatEndInhibit = 0;

	bool finished = false;
	bool filesListChanged = true;
	bool redraw = false;
	while (!finished) {
		if (filesListChanged || redraw) {
			if (filesListChanged) {
				for (unsigned int file = 0, fileY = 280; file < 6; file++, fileY += 20) {
					boxes.setupBox(file, 146, fileY, 408, fileY + 14, &savesList[file + fileListOffset]);
				}
				// Redraw background as file list changed
				surface.blitFrom(*bgFrame, Common::Rect(116, 280, 408, 400), Common::Point(116, 280));
				filesListChanged = false;
			}
			// Don't redraw the scroll buttons
			for (unsigned int box = 0; box < 8; box++) {
				if (box == boxSelected) {
					// Selected
					_fontManager.setForeColor(240);
				} else if (box == 6 && boxSelected == -1u) {
					// Ok and no file selected
					_fontManager.setForeColor(245);
				} else if (box == boxHovered) {
					// Hovered
					_fontManager.setForeColor(241);
				} else {
					// Other cases
					_fontManager.setForeColor(243);
				}

				if (box == boxSelected && saveMode) {
					Common::Rect boxRct = boxes.getBoxRect(box);
					boxRct.top -= 2;
					surface.blitFrom(*bgFrame, boxRct, Common::Point(boxRct.left, boxRct.top));
					boxRct.top += 2;
					if (textCursorState) {
						surface.vLine(textCursorPos, boxRct.top, boxRct.top + 11, 240);
					}
				}
				boxes.display(box, _fontManager);
				if (box < 6) {
					// Draw line below
					surface.hLine(116, 280 + box * 20 + 15, 407, 243); // minus 1 because hLine draws inclusive

					// Display file number
					_fontManager.displayInt(126, 280 + box * 20, fileListOffset + box + 1);
				}
			}
			redraw = false;
			g_system->copyRectToScreen(surface.getPixels(), surface.pitch, 0, 0, surface.w, surface.h);
		}

		g_system->updateScreen();
		pollEvents();
		Common::KeyState key = getNextKey();
		unsigned int mousePressed = getCurrentMouseButton();

		if (!mousePressed) {
			bool boxFound = false;
			// Don't handle scroll arrows hovering
			for (unsigned int box = 0; box < 8; box++) {
				if (boxes.hitTest(box, getMousePos())) {
					boxFound = true;
					if (boxHovered != box) {
						boxHovered = box;
						redraw = true;
					}
				}
			}
			if (!boxFound && boxHovered != -1u) {
				boxHovered = -1;
				redraw = true;
			}
		}
		if (key == Common::KEYCODE_RETURN || (mousePressed == 1 && boxHovered == 6)) {
			// OK
			if (boxSelected != -1u) {
				Common::String &selectedSaveName = savesList[boxSelected + fileListOffset];
				if (!selectedSaveName.size()) {
					selectedSaveName = _messages[56]; // No name
				}
				redraw = true;
				finished = true;
			}
		} else if (mousePressed == 1) {
			if (boxHovered == 7) {
				// Cancel
				boxSelected = -1;
				finished = true;
			} else if (boxHovered != -1u && boxHovered != boxSelected) {
				// This can only be a file
				bool existingSave = (savesList[boxHovered + fileListOffset] != _messages[55]);
				// Don't allow to save on slot 0 when visiting to avoid problems with original visit save
				bool validSave = !(_isVisiting && saveMode && boxSelected == 0);
				if ((saveMode || existingSave) && validSave) {
					// Restore old name
					if (saveMode && boxSelected != -1u) {
						savesList[boxSelected + fileListOffset] = saveNameBackup;
						filesListChanged = true;
					}
					boxSelected = boxHovered;
					// Backup new one
					saveNameBackup = savesList[boxSelected + fileListOffset];
					// Not an existing save clear free name
					if (!existingSave) {
						savesList[boxSelected + fileListOffset] = "";
					}
					redraw = true;
				}
			}
		}
		if (boxSelected != -1u && saveMode) {
			if (key.keycode != Common::KEYCODE_INVALID) {
				// Reference means we edit in place
				Common::String &selectedSaveName = savesList[boxSelected + fileListOffset];
				if (key == Common::KEYCODE_BACKSPACE && selectedSaveName.size() > 0) {
					selectedSaveName.deleteLastChar();
					textCursorNextState = 0;
					redraw = true;
				} else if (key.ascii > 32 && key.ascii < 256 && selectedSaveName.size() < 20) {
					selectedSaveName += key.ascii;
					textCursorNextState = 0;
					redraw = true;
				}
			}
			if (g_system->getMillis() > textCursorNextState) {
				textCursorNextState = g_system->getMillis() + 200; // Blink at 200ms period
				unsigned int width = _fontManager.getStrWidth(savesList[boxSelected + fileListOffset]);
				Common::Rect boxRct = boxes.getBoxRect(boxSelected);
				textCursorPos = boxRct.left + width;
				textCursorState = !textCursorState;
				redraw = true;
			}
		}
		if (!autoRepeatInhibit) {
			bool autoRepeatTrigger = false;
			unsigned int oldFileListOffset = fileListOffset;
			if (mousePressed) {
				if (boxes.hitTest(8, getMousePos()) && fileListOffset > 0) {
					fileListOffset--;
					autoRepeatTrigger = true;
				} else if (boxes.hitTest(9, getMousePos()) && fileListOffset < 99 - 6) {
					fileListOffset++;
					autoRepeatTrigger = true;
				}
			} else if (key == Common::KEYCODE_UP) {
				if (fileListOffset > 0) {
					fileListOffset--;
					autoRepeatTrigger = true;
				}
			} else if (key == Common::KEYCODE_DOWN) {
				if (fileListOffset < 99 - 6) {
					fileListOffset++;
					autoRepeatTrigger = true;
				}
			} else if (key == Common::KEYCODE_PAGEUP) {
				if (fileListOffset > 6) {
					fileListOffset -= 6;
				} else {
					fileListOffset = 0;
				}
			} else if (key == Common::KEYCODE_PAGEDOWN) {
				if (fileListOffset < 99 - 6 - 6) {
					fileListOffset += 6;
				} else {
					fileListOffset = 99 - 6;
				}
			}
			if (autoRepeatTrigger) {
				// Restore old name
				if (saveMode && boxSelected != -1u) {
					savesList[boxSelected + oldFileListOffset] = saveNameBackup;
				}
				boxHovered = -1;
				boxSelected = -1;
				autoRepeatInhibit = true;
				autoRepeatEndInhibit = g_system->getMillis() + autoRepeatDelay;
				filesListChanged = true;
			}
		}
		if (autoRepeatInhibit && g_system->getMillis() > autoRepeatEndInhibit) {
			autoRepeatInhibit = false;
			autoRepeatDelay = 60; // Next rounds will wait 60ms after first one
		}
		if (!mousePressed && key == Common::KEYCODE_INVALID) {
			// Nothing was clicked or pressed: set back autoRepeatDelay to 250ms
			autoRepeatDelay = 250;
		}
	}
	if (boxSelected != -1u) {
		saveName = savesList[boxSelected + fileListOffset];
		// TODO: save list offset
		return boxSelected + fileListOffset + 1;
	} else {
		return -1;
	}
}

const MsgBoxParameters CryOmni3DEngine_Versailles::kWarpMsgBoxParameters = {
	9, 241, 22, 2, 1, 36, 18, 20, 10, 5
};

const MsgBoxParameters CryOmni3DEngine_Versailles::kFixedimageMsgBoxParameters = {
	3, 241, 22, 2, 1, 40, 20, 20, 10, 3
};

void CryOmni3DEngine_Versailles::displayMessageBox(const MsgBoxParameters &params,
        const Graphics::Surface *surface, const Common::String &msg, const Common::Point &position,
        const Common::Functor0<void> &callback) {
	Graphics::ManagedSurface dstSurface;
	dstSurface.create(surface->w, surface->h, surface->format);
	dstSurface.blitFrom(*surface);

	_fontManager.setSurface(&dstSurface);
	_fontManager.setCurrentFont(params.font);
	_fontManager.setTransparentBackground(true);
	_fontManager.setForeColor(params.foreColor);
	_fontManager.setLineHeight(params.lineHeight);
	_fontManager.setSpaceWidth(params.spaceWidth);
	_fontManager.setCharSpacing(params.charSpacing);

	unsigned int width = params.initialWidth;
	unsigned int height = params.initialHeight;
	unsigned int lineCount = 0;
	Common::Point pt = position;
	Common::Rect rct;

	bool notEnough = true;
	bool tooLarge = false;

	while (notEnough && !tooLarge) {
		width += params.incrementWidth;
		height += params.incrementHeight;
		rct = Common::Rect::center(pt.x, pt.y, width, height);
		if (rct.left < 10) {
			rct.left = 10;
			if (pt.x < 320) {
				pt.x += 10;
			}
		}
		if (rct.right >= 630) {
			rct.right = 630;
			if (pt.x > 320) {
				pt.x -= 10;
			}
		}
		if (rct.top <= 10) {
			rct.top = 10;
			if (pt.y < 240) {
				pt.y += 10;
			}
		}
		if (rct.bottom >= 470) {
			rct.bottom = 470;
			if (pt.y > 235) { // sic.
				pt.y -= 10;
			}
		}
		if (rct.left == 10 && rct.top == 10 && rct.right == 630 && rct.bottom == 470) {
			tooLarge = true;
		}
		lineCount = _fontManager.getLinesCount(msg, rct.width() - 12);
		if (lineCount && lineCount * _fontManager.lineHeight() + 18 < (unsigned int)rct.height()) {
			notEnough = false;
		}
	}
	rct.setHeight(lineCount * _fontManager.lineHeight() + 12);
	if (rct.bottom > 479) {
		rct.bottom = 479;
	}

	Graphics::Surface subSurface = dstSurface.getSubArea(rct);
	makeTranslucent(subSurface, surface->getSubArea(rct));
	rct.grow(-6);
	_fontManager.setupBlock(rct);
	_fontManager.displayBlockText(msg);
	// TODO: countdown

	g_system->copyRectToScreen(dstSurface.getPixels(), dstSurface.pitch, 0, 0,
	                           dstSurface.w, dstSurface.h);

	waitMouseRelease();
	unsigned int disappearTime = g_system->getMillis() + msg.size() * params.timeoutChar * 10;
	bool finished = false;
	while (!finished) {
		g_system->updateScreen();

		callback();

		if (g_system->getMillis() > disappearTime) {
			finished = true;
		}
		if (getCurrentMouseButton() == 1) {
			finished = true;
		}
	}

	// Restore image
	g_system->copyRectToScreen(surface->getPixels(), surface->pitch, 0, 0, surface->w, surface->h);
}

void CryOmni3DEngine_Versailles::displayMessageBoxWarp(const Common::String &message) {
	Common::Point mousePos = getMousePos();
	mousePos += Common::Point(0, 32);
	if (mousePos.x > 639) {
		mousePos.x = 639;
	}
	if (mousePos.y > 479) {
		mousePos.y = 479;
	}
	displayMessageBox(kWarpMsgBoxParameters, _omni3dMan.getSurface(), message, mousePos,
	                  Common::Functor0Mem<void, CryOmni3DEngine_Versailles>(this,
	                          &CryOmni3DEngine_Versailles::warpMsgBoxCB));
}

void CryOmni3DEngine_Versailles::displayCredits() {
	waitMouseRelease();

	Graphics::ManagedSurface creditsSurface;
	Image::ImageDecoder *imageDecoder = loadHLZ("credits.hlz");
	if (!imageDecoder) {
		return;
	}

	const Graphics::Surface *bgFrame = imageDecoder->getSurface();

	byte palette[256 * 3];
	memset(palette, 0, 256 * 3);
	// getPalette returns the first color not index 0
	memcpy(palette + 3 * imageDecoder->getPaletteStartIndex(), imageDecoder->getPalette(),
	       3 * imageDecoder->getPaletteColorCount());
	copySubPalette(palette, _cursorPalette, 240, 8);

	creditsSurface.create(bgFrame->w, bgFrame->h, bgFrame->format);

	_fontManager.setCurrentFont(3);
	_fontManager.setTransparentBackground(true);
	_fontManager.setForeColor(243);
	_fontManager.setLineHeight(14);
	_fontManager.setSpaceWidth(0);
	_fontManager.setCharSpacing(1);
	_fontManager.setSurface(&creditsSurface);

	Common::File creditsFile;
	if (!creditsFile.open("credits.txt")) {
		warning("Failed to open credits file: %s", "credits.txt");
		delete imageDecoder;
		return;
	}

	g_system->showMouse(false);

	char line[256];
	bool end = false;
	bool calculatedScreen = false;
	unsigned int lineHeight = 20;
	unsigned int currentY = 0;
	int32 fileOffset = 0;
	bool skipScreen = false;

	while (!end && creditsFile.readLine(line, ARRAYSIZE(line))) {
		// Remove line ending
		line[strlen(line) - 1] = '\0';
		if (!strncmp(line, "###", 3)) {
			// Prefix for commands
			if (!strncmp(line + 3, "ECRAN", 5)) {
				// ECRAN command
				if (calculatedScreen) {
					g_system->copyRectToScreen(creditsSurface.getPixels(), creditsSurface.pitch, 0, 0,
					                           creditsSurface.w, creditsSurface.h);
					if (skipScreen) {
						// Just display palette
						setPalette(palette, 0, 256);
					} else {
						fadeInPalette(palette);
					}
					skipScreen = false;
					// Wait
					unsigned int endScreenTime = g_system->getMillis() + 6000;
					while (g_system->getMillis() < endScreenTime && !skipScreen) {
						g_system->updateScreen();
						if (pollEvents()) {
							if (getCurrentMouseButton() == 1) {
								skipScreen = true;
							}
							Common::KeyCode kc = getNextKey().keycode;
							while (kc != Common::KEYCODE_INVALID) {
								if (kc == Common::KEYCODE_SPACE) {
									skipScreen = true;
									break;
								} else if (kc == Common::KEYCODE_ESCAPE) {
									skipScreen = true;
									end = true;
									break;
								}
								kc = getNextKey().keycode;
							}
							clearKeys();
						}
						if (g_engine->shouldQuit()) {
							skipScreen = true;
							end = true;
						}
					}
					if (!skipScreen) {
						fadeOutPalette();
						fillSurface(0);
					}
					currentY = 0;
					fileOffset = creditsFile.pos();
					calculatedScreen = false;
				} else {
					// We just finished calculated all lines, roll back and display them
					creditsFile.seek(fileOffset, SEEK_SET);
					calculatedScreen = true;
					if (currentY <= 480 - lineHeight) {
						// Center in screen
						currentY = (480 - lineHeight) / 2 - currentY / 2;
					} else {
						currentY = 3;
					}
					creditsSurface.blitFrom(*bgFrame);
				}
			} else if (!strcmp(line + 3, "T0")) {
				_fontManager.setCurrentFont(1);
				lineHeight = _fontManager.getFontMaxHeight() + 10;
			} else if (!strcmp(line + 3, "T1")) {
				_fontManager.setCurrentFont(2);
				lineHeight = _fontManager.getFontMaxHeight() + 10;
			} else if (!strcmp(line + 3, "T2")) {
				_fontManager.setCurrentFont(4);
				lineHeight = _fontManager.getFontMaxHeight() + 10;
			} else if (!strcmp(line + 3, "T3")) {
				_fontManager.setCurrentFont(2);
				lineHeight = _fontManager.getFontMaxHeight() + 10;
			} else if (!strcmp(line + 3, "T4")) {
				_fontManager.setCurrentFont(5);
				lineHeight = _fontManager.getFontMaxHeight() + 10;
			} else if (!strcmp(line + 3, "T5")) {
				_fontManager.setCurrentFont(6);
				lineHeight = _fontManager.getFontMaxHeight() + 10;
			} else {
				warning("Unknown ### command : %s", line + 3);
			}
		} else {
			// Text
			if (calculatedScreen) {
				unsigned int width = _fontManager.getStrWidth(line);
				// Center around 315
				_fontManager.displayStr(315 - width / 2, currentY, line);
			}
			currentY += lineHeight;
		}
	}
	g_system->showMouse(true);
}

} // End of namespace Versailles
} // End of namespace CryOmni3D