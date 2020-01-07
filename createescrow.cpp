#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/action.hpp>

#include "createescrow.hpp"

#include "lib/common.h"

#include "models/accounts.h"
#include "models/balances.h"
#include "models/registry.h"
#include "models/bandwidth.h"

#include "constants.cpp"
#include "createaccounts.cpp"
#include "stakes.cpp"

namespace createescrow {
    using namespace eosio;
    using namespace common;
    using namespace accounts;
    using namespace balance;
    using namespace bandwidth;
    using namespace registry;
    using namespace std;

    create_escrow::create_escrow(name s, name code, datastream<const char *> ds) : contract(s, code, ds),
                                                                          dapps(_self, _self.value),
                                                                          balances(_self, _self.value),
                                                                          token(_self, _self.value){}
    
    
    /***
     * Called to specify the following details:
     * symbol:              the core token of the chain or the token used to pay for new user accounts of the chain  
     * newAccountContract:  the contract to call for new account action 
     * minimumram:           minimum bytes of RAM to put in a new account created on the chain 
    */
    void create_escrow::init(const symbol &symbol, name newaccountcontract, name newaccountaction, uint64_t minimumram)
    {
        require_auth(_self);

        auto iterator = token.find(symbol.raw());

        if (iterator == token.end())
            token.emplace(_self, [&](auto &row) {
                row.S_SYS = symbol;
                row.newaccountcontract = newaccountcontract;
                row.newaccountaction = newaccountaction;
                row.min_ram = minimumram;
            });
        else
            token.modify(iterator, same_payer, [&](auto &row) {
                row.S_SYS = symbol;
                row.newaccountcontract = newaccountcontract;
                row.newaccountaction = newaccountaction;
                row.min_ram = minimumram;
            });
    }

    /***
     * Called to define an account name as the owner of a dapp along with the following details:
     * owner:           account name to be registered as the owner of the dapp 
     * dapp:            the string/account name representing the dapp
     * ram_bytes:       bytes of ram to put in the new user account created for the dapp
     * net:             EOS amount to be staked for net
     * cpu:             EOS amount to be staked for cpu
     * airdropcontract: airdropdata struct/json or null
     * Only the owner account/whitelisted account will be able to create new user account for the dapp
     */

    void create_escrow::define(name & owner, string dapp, uint64_t ram_bytes, asset net, asset cpu, uint64_t pricekey, airdropdata & airdrop, bool use_rex, rexdata &rex)
    {
        require_auth(dapp != "free" ? owner : _self);

        auto iterator = dapps.find(toUUID(dapp));

        eosio_assert(iterator == dapps.end() || (iterator != dapps.end() && iterator->owner == owner),
                     ("the dapp " + dapp + " is already registered by another account").c_str());

        uint64_t min_ram = create_escrow::getMinimumRAM();

        eosio_assert(ram_bytes >= min_ram, ("ram for new accounts must be equal to or greater than " + to_string(min_ram) + " bytes.").c_str());

        // Creating a new dapp reference
        if (iterator == dapps.end())
            dapps.emplace(_self, [&](auto &row) {
                row.owner = owner;
                row.dapp = dapp;
                row.pricekey = pricekey;
                row.ram_bytes = ram_bytes;
                row.net = net;
                row.cpu = cpu;
                row.airdrop = airdrop;
                row.use_rex = use_rex;
                row.rex = rex;
            });

        // Updating an existing dapp reference's configurations
        else
            dapps.modify(iterator, same_payer, [&](auto &row) {
                row.ram_bytes = ram_bytes;
                row.pricekey = pricekey;
                row.net = net;
                row.cpu = cpu;
                row.airdrop = airdrop;
                row.use_rex = use_rex;
                row.rex = rex;
            });
    }
    
    /***
     * Lets the owner account of the dapp to whitelist other accounts. 
     */
    void create_escrow::whitelist(name owner, name account, string dapp)
    {
        require_auth(owner);

        auto iterator = dapps.find(toUUID(dapp));

        if (iterator != dapps.end() && owner == iterator->owner)
            dapps.modify(iterator, same_payer, [&](auto &row) {
                if (std::find(row.custodians.begin(), row.custodians.end(), account) == row.custodians.end())
                {
                    row.custodians.push_back(account);
                }
            });

        else
            eosio_assert(false, ("the dapp " + dapp + " is not owned by account " + owner.to_string()).c_str());
    }

