#include "MarketAgent.h"
#include "Serializer.h"
#include "Player.h"
#include "Pi.h"

void MarketAgent::Load(Serializer::Reader &rd)
{
	m_money = rd.Int64();
}

void MarketAgent::Save(Serializer::Writer &wr) const
{
	wr.Int64(m_money);
}

bool MarketAgent::SellTo(MarketAgent *other, Equip::Type t, bool verbose)
{
	if (other->CanBuy(t, verbose) && CanSell(t, verbose) && other->Pay(this, GetPrice(t), verbose)) {
		Sold(t);
		other->Bought(t);
		return true;
	} else return false;
}
	
bool MarketAgent::BuyFrom(MarketAgent *other, Equip::Type t, bool verbose)
{
	if (other->CanSell(t, verbose) && CanBuy(t, verbose) && Pay(other, GetPrice(t), verbose)) {
		other->Sold(t);
		Bought(t);
		return true;
	} else return false;
}
	
bool MarketAgent::Pay(MarketAgent *b, Sint64 amount, bool verbose) {
	if (m_money < amount) {
		if (verbose) {
			if (this == Pi::player) {
				Pi::Message("", "You do not have enough credits.");
			} else {
				Pi::Message("", "Trader does not have enough credits.");
			}
		}
		return false;
	}
	b->m_money += amount;
	m_money -= amount;
	return true;
}
