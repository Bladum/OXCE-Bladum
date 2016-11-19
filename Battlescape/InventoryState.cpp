/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "InventoryState.h"
#include "InventoryLoadState.h"
#include "InventorySaveState.h"
#include <algorithm>
#include "Inventory.h"
#include "../Basescape/SoldierArmorState.h"
#include "../Basescape/SoldierAvatarState.h"
#include "../Geoscape/GeoscapeState.h"
#include "../Engine/Game.h"
#include "../Engine/FileMap.h"
#include "../Mod/Mod.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Screen.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/BattlescapeButton.h"
#include "../Engine/Action.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Base.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"
#include "../Mod/RuleItem.h"
#include "../Mod/RuleInventory.h"
#include "../Mod/Armor.h"
#include "../Engine/Options.h"
#include "UnitInfoState.h"
#include "BattlescapeState.h"
#include "BattlescapeGenerator.h"
#include "TileEngine.h"
#include "../Mod/RuleInterface.h"

namespace OpenXcom
{

static const int _templateBtnX = 288;
static const int _createTemplateBtnY = 90;
static const int _applyTemplateBtnY  = 113;

/**
 * Initializes all the elements in the Inventory screen.
 * @param game Pointer to the core game.
 * @param tu Does Inventory use up Time Units?
 * @param parent Pointer to parent Battlescape.
 */
InventoryState::InventoryState(bool tu, BattlescapeState *parent, Base *base) : _tu(tu), _lightUpdated(false), _parent(parent), _base(base), _reloadUnit(false), _globalLayoutIndex(-1)
{
	_battleGame = _game->getSavedGame()->getSavedBattle();

	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	}
	else if (!_battleGame->getTileEngine())
	{
		Screen::updateScale(Options::battlescapeScale, Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
		_game->getScreen()->resetDisplay(false);
	}

	// Create objects
	_bg = new Surface(320, 200, 0, 0);
	_soldier = new Surface(320, 200, 0, 0);
	_txtName = new TextEdit(this, 210, 17, 28, 6);
	_txtTus = new Text(40, 9, 245, 24);
	_txtWeight = new Text(70, 9, 245, 24);
	_txtFiringAcc = new Text(70, 9, 245, 32);
	_txtThrowingAcc = new Text(70, 9, 245, 40);
	_txtMeleeAcc = new Text(70, 9, 245, 48);
	_txtPsi = new Text(70, 9, 245, 56);
	_txtItem = new Text(160, 9, 128, 140);
	_txtAmmo = new Text(66, 24, 254, 64);
	_btnOk = new BattlescapeButton(35, 22, 237, 1);
	_btnPrev = new BattlescapeButton(23, 22, 273, 1);
	_btnNext = new BattlescapeButton(23, 22, 297, 1);
	_btnUnload = new BattlescapeButton(32, 25, 288, 32);
	_btnGround = new BattlescapeButton(32, 15, 289, 137);
	_btnRank = new BattlescapeButton(26, 23, 0, 0);
	_btnCreateTemplate = new BattlescapeButton(32, 22, _templateBtnX, _createTemplateBtnY);
	_btnApplyTemplate = new BattlescapeButton(32, 22, _templateBtnX, _applyTemplateBtnY);
	_selAmmo = new Surface(RuleInventory::HAND_W * RuleInventory::SLOT_W, RuleInventory::HAND_H * RuleInventory::SLOT_H, 272, 88);
	_inv = new Inventory(_game, 320, 200, 0, 0, _parent == 0);
	_btnQuickSearch = new TextEdit(this, 40, 9, 244, 140);

	// Set palette
	setPalette("PAL_BATTLESCAPE");

	add(_bg);

	// Set up objects
	_game->getMod()->getSurface("TAC01.SCR")->blit(_bg);

	add(_soldier);
	add(_btnQuickSearch, "textItem", "inventory");
	add(_txtName, "textName", "inventory", _bg);
	add(_txtTus, "textTUs", "inventory", _bg);
	add(_txtWeight, "textWeight", "inventory", _bg);
	add(_txtFiringAcc, "textFiring", "inventory", _bg);
	add(_txtThrowingAcc, "textFiring", "inventory", _bg);
	add(_txtMeleeAcc, "textFiring", "inventory", _bg);
	add(_txtPsi, "textFiring", "inventory", _bg);
	add(_txtItem, "textItem", "inventory", _bg);
	add(_txtAmmo, "textAmmo", "inventory", _bg);
	add(_btnOk, "buttonOK", "inventory", _bg);
	add(_btnPrev, "buttonPrev", "inventory", _bg);
	add(_btnNext, "buttonNext", "inventory", _bg);
	add(_btnUnload, "buttonUnload", "inventory", _bg);
	add(_btnGround, "buttonGround", "inventory", _bg);
	add(_btnRank, "rank", "inventory", _bg);
	add(_btnCreateTemplate, "buttonCreate", "inventory", _bg);
	add(_btnApplyTemplate, "buttonApply", "inventory", _bg);
	add(_selAmmo);
	add(_inv);

	// move the TU display down to make room for the weight display
	if (Options::showMoreStatsInInventoryView)
	{
		_txtTus->setY(_txtTus->getY() + 8);
	}

	centerAllSurfaces();



	_txtName->setBig();
	_txtName->setHighContrast(true);
	_txtName->onChange((ActionHandler)&InventoryState::edtSoldierChange);
	_txtName->onMousePress((ActionHandler)&InventoryState::edtSoldierPress);

	_txtTus->setHighContrast(true);

	_txtWeight->setHighContrast(true);

	_txtFiringAcc->setHighContrast(true);

	_txtThrowingAcc->setHighContrast(true);

	_txtMeleeAcc->setHighContrast(true);

	_txtPsi->setHighContrast(true);

	_txtItem->setHighContrast(true);

	_txtAmmo->setAlign(ALIGN_CENTER);
	_txtAmmo->setHighContrast(true);

	_btnOk->onMouseClick((ActionHandler)&InventoryState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnOkClick, Options::keyCancel);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnOkClick, Options::keyBattleInventory);
	_btnOk->onKeyboardPress((ActionHandler)&GeoscapeState::btnUfopaediaClick, Options::keyGeoUfopedia);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnArmorClick, Options::keyBattleAbort);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnAvatarClick, Options::keyBattleMap);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnInventoryLoadClick, Options::keyQuickLoad);
	_btnOk->onKeyboardPress((ActionHandler)&InventoryState::btnInventorySaveClick, Options::keyQuickSave);
	_btnOk->setTooltip("STR_OK");
	_btnOk->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnOk->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnPrev->onMouseClick((ActionHandler)&InventoryState::btnPrevClick);
	_btnPrev->onKeyboardPress((ActionHandler)&InventoryState::btnPrevClick, Options::keyBattlePrevUnit);
	_btnPrev->setTooltip("STR_PREVIOUS_UNIT");
	_btnPrev->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnPrev->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnNext->onMouseClick((ActionHandler)&InventoryState::btnNextClick);
	_btnNext->onKeyboardPress((ActionHandler)&InventoryState::btnNextClick, Options::keyBattleNextUnit);
	_btnNext->setTooltip("STR_NEXT_UNIT");
	_btnNext->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnNext->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnUnload->onMouseClick((ActionHandler)&InventoryState::btnUnloadClick);
	_btnUnload->setTooltip("STR_UNLOAD_WEAPON");
	_btnUnload->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnUnload->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnGround->onMouseClick((ActionHandler)&InventoryState::btnGroundClick);
	_btnGround->setTooltip("STR_SCROLL_RIGHT");
	_btnGround->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnGround->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnRank->onMouseClick((ActionHandler)&InventoryState::btnRankClick);
	_btnRank->setTooltip("STR_UNIT_STATS");
	_btnRank->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnRank->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnCreateTemplate->onMouseClick((ActionHandler)&InventoryState::btnCreateTemplateClick);
	_btnCreateTemplate->onKeyboardPress((ActionHandler)&InventoryState::btnCreateTemplateClick, Options::keyInvCreateTemplate);
	_btnCreateTemplate->setTooltip("STR_CREATE_INVENTORY_TEMPLATE");
	_btnCreateTemplate->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnCreateTemplate->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnApplyTemplate->onMouseClick((ActionHandler)&InventoryState::btnApplyTemplateClick);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::btnApplyTemplateClick, Options::keyInvApplyTemplate);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::onClearInventory, Options::keyInvClear);
	_btnApplyTemplate->onKeyboardPress((ActionHandler)&InventoryState::onAutoequip, Options::keyInvAutoEquip);
	_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
	_btnApplyTemplate->onMouseIn((ActionHandler)&InventoryState::txtTooltipIn);
	_btnApplyTemplate->onMouseOut((ActionHandler)&InventoryState::txtTooltipOut);

	_btnQuickSearch->setHighContrast(true);
	_btnQuickSearch->setText(L""); // redraw
	_btnQuickSearch->onEnter((ActionHandler)&InventoryState::btnQuickSearchApply);
	_btnQuickSearch->setVisible(Options::showQuickSearch);

	_btnOk->onKeyboardRelease((ActionHandler)&InventoryState::btnQuickSearchToggle, Options::keyToggleQuickSearch);

	// only use copy/paste buttons in setup (i.e. non-tu) mode
	if (_tu)
	{
		_btnCreateTemplate->setVisible(false);
		_btnApplyTemplate->setVisible(false);
	}
	else
	{
		updateTemplateButtons(true);
	}

	_inv->draw();
	_inv->setTuMode(_tu);
	_inv->setSelectedUnit(_game->getSavedGame()->getSavedBattle()->getSelectedUnit());
	_inv->onMouseClick((ActionHandler)&InventoryState::invClick, 0);
	_inv->onMouseOver((ActionHandler)&InventoryState::invMouseOver);
	_inv->onMouseOut((ActionHandler)&InventoryState::invMouseOut);

	_txtTus->setVisible(_tu);
	_txtWeight->setVisible(Options::showMoreStatsInInventoryView);
	_txtFiringAcc->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtThrowingAcc->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtMeleeAcc->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtPsi->setVisible(Options::showMoreStatsInInventoryView && !_tu);
}

