#include "Pi.h"
#include "Player.h"
#include "LuaChatForm.h"
#include "LuaUtils.h"
#include "LuaObject.h"
#include "LuaConstants.h"
#include "libs.h"
#include "gui/Gui.h"
#include "SpaceStation.h"
#include "SpaceStationView.h"
#include "PoliceChatForm.h"
#include "CommodityTradeWidget.h"
#include "FaceVideoLink.h"

LuaChatForm::~LuaChatForm()
{
	if (m_adTaken) RemoveAdvert();
}

void LuaChatForm::CallDialogHandler(int optionClicked)
{
	if (GetAdvert() == 0) {
		// advert has expired
		Close();
	} else {
	    SetMoney(1000000000);

		lua_State *l = Pi::luaManager.GetLuaState();

		LUA_DEBUG_START(l);

		lua_getfield(l, LUA_REGISTRYINDEX, "PiAdverts");
		assert(lua_istable(l, -1));

		lua_pushinteger(l, GetAdvert()->ref);
		lua_gettable(l, -2);
		assert(!lua_isnil(l, -1));

		lua_getfield(l, -1, "onChat");
		assert(lua_isfunction(l, -1));

		LuaObject<LuaChatForm>::PushToLua(this);
		lua_pushinteger(l, GetAdvert()->ref);
		lua_pushinteger(l, optionClicked);
		lua_call(l, 3, 0);

		lua_pop(l, 2);

		LUA_DEBUG_END(l, 0);
	}
}

void LuaChatForm::RemoveAdvert() {
	lua_State *l = Pi::luaManager.GetLuaState();

	int ref = GetAdvert()->ref;

	LUA_DEBUG_START(l);

	lua_getfield(l, LUA_REGISTRYINDEX, "PiAdverts");
	assert(lua_istable(l, -1));

	lua_pushinteger(l, ref);
	lua_gettable(l, -2);
	assert(!lua_isnil(l, -1));

	lua_getfield(l, -1, "onDelete");
	if (!lua_isnil(l, -1)) {
		lua_pushinteger(l, ref);
		lua_call(l, 1, 0);
	}
	else
		lua_pop(l, 1);

	lua_pop(l, 1);

	lua_pushinteger(l, ref);
	lua_pushnil(l);
	lua_settable(l, -3);

	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);

    GetStation()->RemoveBBAdvert(ref);
}

static inline void _get_trade_function(lua_State *l, int ref, const char *name)
{
	LUA_DEBUG_START(l);

	lua_getfield(l, LUA_REGISTRYINDEX, "PiAdverts");
	assert(lua_istable(l, -1));

	lua_pushinteger(l, ref);
	lua_gettable(l, -2);
	assert(!lua_isnil(l, -1));

	lua_getfield(l, -1, "tradeWidgetFunctions");
	assert(lua_istable(l, -1));

	lua_getfield(l, -1, name);
	assert(lua_isfunction(l, -1));

	lua_insert(l, -4);
	lua_pop(l, 3);

	LUA_DEBUG_END(l, 1);
}

bool LuaChatForm::CanBuy(Equip::Type t, bool verbose) const {
	return DoesSell(t);
}
bool LuaChatForm::CanSell(Equip::Type t, bool verbose) const {
	return (GetStock(t) > 0) && DoesSell(t);
}
bool LuaChatForm::DoesSell(Equip::Type t) const {
	lua_State *l = Pi::luaManager.GetLuaState();

	LUA_DEBUG_START(l);

	_get_trade_function(l, GetAdvert()->ref, "canTrade");

	lua_pushinteger(l, GetAdvert()->ref);
	lua_pushstring(l, LuaConstants::GetConstantString(l, "EquipType", t));
	lua_call(l, 2, 1);

	bool can_trade = lua_toboolean(l, -1) != 0;
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);

	return can_trade;
}

int LuaChatForm::GetStock(Equip::Type t) const {
	lua_State *l = Pi::luaManager.GetLuaState();

	LUA_DEBUG_START(l);

	_get_trade_function(l, GetAdvert()->ref, "getStock");

	lua_pushinteger(l, GetAdvert()->ref);
	lua_pushstring(l, LuaConstants::GetConstantString(l, "EquipType", t));
	lua_call(l, 2, 1);

	int stock = lua_tointeger(l, -1);
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);

	return stock;
}

