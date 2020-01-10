#include "createescrow.hpp"

#include "lib/common.h"
#include "models/registry.h"
namespace createescrow
{
/*
    * Called when a new user account is created
    * Transfers the dapp tokens to the new account created
    */
void create_escrow::airdrop(string dapp, name account)
{
    registry::Registry dapps(_self, _self.value);
    auto iterator = dapps.find(common::toUUID(dapp));
    if (iterator != dapps.end())
        dapps.modify(iterator, same_payer, [&](auto &row) {
            // check if the dapp has opted for airdrop
            if (row.airdrop->contract != name(""))
            {
                if (row.airdrop->balance.amount > 0)
                {
                    asset tokens = row.airdrop->amount;
                    auto memo = "airdrop " + tokens.to_string() + " to " + account.to_string();
                    action(
                        permission_level{_self, "active"_n},
                        row.airdrop->contract,
                        name("transfer"),
                        make_tuple(_self, account, tokens, memo))
                        .send();
                    row.airdrop->balance -= tokens;
                }
                else
                {
                    check(false, ("Not enough " + row.airdrop->balance.symbol.code().to_string() + " with createescrow to airdrop for"+ row.dapp +"app").c_str());
                }
            }
        });
}
} // namespace createescrow