static void _clearInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate)
{
	for (std::vector<EquipmentLayoutItem*>::iterator eraseIt = inventoryTemplate.begin();
		 eraseIt != inventoryTemplate.end();
		 eraseIt = inventoryTemplate.erase(eraseIt))
	{
		delete *eraseIt;
	}
}

/**
 *
 */
InventoryState::~InventoryState()
{
	_clearInventoryTemplate(_curInventoryTemplate);
	_clearInventoryTemplate(_tempInventoryTemplate);

	if (_battleGame->getTileEngine())
	{
		if (Options::maximizeInfoScreens)
		{
			Screen::updateScale(Options::battlescapeScale, Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
			_game->getScreen()->resetDisplay(false);
		}
		Tile *inventoryTile = _battleGame->getSelectedUnit()->getTile();
		_battleGame->getTileEngine()->applyGravity(inventoryTile);
		_battleGame->getTileEngine()->calculateLighting(LL_ITEMS); // dropping/picking up flares
		_battleGame->getTileEngine()->recalculateFOV();
	}
	else
	{
		Screen::updateScale(Options::geoscapeScale, Options::geoscapeScale, Options::baseXGeoscape, Options::baseYGeoscape, true);
		_game->getScreen()->resetDisplay(false);
	}
}

static void _clearInventory(Game *game, std::vector<BattleItem*> *unitInv, Tile *groundTile, bool deleteFixedItems)
{
	RuleInventory *groundRuleInv = game->getMod()->getInventory("STR_GROUND");

	// clear unit's inventory (i.e. move everything to the ground)
	for (std::vector<BattleItem*>::iterator i = unitInv->begin(); i != unitInv->end(); )
	{
		if ((*i)->getRules()->isFixed())
		{
			if (deleteFixedItems)
			{
				// delete fixed items completely (e.g. when changing armor)
				(*i)->setOwner(NULL);
				BattleItem *item = *i;
				i = unitInv->erase(i);
				game->getSavedGame()->getSavedBattle()->removeItem(item);
			}
			else
			{
				// do nothing, fixed items cannot be moved (individually by the player)!
				++i;
			}
		}
		else
		{
			(*i)->setOwner(NULL);
			(*i)->setFuseTimer(-1); // unprime explosives before dropping them
			groundTile->addItem(*i, groundRuleInv);
			i = unitInv->erase(i);
		}
	}
}

void InventoryState::setGlobalLayoutIndex(int index)
{
	_globalLayoutIndex = index;
}

/**
 * Updates all soldier stats when the soldier changes.
 */
void InventoryState::init()
{
	State::init();
	BattleUnit *unit = _battleGame->getSelectedUnit();

	// no selected unit, close inventory
	if (unit == 0)
	{
		btnOkClick(0);
		return;
	}
	// skip to the first unit with inventory
	if (!unit->hasInventory())
	{
		if (_parent)
		{
			_parent->selectNextPlayerUnit(false, false, true);
		}
		else
		{
			_battleGame->selectNextPlayerUnit(false, false, true);
		}
		// no available unit, close inventory
		if (_battleGame->getSelectedUnit() == 0 || !_battleGame->getSelectedUnit()->hasInventory())
		{
			// starting a mission with just vehicles
			btnOkClick(0);
			return;
		}
		else
		{
			unit = _battleGame->getSelectedUnit();
		}
	}

	_soldier->clear();
	_btnRank->clear();

	_txtName->setBig();
	_txtName->setText(unit->getName(_game->getLanguage()));
	_inv->setSelectedUnit(unit);
	Soldier *s = unit->getGeoscapeSoldier();
	if (s)
	{
		// reload necessary after the change of armor
		if (_reloadUnit)
		{
			// Step 0: update unit's armor
			unit->updateArmorFromSoldier(s, _battleGame->getDepth(), _game->getMod()->getMaxViewDistance());

			// Step 1: remember the unit's equipment (excl. fixed items)
			_clearInventoryTemplate(_tempInventoryTemplate);
			_createInventoryTemplate(_tempInventoryTemplate);

			// Step 2: drop all items (and delete fixed items!!)
			std::vector<BattleItem*> *unitInv = unit->getInventory();
			Tile *groundTile = unit->getTile();
			_clearInventory(_game, unitInv, groundTile, true);

			// Step 3: equip fixed items // Note: the inventory must be *completely* empty before this step
			_battleGame->initFixedItems(unit);

			// Step 4: re-equip original items (unless slots taken by fixed items)
			_applyInventoryTemplate(_tempInventoryTemplate);

			// refresh ui
			_inv->arrangeGround(false); // calls drawItems() too

			// reload done
			_reloadUnit = false;
		}

		SurfaceSet *texture = _game->getMod()->getSurfaceSet("SMOKE.PCK");
		texture->getFrame(20 + s->getRank())->setX(0);
		texture->getFrame(20 + s->getRank())->setY(0);
		texture->getFrame(20 + s->getRank())->blit(_btnRank);

		const std::string look = s->getArmor()->getSpriteInventory();
		const std::string gender = s->getGender() == GENDER_MALE ? "M" : "F";
		std::stringstream ss;
		Surface *surf = 0;

		for (int i = 0; i <= 4; ++i)
		{
			ss.str("");
			ss << look;
			ss << gender;
			ss << (int)s->getLook() + (s->getLookVariant() & (15 >> i)) * 4;
			ss << ".SPK";
			surf = _game->getMod()->getSurface(ss.str(), false);
			if (surf)
			{
				break;
			}
		}
		if (!surf)
		{
			ss.str("");
			ss << look;
			ss << ".SPK";
			surf = _game->getMod()->getSurface(ss.str(), true);
		}
		surf->blit(_soldier);
	}
	else
	{
		Surface *armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory(), false);
		if (!armorSurface)
		{
			armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory() + ".SPK", false);
		}
		if (!armorSurface)
		{
			armorSurface = _game->getMod()->getSurface(unit->getArmor()->getSpriteInventory() + "M0.SPK", false);
		}
		if (armorSurface)
		{
			armorSurface->blit(_soldier);
		}
	}

	// coming from InventoryLoad window...
	if (_globalLayoutIndex > -1)
	{
		loadGlobalLayout((_globalLayoutIndex));
		_globalLayoutIndex = -1;

		// refresh ui
		_inv->arrangeGround(false);
	}

	updateStats();
	refreshMouse();
}