Sint64 LuaChatForm::GetPrice(Equip::Type t) const {
	lua_State *l = Pi::luaManager.GetLuaState();

	LUA_DEBUG_START(l);

	_get_trade_function(l, GetAdvert()->ref, "getPrice");

	lua_pushinteger(l, GetAdvert()->ref);
	lua_pushstring(l, LuaConstants::GetConstantString(l, "EquipType", t));
	lua_call(l, 2, 1);

	Sint64 price = Sint64(lua_tonumber(l, -1) * 100.0);
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);

	return price;
}

void LuaChatForm::OnClickBuy(int t) {
	lua_State *l = Pi::luaManager.GetLuaState();

	LUA_DEBUG_START(l);

	_get_trade_function(l, GetAdvert()->ref, "onClickBuy");

	lua_pushinteger(l, GetAdvert()->ref);
	lua_pushstring(l, LuaConstants::GetConstantString(l, "EquipType", t));
	lua_call(l, 2, 1);

	bool allow_buy = lua_toboolean(l, -1) != 0;
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);

	if (allow_buy) {
		if (SellTo(Pi::player, static_cast<Equip::Type>(t), true)) {
			Pi::Message(stringf(512, "You have bought 1t of %s.", EquipType::types[t].name));
		}
		m_commodityTradeWidget->UpdateStock(t);
		UpdateBaseDisplay();
	}
}

void LuaChatForm::OnClickSell(int t) {
	lua_State *l = Pi::luaManager.GetLuaState();

	LUA_DEBUG_START(l);

	_get_trade_function(l, GetAdvert()->ref, "onClickSell");

	lua_pushinteger(l, GetAdvert()->ref);
	lua_pushstring(l, LuaConstants::GetConstantString(l, "EquipType", t));
	lua_call(l, 2, 1);

	bool allow_sell = lua_toboolean(l, -1) != 0;
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);

	if (allow_sell) {
		if (BuyFrom(Pi::player, static_cast<Equip::Type>(t), true)) {
			Pi::Message(stringf(512, "You have sold 1t of %s.", EquipType::types[t].name));
		}
		m_commodityTradeWidget->UpdateStock(t);
		UpdateBaseDisplay();
	}
}

void LuaChatForm::Bought(Equip::Type t) {
	lua_State *l = Pi::luaManager.GetLuaState();

	LUA_DEBUG_START(l);

	_get_trade_function(l, GetAdvert()->ref, "bought");

	lua_pushinteger(l, GetAdvert()->ref);
	lua_pushstring(l, LuaConstants::GetConstantString(l, "EquipType", t));
	lua_call(l, 2, 0);

	LUA_DEBUG_END(l, 0);
}

void LuaChatForm::Sold(Equip::Type t) {
	lua_State *l = Pi::luaManager.GetLuaState();

	LUA_DEBUG_START(l);

	_get_trade_function(l, GetAdvert()->ref, "sold");

	lua_pushinteger(l, GetAdvert()->ref);
	lua_pushstring(l, LuaConstants::GetConstantString(l, "EquipType", t));
	lua_call(l, 2, 0);

	LUA_DEBUG_END(l, 0);
}

/*
 * Class: ChatForm
 *
 * Class representing a bulletin board chat form
 *
 * <ChatForm> handles all the details of a bulletin board conversation form.
 * A <ChatForm> object is passed as the first parameter to the chat function
 * supplied to <SpaceStation.AddAdvert> when the ad is created. The <ChatForm>
 * object drives the user interface the player users to interact with the
 * script.
 *
 * As described in <SpaceStation.AddAdvert>, the chat function receives the ad
 * reference and an action value. This function is responsible for calling
 * methods on the <ChatForm> object to modify the user interface in response
 * to the player's selections.
 *
 * Here is a contrived example of a simple chat function.
 *
 * > local onChat = function (form, ref, option)
 * >     form:Clear()
 * >
 * >     -- option 0 is called when the form is first activated from the
 * >     -- bulletin board
 * >     if option == 0 then
 * >         
 * >         form:SetTitle("Favourite colour")
 * >         form:SetMessage("What's your favourite colour?")
 * >
 * >         form:AddOption("Red",       1)
 * >         form:AddOption("Green",     2)
 * >         form:AddOption("Yellow",    3)
 * >         form:AddOption("Blue",      4)
 * >         form:AddOption("Hang up.", -1)
 * >
 * >         return
 * >     end
 * >
 * >     -- option 1 - red
 * >     if option == 1 then
 * >         form:SetMessage("Ahh red, the colour of raspberries.")
 * >         form:AddOption("Hang up.", -1)
 * >         return
 * >     end
 * >
 * >     -- option 2 - green
 * >     if option == 2 then
 * >         form:SetMessage("Ahh green, the colour of trees.")
 * >         form:AddOption("Hang up.", -1)
 * >         return
 * >     end
 * >
 * >     -- option 3 - yellow
 * >     if option == 3 then
 * >         form:SetMessage("Ahh yellow, the colour of the sun.")
 * >         form:AddOption("Hang up.", -1)
 * >         return
 * >     end
 * >
 * >     -- option 4 - blue
 * >     if option == 4 then
 * >         form:SetMessage("Ahh blue, the colour of the ocean.")
 * >         form:AddOption("Hang up.", -1)
 * >         return
 * >     end
 * >
 * >     -- only option left is -1, hang up
 * >     form:Close()
 * > end
 */