    /***
     * Transfers the remaining balance of a contributor from createbridge back to the contributor
     * reclaimer: account trying to reclaim the balance
     * dapp:      the dapp name for which the account is trying to reclaim the balance
     * sym:       symbol of the tokens to be reclaimed. It can have value based on the following scenarios:
     *            - reclaim the "native" token balance used to create accounts. For ex - EOS/SYS
     *            - reclaim the remaining airdrop token balance used to airdrop dapp tokens to new user accounts. For ex- IQ/LUM
     */
    void create_escrow::reclaim(name reclaimer, string dapp, string sym)
    {
        require_auth(reclaimer);

        asset reclaimer_balance;
        bool nocontributor;

        // check if the user is trying to reclaim the system tokens
        if (sym == create_escrow::getCoreSymbol().code().to_string())
        {

            auto iterator = balances.find(common::toUUID(dapp));

            if (iterator != balances.end())
            {

                balances.modify(iterator, same_payer, [&](auto &row) {
                    auto pred = [reclaimer](const contributors &item) {
                        return item.contributor == reclaimer;
                    };
                    auto reclaimer_record = remove_if(std::begin(row.contributors), std::end(row.contributors), pred);
                    if (reclaimer_record != row.contributors.end())
                    {
                        reclaimer_balance = reclaimer_record->balance;

                        // only erase the contributor row if the cpu and net balances are also 0
                        if (reclaimer_record->net_balance == asset(0'0000, getCoreSymbol()) && reclaimer_record->cpu_balance == asset(0'0000, getCoreSymbol()))
                        {
                            row.contributors.erase(reclaimer_record, row.contributors.end());
                        }
                        else
                        {
                            reclaimer_record->balance -= reclaimer_balance;
                        }

                        row.balance -= reclaimer_balance;
                    }
                    else
                    {
                        eosio_assert(false, ("no remaining contribution for " + dapp + " by " + reclaimer.to_string()).c_str());
                    }

                    nocontributor = row.contributors.empty();
                });

                // delete the entire balance object if no contributors are there for the dapp
                if (nocontributor && iterator->balance == asset(0'0000, getCoreSymbol()))
                {
                    balances.erase(iterator);
                }

                // transfer the remaining balance for the contributor from the createbridge account to contributor's account
                auto memo = "reimburse the remaining balance to " + reclaimer.to_string();
                action(
                    permission_level{_self, "active"_n},
                    name("eosio.token"),
                    name("transfer"),
                    make_tuple(_self, reclaimer, reclaimer_balance, memo))
                    .send();
            }
            else
            {
                eosio_assert(false, ("no funds given by " + reclaimer.to_string() + " for " + dapp).c_str());
            }
        }
        // user is trying to reclaim custom dapp tokens
        else
        {
            auto iterator = dapps.find(toUUID(dapp));
            if (iterator != dapps.end())
                dapps.modify(iterator, same_payer, [&](auto &row) {
                    if (row.airdrop->contract != name("") && row.airdrop->tokens.symbol.code().to_string() == sym && row.owner == name(reclaimer))
                    {
                        auto memo = "reimburse the remaining airdrop balance for " + dapp + " to " + reclaimer.to_string();
                        if (row.airdrop->tokens != asset(0'0000, row.airdrop->tokens.symbol))
                        {
                            action(
                                permission_level{_self, "active"_n},
                                row.airdrop->contract,
                                name("transfer"),
                                make_tuple(_self, reclaimer, row.airdrop->tokens, memo))
                                .send();
                            row.airdrop->tokens -= row.airdrop->tokens;
                        }
                        else
                        {
                            eosio_assert(false, ("No remaining airdrop balance for " + dapp + ".").c_str());
                        }
                    }
                    else
                    {
                        eosio_assert(false, ("the remaining airdrop balance for " + dapp + " can only be claimed by its owner/whitelisted account.").c_str());
                    }
                });
        }
    }

    // to check if createbridge is deployed and functioning
    void create_escrow::ping(name & from){
        print('ping');
    }
}