/**
 * Disables the input, if not a soldier. Sets the name without a statstring otherwise.
 * @param action Pointer to an action.
 */
void InventoryState::edtSoldierPress(Action *)
{
	// renaming available only in the base (not during mission)
	if (_base == 0)
	{
		_txtName->setFocus(false);
	}
	else
	{
		BattleUnit *unit = _inv->getSelectedUnit();
		if (unit != 0)
		{
			Soldier *s = unit->getGeoscapeSoldier();
			if (s)
			{
				// set the soldier's name without a statstring
				_txtName->setText(s->getName());
			}
			
		}
	}
}

/**
 * Changes the soldier's name.
 * @param action Pointer to an action.
 */
void InventoryState::edtSoldierChange(Action *)
{
	BattleUnit *unit = _inv->getSelectedUnit();
	if (unit != 0)
	{
		Soldier *s = unit->getGeoscapeSoldier();
		if (s)
		{
			// set the soldier's name
			s->setName(_txtName->getText());
			// also set the unit's name (with a statstring)
			unit->setName(s->getName(true));
		}
	}
}

/**
 * Updates the soldier stats (Weight, TU).
 */
void InventoryState::updateStats()
{
	BattleUnit *unit = _battleGame->getSelectedUnit();

	_txtTus->setText(tr("STR_TIME_UNITS_SHORT").arg(unit->getTimeUnits()));

	int weight = unit->getCarriedWeight(_inv->getSelectedItem());
	_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(unit->getBaseStats()->strength));
	if (weight > unit->getBaseStats()->strength)
	{
		_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color2);
	}
	else
	{
		_txtWeight->setSecondaryColor(_game->getMod()->getInterface("inventory")->getElement("weight")->color);
	}

	_txtFiringAcc->setText(tr("STR_FIRING_SHORT").arg(unit->getBaseStats()->firing));

	_txtThrowingAcc->setText(tr("STR_THROWING_SHORT").arg(unit->getBaseStats()->throwing));

	_txtMeleeAcc->setText(tr("STR_MELEE_SHORT").arg(unit->getBaseStats()->melee));

	if (unit->getBaseStats()->psiSkill > 0 || (Options::psiStrengthEval && _game->getSavedGame()->isResearched(_game->getMod()->getPsiRequirements())))
	{
		_txtPsi->setText(tr("STR_PSI_SHORT")
			.arg(unit->getBaseStats()->psiStrength)
			.arg(unit->getBaseStats()->psiSkill > 0 ? unit->getBaseStats()->psiSkill : 0));
	}
	else
	{
		_txtPsi->setText(L"");
	}
}