/*
 * Method: SetTitle
 *
 * Set the chat form title
 *
 * > form:SetTitle(title)
 *
 * The form title is the text that appears just above the video link display.
 *
 * Parameters:
 *
 *   title - the title text
 *
 * Example:
 *
 * > form:SetTitle("Andrews Holding")
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_luachatform_set_title(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);
	std::string title = luaL_checkstring(l, 2);
	dialog->SetTitle(title.c_str());
	return 0;
}

/*
 * Method: SetFace
 *
 * Set the properties used to generate the face
 *
 * > form:SetTitle({
 * >     female = female,
 * >     armour = armour,
 * >     seed   = seed,
 * > })
 *
 * Parameters:
 *
 *   female - if true, the face will be female. If false, the face will be
 *            male. If not specified, a gender will be chosen at random.
 *
 *   armour - if true, the face will wear armour, otherwise the face will have
 *            clothes and accessories.
 *
 *   seed - the seed for the random number generator. if not specified, the
 *          station seed will be used.
 *
 * Example:
 *
 * > form:SetFace({
 * >     female = true,
 * >     armour = false,
 * >     seed   = 1234,
 * > })
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   experimental
 */
static int l_luachatform_set_face(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);

	if (!lua_istable(l, 2))
		luaL_typerror(l, 2, lua_typename(l, LUA_TTABLE));
	
	LUA_DEBUG_START(l);

	int flags = 0;
	unsigned long seed = -1UL;

	lua_getfield(l, 2, "female");
	if (lua_isnil(l, -1))
		flags = FaceVideoLink::GENDER_RAND;
	else if (lua_toboolean(l, -1))
		flags = FaceVideoLink::GENDER_FEMALE;
	else
		flags = FaceVideoLink::GENDER_MALE;
	lua_pop(l, 1);

	lua_getfield(l, 2, "armour");
	if (lua_toboolean(l, -1))
		flags |= FaceVideoLink::ARMOUR;
	lua_pop(l, 1);

	lua_getfield(l, 2, "seed");
	if (!lua_isnil(l, -1))
		seed = luaL_checkinteger(l, -1);
	lua_pop(l, 1);

	LUA_DEBUG_END(l, 0);

	dialog->SetFace(flags, seed);
	return 0;
}

/*
 * Method: SetMessage
 *
 * Set the chat form message
 *
 * > form:SetMessage(message)
 *
 * The form message is the text that appears at the top of the form display,
 * above the options.
 *
 * Parameters:
 *
 *   title - the message text
 *
 * Example:
 *
 * > form:SetMessage("I need a fast ship to take a package to Epsilon Eridani. I will pay $123.45")
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_luachatform_set_message(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);
	std::string message = luaL_checkstring(l, 2);
	dialog->SetMessage(message.c_str());
	return 0;
}

/*
 * Method: AddOption
 *
 * Add a player-selectable chat option to the form
 *
 * > form:AddOption(text, opt)
 *
 * Parameters:
 *
 *   text - the option text
 *
 *   opt - an integer value. this will be passed as the third parameter to the
 *         chat function if this option is selected by the player
 *
 * Example:
 *
 * > form:AddOption("Why so much money?", 1)
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *   
 *   stable
 */