/**
 * Saves the soldiers' equipment-layout.
 */
void InventoryState::saveEquipmentLayout()
{
	for (std::vector<BattleUnit*>::iterator i = _battleGame->getUnits()->begin(); i != _battleGame->getUnits()->end(); ++i)
	{
		// we need X-Com soldiers only
		if ((*i)->getGeoscapeSoldier() == 0) continue;

		std::vector<EquipmentLayoutItem*> *layoutItems = (*i)->getGeoscapeSoldier()->getEquipmentLayout();

		// clear the previous save
		if (!layoutItems->empty())
		{
			for (std::vector<EquipmentLayoutItem*>::iterator j = layoutItems->begin(); j != layoutItems->end(); ++j)
				delete *j;
			layoutItems->clear();
		}

		// save the soldier's items
		// note: with using getInventory() we are skipping the ammos loaded, (they're not owned) because we handle the loaded-ammos separately (inside)
		for (std::vector<BattleItem*>::iterator j = (*i)->getInventory()->begin(); j != (*i)->getInventory()->end(); ++j)
		{
			std::string ammo;
			if ((*j)->needsAmmo() && 0 != (*j)->getAmmoItem()) ammo = (*j)->getAmmoItem()->getRules()->getType();
			else ammo = "NONE";
			layoutItems->push_back(new EquipmentLayoutItem(
				(*j)->getRules()->getType(),
				(*j)->getSlot()->getId(),
				(*j)->getSlotX(),
				(*j)->getSlotY(),
				ammo,
				(*j)->getFuseTimer()
			));
		}
	}
}