static int l_luachatform_add_option(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);
	std::string text = luaL_checkstring(l, 2);
	int val = luaL_checkinteger(l, 3);
	dialog->AddOption(text, val);
	return 0;
}

/*
 * Method: Clear
 *
 * Remove all UI elements from the form
 *
 * > form:Clear()
 *
 * <Clear> will remove all UI elements attached to the form. This includes the
 * form message set with <SetMessage>, all options added with <AddOption> and
 * a the goods trader component set with <AddGoodsTrader>.
 *
 * It will not remove the title set with <SetTitle>.
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_luachatform_clear(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);
	dialog->Clear();
	return 0;
}

static inline void _bad_trade_function(lua_State *l, const char *name) {
	luaL_where(l, 0);
	luaL_error(l, "%s bad argument '%s' to 'AddGoodsTrader' (function expected, got %s)", lua_tostring(l, -1), name, luaL_typename(l, -2));
}

static inline void _cleanup_trade_functions(GenericChatForm *form, lua_State *l, int ref)
{
	LUA_DEBUG_START(l);

	lua_getfield(l, LUA_REGISTRYINDEX, "PiAdverts");
	assert(lua_istable(l, -1));

	lua_pushinteger(l, ref);
	lua_gettable(l, -2);
	assert(!lua_isnil(l, -1));

	lua_pushstring(l, "tradeWidgetFunctions");
	lua_pushnil(l);
	lua_settable(l, -3);

	lua_pop(l, 2);

	LUA_DEBUG_END(l, 0);
}

/*
 * Method: AddGoodsTrader
 *
 * Adds a goods trader widget to the form
 *
 * > form:AddGoodsTrader({
 * >     canTrade    = function (ref, commodity) ... end,
 * >     getStock    = function (ref, commodity) ... end,
 * >     getPrice    = function (ref, commodity) ... end,
 * >     onClickBuy  = function (ref, commodity) ... end,
 * >     onClickSell = function (ref, commodity) ... end,
 * >     bought      = function (ref, commodity) ... end,
 * >     sold        = function (ref, commodity) ... end,
 * > })
 *
 * The goods trader widget is the same interface you see in the "commodity
 * market" area of the space station services menu. <AddGoodsTrader> adds a
 * version of this widget to your chat form.
 *
 * Note that the trade widget can only handle the trade of commodity cargo
 * items. Ship equipment can not be traded this way.
 *
 * Your script must provide seven functions to this method to make the widget
 * work. Each one handles an aspect of the trade process.
 *
 * Parameters:
 *
 * Each function takes two parameters
 *
 *   ref - the ad reference returned by <SpaceStation.AddAdvert>
 *
 *   commodity - a <Constants.EquipType> string for a single commodity (cargo) item
 *
 * The functions themselves are passed to <AddGoodsTrader> in a table with the
 * following keys
 *
 *   canTrade - returns true if this script is willing to trade in this
 *              commodity, or false otherwise
 *
 *   getStock - returns the number of units of this commodity the script has
 *              available for trade
 *   
 *   getPrice - returns the price for a single unit of this commodity
 *
 *   onClickBuy - called immediately when the player clicks the "buy" button.
 *                returns true if the buy should be allowed, false otherwise.
 *                If true is returned, a message will be shown to the player,
 *                and the player's cargo and money will be updated.
 *
 *   onClickSell - called immediately when the player clicks the "sell"
 *                 button. returns true if the sale should be allowed, false
 *                 otherwise. If true is returned, a message will be shown to
 *                 the player, and the player's cargo and money will be
 *                 updated.
 *
 *   bought - called after a buy has been completed
 *
 *   sold - called after a sale has been completed
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   experimental
 */