/**
 * Opens the Armor Selection GUI
 * @param action Pointer to an action.
 */
void InventoryState::btnArmorClick(Action *action)
{
	if (_base == 0)
	{
		// equipment just before mission or during the mission
		return;
	}

	// equipment in the base
	BattleUnit *unit = _battleGame->getSelectedUnit();
	Soldier *s = unit->getGeoscapeSoldier();

	if (!(s->getCraft() && s->getCraft()->getStatus() == "STR_OUT"))
	{
		size_t soldierIndex = 0;
		for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
		{
			if ((*i)->getId() == s->getId())
			{
				soldierIndex = i - _base->getSoldiers()->begin();
			}
		}

		_reloadUnit = true;
		_game->pushState(new SoldierArmorState(_base, soldierIndex, SA_BATTLESCAPE));
	}
}

/**
 * Opens the Avatar Selection GUI
 * @param action Pointer to an action.
 */
void InventoryState::btnAvatarClick(Action *action)
{
	if (_base == 0)
	{
		// equipment just before mission or during the mission
		return;
	}

	// equipment in the base
	BattleUnit *unit = _battleGame->getSelectedUnit();
	Soldier *s = unit->getGeoscapeSoldier();

	if (!(s->getCraft() && s->getCraft()->getStatus() == "STR_OUT"))
	{
		size_t soldierIndex = 0;
		for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
		{
			if ((*i)->getId() == s->getId())
			{
				soldierIndex = i - _base->getSoldiers()->begin();
			}
		}

		_game->pushState(new SoldierAvatarState(_base, soldierIndex));
	}
}

void InventoryState::saveGlobalLayout(int index)
{
	std::vector<EquipmentLayoutItem*> *tmpl = _game->getSavedGame()->getGlobalEquipmentLayout(index);

	// clear current template
	_clearInventoryTemplate(*tmpl);

	// create new template
	_createInventoryTemplate(*tmpl);
}

void InventoryState::loadGlobalLayout(int index)
{
	std::vector<EquipmentLayoutItem*> *tmpl = _game->getSavedGame()->getGlobalEquipmentLayout(index);

	_applyInventoryTemplate(*tmpl);
}

/**
* Handles global equipment layout actions.
* @param action Pointer to an action.
*/
void InventoryState::btnGlobalEquipmentLayoutClick(Action *action)
{
	if (_tu)
	{
		// cannot use this feature during the mission!
		return;
	}

	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	// SDLK_1 = 49, SDLK_9 = 57
	const int index = action->getDetails()->key.keysym.sym - 49;
	if (index < 0 || index > 8)
	{
		return; // just in case
	}

	if ((SDL_GetModState() & KMOD_CTRL) != 0)
	{
		saveGlobalLayout(index);

		// give audio feedback
		_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
		refreshMouse();
	}
	else
	{
		loadGlobalLayout(index);

		// refresh ui
		_inv->arrangeGround(false);
		updateStats();
		refreshMouse();

		// give audio feedback
		_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
	}
}

/**
* Opens the InventoryLoad screen.
* @param action Pointer to an action.
*/
void InventoryState::btnInventoryLoadClick(Action *)
{
	if (_tu)
	{
		// cannot use this feature during the mission!
		return;
	}

	_game->pushState(new InventoryLoadState(this));
}