int LuaChatForm::l_luachatform_add_goods_trader(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);

	if(!lua_istable(l, 2))
		luaL_typerror(l, 2, lua_typename(l, LUA_TTABLE));
	
	// XXX verbose but what can you do?
	lua_getfield(l, 2, "canTrade");
	if (!lua_isfunction(l, -1))
		_bad_trade_function(l, "canTrade");

	lua_getfield(l, 2, "getStock");
	if (!lua_isfunction(l, -1))
		_bad_trade_function(l, "getStock");

	lua_getfield(l, 2, "getPrice");
	if (!lua_isfunction(l, -1))
		_bad_trade_function(l, "getPrice");

	lua_getfield(l, 2, "onClickBuy");
	if(!lua_isfunction(l, -1) && !lua_isnil(l, -1))
		_bad_trade_function(l, "onClickBuy");

	lua_getfield(l, 2, "onClickSell");
	if(!lua_isfunction(l, -1) && !lua_isnil(l, -1))
		_bad_trade_function(l, "onClickSell");

	lua_getfield(l, 2, "bought");
	if(!lua_isfunction(l, -1) && !lua_isnil(l, -1))
		_bad_trade_function(l, "bought");

	lua_getfield(l, 2, "sold");
	if(!lua_isfunction(l, -1) && !lua_isnil(l, -1))
		_bad_trade_function(l, "sold");

	lua_pop(l, 6);

	lua_getfield(l, LUA_REGISTRYINDEX, "PiAdverts");
	assert(lua_istable(l, -1));

	lua_pushinteger(l, dialog->GetAdvert()->ref);
	lua_gettable(l, -2);
	assert(!lua_isnil(l, -1));

	lua_pushstring(l, "tradeWidgetFunctions");
	lua_pushvalue(l, 2);
	lua_settable(l, -3);

	lua_pop(l, 2);

	CommodityTradeWidget *w = new CommodityTradeWidget(dialog);
	w->onClickBuy.connect(sigc::mem_fun(dialog, &LuaChatForm::OnClickBuy));
	w->onClickSell.connect(sigc::mem_fun(dialog, &LuaChatForm::OnClickSell));
	Gui::Fixed *f = new Gui::Fixed(400.0, 200.0);
	f->Add(w, 0, 0);
	dialog->m_msgregion->PackEnd(f);

	dialog->m_commodityTradeWidget = w;

	dialog->onClose.connect(sigc::bind(sigc::ptr_fun(_cleanup_trade_functions), l, dialog->GetAdvert()->ref));

	return 0;
}

/*
 * Method: Close
 *
 * Closes the form and returns the player to the bulletin board
 *
 * > form:Close()
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_luachatform_close(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);
	dialog->Close();
	return 0;
}

/*
 * Method: Refresh
 *
 * Recalculates and redraws the entire chat form display
 *
 * > form:Refresh()
 *
 * This recalulates ship capacity and money stats and redraws everything on
 * the screen, including the form. Use this after your chat function changes
 * the player equipment, cargo or money.
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_luachatform_refresh(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);
	dialog->UpdateBaseDisplay();
	return 0;
}

/*
 * Method: GotoPolice
 *
 * Aborts the chat form and takes the player to the station police screen
 *
 * > form:GotoPolice()
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   experimental
 */
static int l_luachatform_goto_police(lua_State *l)
{
	Pi::spaceStationView->JumpToForm(new PoliceChatForm());
	return 0;
}

/*
 * Method: RemoveAdvertOnClose
 *
 * Flags that the advert attached the form should be removed from the bulletin
 * board when the form closes
 *
 * > form:RemoveAdvertOnClose()
 *
 * This does not actually close the form, it simply sets an internal flag that
 * will be acted upon at close.
 *
 * Availability:
 *
 *   alpha 10
 *
 * Status:
 *
 *   stable
 */
static int l_luachatform_remove_advert_on_close(lua_State *l)
{
	LuaChatForm *dialog = LuaObject<LuaChatForm>::GetFromLua(1);
	dialog->RemoveAdvertOnClose();
	return 0;
}

template <> const char *LuaObject<LuaChatForm>::s_type = "ChatForm";

template <> void LuaObject<LuaChatForm>::RegisterClass()
{
	static const luaL_reg l_methods[] = {
		{ "Clear",               l_luachatform_clear                         },
		{ "SetTitle",            l_luachatform_set_title                     },
        { "SetFace",             l_luachatform_set_face                      },
		{ "SetMessage",          l_luachatform_set_message                   },
		{ "AddOption",           l_luachatform_add_option                    },
		{ "AddGoodsTrader",      LuaChatForm::l_luachatform_add_goods_trader },
		{ "Close",               l_luachatform_close                         },
		{ "Refresh",             l_luachatform_refresh                       },
		{ "GotoPolice",          l_luachatform_goto_police                   },
		{ "RemoveAdvertOnClose", l_luachatform_remove_advert_on_close        },
		{ 0, 0 }
	};

	LuaObjectBase::CreateClass(s_type, NULL, l_methods, NULL, NULL);
}