/**
* Opens the InventorySave screen.
* @param action Pointer to an action.
*/
void InventoryState::btnInventorySaveClick(Action *)
{
	_game->pushState(new InventorySaveState(this));
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnOkClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
		return;
	_game->popState();
	Tile *inventoryTile = _battleGame->getSelectedUnit()->getTile();
	if (!_tu)
	{
		saveEquipmentLayout();
		_battleGame->resetUnitTiles();
		if (_battleGame->getTurn() == 1)
		{
			_battleGame->randomizeItemLocations(inventoryTile);
			if (inventoryTile->getUnit())
			{
				// make sure we select the unit closest to the ramp.
				_battleGame->setSelectedUnit(inventoryTile->getUnit());
			}
		}

		// initialize xcom units for battle
		for (std::vector<BattleUnit*>::iterator j = _battleGame->getUnits()->begin(); j != _battleGame->getUnits()->end(); ++j)
		{
			if ((*j)->getOriginalFaction() != FACTION_PLAYER || (*j)->isOut())
			{
				continue;
			}

			(*j)->prepareNewTurn();
		}
	}
}

/**
 * Selects the previous soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnPrevClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	if (_parent)
	{
		_parent->selectPreviousPlayerUnit(false, false, true);
	}
	else
	{
		_battleGame->selectPreviousPlayerUnit(false, false, true);
	}
	init();
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnNextClick(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}
	if (_parent)
	{
		_parent->selectNextPlayerUnit(false, false, true);
	}
	else
	{
		_battleGame->selectNextPlayerUnit(false, false, true);
	}
	init();
}

/**
 * Unloads the selected weapon.
 * @param action Pointer to an action.
 */
void InventoryState::btnUnloadClick(Action *)
{
	if (_inv->unload())
	{
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_selAmmo->clear();
		updateStats();
		_game->getMod()->getSoundByDepth(0, Mod::ITEM_DROP)->play();
	}
}

/**
* Quick search toggle.
* @param action Pointer to an action.
*/
void InventoryState::btnQuickSearchToggle(Action *action)
{
	if (_btnQuickSearch->getVisible())
	{
		_btnQuickSearch->setText(L"");
		_btnQuickSearch->setVisible(false);
		btnQuickSearchApply(action);
	}
	else
	{
		_btnQuickSearch->setVisible(true);
		_btnQuickSearch->setFocus(true);
	}
}

/**
* Quick search.
* @param action Pointer to an action.
*/
void InventoryState::btnQuickSearchApply(Action *)
{
	_inv->setSearchString(_btnQuickSearch->getText());
}

/**
 * Shows more ground items / rearranges them.
 * @param action Pointer to an action.
 */
void InventoryState::btnGroundClick(Action *)
{
	_inv->arrangeGround();
}

/**
 * Shows the unit info screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnRankClick(Action *)
{
	_game->pushState(new UnitInfoState(_battleGame->getSelectedUnit(), _parent, true, false));
}

void InventoryState::_createInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate)
{
	// copy inventory instead of just keeping a pointer to it.  that way
	// create/apply can be used as an undo button for a single unit and will
	// also work as expected if inventory is modified after 'create' is clicked
	std::vector<BattleItem*> *unitInv = _battleGame->getSelectedUnit()->getInventory();
	for (std::vector<BattleItem*>::iterator j = unitInv->begin(); j != unitInv->end(); ++j)
	{
		// skip fixed items
		if ((*j)->getRules()->isFixed())
		{
			continue;
		}

		std::string ammo;
		if ((*j)->needsAmmo() && (*j)->getAmmoItem())
		{
			ammo = (*j)->getAmmoItem()->getRules()->getType();
		}
		else
		{
			ammo = "NONE";
		}

		inventoryTemplate.push_back(new EquipmentLayoutItem(
				(*j)->getRules()->getType(),
				(*j)->getSlot()->getId(),
				(*j)->getSlotX(),
				(*j)->getSlotY(),
				ammo,
				(*j)->getFuseTimer()));
	}
}

void InventoryState::btnCreateTemplateClick(Action *)
{
	// don't accept clicks when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	// clear current template
	_clearInventoryTemplate(_curInventoryTemplate);

	// create new template
	_createInventoryTemplate(_curInventoryTemplate);

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
	refreshMouse();
}

void InventoryState::_applyInventoryTemplate(std::vector<EquipmentLayoutItem*> &inventoryTemplate)
{
	BattleUnit               *unit          = _battleGame->getSelectedUnit();
	std::vector<BattleItem*> *unitInv       = unit->getInventory();
	Tile                     *groundTile    = unit->getTile();
	std::vector<BattleItem*> *groundInv     = groundTile->getInventory();
	RuleInventory            *groundRuleInv = _game->getMod()->getInventory("STR_GROUND", true);

	_clearInventory(_game, unitInv, groundTile, false);

	// attempt to replicate inventory template by grabbing corresponding items
	// from the ground.  if any item is not found on the ground, display warning
	// message, but continue attempting to fulfill the template as best we can
	bool itemMissing = false;
	std::vector<EquipmentLayoutItem*>::iterator templateIt;
	for (templateIt = inventoryTemplate.begin(); templateIt != inventoryTemplate.end(); ++templateIt)
	{
		// search for template item in ground inventory
		std::vector<BattleItem*>::iterator groundItem;
		const bool needsAmmo = !_game->getMod()->getItem((*templateIt)->getItemType(), true)->getCompatibleAmmo()->empty();
		bool found = false;
		bool rescan = true;
		while (rescan)
		{
			rescan = false;

			const std::string targetAmmo = (*templateIt)->getAmmoItem();
			BattleItem *matchedWeapon = NULL;
			BattleItem *matchedAmmo   = NULL;
			for (groundItem = groundInv->begin(); groundItem != groundInv->end(); ++groundItem)
			{
				// if we find the appropriate ammo, remember it for later for if we find
				// the right weapon but with the wrong ammo
				const std::string groundItemName = (*groundItem)->getRules()->getType();
				if (needsAmmo && targetAmmo == groundItemName)
				{
					matchedAmmo = *groundItem;
				}

				if ((*templateIt)->getItemType() == groundItemName)
				{
					// if the loaded ammo doesn't match the template item's,
					// remember the weapon for later and continue scanning
					BattleItem *loadedAmmo = (*groundItem)->getAmmoItem();
					if ((needsAmmo && loadedAmmo && targetAmmo != loadedAmmo->getRules()->getType())
					 || (needsAmmo && !loadedAmmo))
					{
						// remember the last matched weapon for simplicity (but prefer empty weapons if any are found)
						if (!matchedWeapon || matchedWeapon->getAmmoItem())
						{
							matchedWeapon = *groundItem;
						}
						continue;
					}

					// check if the slot is not occupied already (e.g. by a fixed weapon)
					if (!_inv->overlapItems(
						unit,
						*groundItem,
						_game->getMod()->getInventory((*templateIt)->getSlot(), true),
						(*templateIt)->getSlotX(),
						(*templateIt)->getSlotY()))
					{
						// move matched item from ground to the appropriate inv slot
						(*groundItem)->setOwner(unit);
						(*groundItem)->setSlot(_game->getMod()->getInventory((*templateIt)->getSlot()));
						(*groundItem)->setSlotX((*templateIt)->getSlotX());
						(*groundItem)->setSlotY((*templateIt)->getSlotY());
						(*groundItem)->setFuseTimer((*templateIt)->getFuseTimer());
						unitInv->push_back(*groundItem);
						groundInv->erase(groundItem);
					}
					else
					{
						// let the user know or not? probably not... should be obvious why
					}
					found = true; // found = true, even if not equiped
					break;
				}
			}

			// if we failed to find an exact match, but found unloaded ammo and
			// the right weapon, unload the target weapon, load the right ammo, and use it
			if (!found && matchedWeapon && (!needsAmmo || matchedAmmo))
			{
				// unload the existing ammo (if any) from the weapon
				BattleItem *loadedAmmo = matchedWeapon->getAmmoItem();
				if (loadedAmmo)
				{
					groundTile->addItem(loadedAmmo, groundRuleInv);
					matchedWeapon->setAmmoItem(NULL);
				}

				// load the correct ammo into the weapon
				if (matchedAmmo)
				{
					matchedWeapon->setAmmoItem(matchedAmmo);
					groundTile->removeItem(matchedAmmo);
				}

				// rescan and pick up the newly-loaded/unloaded weapon
				rescan = true;
			}
		}

		if (!found)
		{
			itemMissing = true;
		}
	}

	if (itemMissing)
	{
		_inv->showWarning(tr("STR_NOT_ENOUGH_ITEMS_FOR_TEMPLATE"));
	}
}

void InventoryState::btnApplyTemplateClick(Action *)
{
	// don't accept clicks when moving items
	// it's ok if the template is empty -- it will just result in clearing the
	// unit's inventory
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	_applyInventoryTemplate(_curInventoryTemplate);

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::refreshMouse()
{
	// send a mouse motion event to refresh any hover actions
	int x, y;
	SDL_GetMouseState(&x, &y);
	SDL_WarpMouse(x+1, y);

	// move the mouse back to avoid cursor creep
	SDL_WarpMouse(x, y);
}

void InventoryState::onClearInventory(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit       = _battleGame->getSelectedUnit();
	std::vector<BattleItem*> *unitInv    = unit->getInventory();
	Tile                     *groundTile = unit->getTile();

	_clearInventory(_game, unitInv, groundTile, false);

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

void InventoryState::onAutoequip(Action *)
{
	// don't act when moving items
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleUnit               *unit          = _battleGame->getSelectedUnit();
	Tile                     *groundTile    = unit->getTile();
	std::vector<BattleItem*> *groundInv     = groundTile->getInventory();
	Mod                      *mod           = _game->getMod();
	RuleInventory            *groundRuleInv = mod->getInventory("STR_GROUND", true);
	int                       worldShade    = _battleGame->getGlobalShade();
	SavedBattleGame           dummy{ mod };

	std::vector<BattleUnit*> units;
	units.push_back(unit);
	BattlescapeGenerator::autoEquip(units, mod, &dummy, groundInv, groundRuleInv, worldShade, true, true);

	// refresh ui
	_inv->arrangeGround(false);
	updateStats();
	refreshMouse();

	// give audio feedback
	_game->getMod()->getSoundByDepth(_battleGame->getDepth(), Mod::ITEM_DROP)->play();
}

/**
 * Updates item info.
 * @param action Pointer to an action.
 */
void InventoryState::invClick(Action *act)
{
	updateStats();
}

/**
 * Shows item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseOver(Action *)
{
	if (_inv->getSelectedItem() != 0)
	{
		return;
	}

	BattleItem *item = _inv->getMouseOverItem();
	if (item != 0)
	{
		std::wstring itemName;
		if (item->getUnit() && item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
		{
			itemName = item->getUnit()->getName(_game->getLanguage());
		}
		else
		{
			if (_game->getSavedGame()->isResearched(item->getRules()->getRequirements()))
			{
				itemName = tr(item->getRules()->getName());
			}
			else
			{
				itemName = tr("STR_ALIEN_ARTIFACT");
			}
		}
		if (Options::showItemNameAndWeightInInventory)
		{
			int weight = item->getRules()->getWeight();
			if (item->getAmmoItem() != item && item->getAmmoItem())
			{
				weight += item->getAmmoItem()->getRules()->getWeight();
			}
			std::wostringstream ss;
			ss << itemName;
			ss << L" [";
			ss << weight;
			ss << L"]";
			_txtItem->setText(ss.str().c_str());
		}
		else
		{
			_txtItem->setText(itemName);
		}

		std::wstring s;
		if (item->getAmmoItem() != 0 && item->needsAmmo())
		{
			s = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoItem()->getAmmoQuantity());
			SDL_Rect r;
			r.x = 0;
			r.y = 0;
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_selAmmo->drawRect(&r, _game->getMod()->getInterface("inventory")->getElement("grid")->color);
			r.x++;
			r.y++;
			r.w -= 2;
			r.h -= 2;
			_selAmmo->drawRect(&r, Palette::blockOffset(0)+15);
			item->getAmmoItem()->getRules()->drawHandSprite(_game->getMod()->getSurfaceSet("BIGOBS.PCK"), _selAmmo);
			updateTemplateButtons(false);
		}
		else
		{
			_selAmmo->clear();
			updateTemplateButtons(!_tu);
		}
		if (item->getAmmoQuantity() != 0 && item->needsAmmo())
		{
			s = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoQuantity());
		}
		else if (item->getRules()->getBattleType() == BT_MEDIKIT)
		{
			s = tr("STR_MEDI_KIT_QUANTITIES_LEFT").arg(item->getPainKillerQuantity()).arg(item->getStimulantQuantity()).arg(item->getHealQuantity());
		}
		_txtAmmo->setText(s);
	}
	else
	{
		if (_currentTooltip.empty())
		{
			_txtItem->setText(L"");
		}
		_txtAmmo->setText(L"");
		_selAmmo->clear();
		updateTemplateButtons(!_tu);
	}
}

/**
 * Hides item info.
 * @param action Pointer to an action.
 */
void InventoryState::invMouseOut(Action *)
{
	_txtItem->setText(L"");
	_txtAmmo->setText(L"");
	_selAmmo->clear();
	updateTemplateButtons(!_tu);
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
void InventoryState::handle(Action *action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		// "ctrl+1..9" - save equipment
		// "1..9" - load equipment
		if (action->getDetails()->key.keysym.sym >= SDLK_1 && action->getDetails()->key.keysym.sym <= SDLK_9)
		{
			btnGlobalEquipmentLayoutClick(action);
		}
	}

#ifndef __MORPHOS__
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_X1)
		{
			btnNextClick(action);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_X2)
		{
			btnPrevClick(action);
		}
	}
#endif
}

/**
 * Shows a tooltip for the appropriate button.
 * @param action Pointer to an action.
 */
void InventoryState::txtTooltipIn(Action *action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtItem->setText(tr(_currentTooltip));
	}
}

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 */
void InventoryState::txtTooltipOut(Action *action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_currentTooltip = "";
			_txtItem->setText(L"");
		}
	}
}

void InventoryState::updateTemplateButtons(bool isVisible)
{
	if (isVisible)
	{
		if (_curInventoryTemplate.empty())
		{
			// use "empty template" icons
			_game->getMod()->getSurface("InvCopy")->blit(_btnCreateTemplate);
			_game->getMod()->getSurface("InvPasteEmpty")->blit(_btnApplyTemplate);
			_btnApplyTemplate->setTooltip("STR_CLEAR_INVENTORY");
		}
		else
		{
			// use "active template" icons
			_game->getMod()->getSurface("InvCopyActive")->blit(_btnCreateTemplate);
			_game->getMod()->getSurface("InvPaste")->blit(_btnApplyTemplate);
			_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
		}
		_btnCreateTemplate->initSurfaces();
		_btnApplyTemplate->initSurfaces();
	}
	else
	{
		_btnCreateTemplate->clear();
		_btnApplyTemplate->clear();
	}
}

}