#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/action.hpp>

#include "lib/common.h"

#include "models/accounts.h"
#include "models/balances.h"
#include "models/registry.h"
#include "models/bandwidth.h"

#include "createaccounts.cpp"

using namespace eosio;
using namespace common;
using namespace accounts;
using namespace balances;
using namespace bandwidth;
using namespace registry;
using namespace std;

CONTRACT createbridge : contract, public createaccounts
{
private:
    Registry dapps;
    Balances balances;
    Token token;

public:
    using contract::contract;
    createbridge(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds),
                                                                          dapps(_self, _self.value),
                                                                          balances(_self, _self.value),
                                                                          token(_self, _self.value) {}

    name createbridgename = common::createbridgeName;

    template <typename T>
    void cleanTable()
    {
        T db(_self, _self.value);
        while (db.begin() != db.end())
        {
            auto itr = --db.end();
            db.erase(itr);
        }
    }

    ACTION clean()
    {
        require_auth(_self);
        cleanTable<Balances>();
    }

    ACTION cleanreg()
    {
        require_auth(_self);
        cleanTable<Registry>();
    }

    ACTION cleantoken()
    {
        require_auth(_self);
        cleanTable<Token>();
    }

    /**********************************************/
    /***                                        ***/
    /***                Actions                 ***/
    /***                                        ***/
    /**********************************************/

    /***
     * Called to specify the following details:
     * symbol:              the core token of the chain or the token used to pay for new user accounts of the chain  
     * newAccountContract:  the contract to call for new account action 
     * minimumram:           minimum bytes of RAM to put in a new account created on the chain 
    */

    ACTION init(const symbol &symbol, name newaccountcontract, name newaccountaction, uint64_t minimumram)
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

    ACTION define(name & owner, string dapp, uint64_t ram_bytes, asset net, asset cpu, uint64_t pricekey, airdropdata & airdrop, bool use_rex, rexdata &rex)
    {
        require_auth(dapp != "free" ? owner : _self);

        auto iterator = dapps.find(toUUID(dapp));

        eosio_assert(iterator == dapps.end() || (iterator != dapps.end() && iterator->owner == owner),
                     ("the dapp " + dapp + " is already registered by another account").c_str());

        uint64_t min_ram = getMinimumRAM();

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
    ACTION whitelist(name owner, name account, string dapp)
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
     * Creates a new user account. 
     * It also airdrops custom dapp tokens to the new user account if a dapp owner has opted for airdrops
     * memo:                name of the account paying for the balance left after getting the donation from the dapp contributors 
     * account:             name of the account to be created
     * ownerkey,activekey:  key pair for the new account  
     * origin:              the string representing the dapp to create the new user account for. For ex- everipedia.org, lumeos
     * For new user accounts, it follows the following steps:
     * 1. Choose a contributor, if any, for the dapp to fund the cost for new account creation
     * 2. Check if the contributor is funding 100 %. If not, check if the "memo" account has enough to fund the remaining cost of account creation
    */
    ACTION create(string & memo, name & account, public_key & ownerkey, public_key & activekey, string & origin, name referral)
    {
        auto iterator = dapps.find(toUUID(origin));

        // Only owner/whitelisted account for the dapp can create accounts
        if (iterator != dapps.end())
        {
            if (name(memo) == iterator->owner)
                require_auth(iterator->owner);
            else if (checkIfWhitelisted(name(memo), origin))
                require_auth(name(memo));
            else if (origin == "free")
                print("using globally available free funds to create account");
            else
                eosio_assert(false, ("only owner or whitelisted accounts can create new user accounts for " + origin).c_str());
        }
        else
        {
            eosio_assert(false, ("no owner account found for " + origin).c_str());
        }

        authority owner{.threshold = 1, .keys = {key_weight{ownerkey, 1}}, .accounts = {}, .waits = {}};
        authority active{.threshold = 1, .keys = {key_weight{activekey, 1}}, .accounts = {}, .waits = {}};
        createJointAccount(memo, account, origin, owner, active, referral);
    }

    /***
     * Transfers the remaining balance of a contributor from createbridge back to the contributor
     * reclaimer: account trying to reclaim the balance
     * dapp:      the dapp name for which the account is trying to reclaim the balance
     * sym:       symbol of the tokens to be reclaimed. It can have value based on the following scenarios:
     *            - reclaim the "native" token balance used to create accounts. For ex - EOS/SYS
     *            - reclaim the remaining airdrop token balance used to airdrop dapp tokens to new user accounts. For ex- IQ/LUM
     */
    ACTION reclaim(name reclaimer, string dapp, string sym)
    {
        require_auth(reclaimer);

        asset reclaimer_balance;
        bool nocontributor;

        // check if the user is trying to reclaim the system tokens
        if (sym == getCoreSymbol().code().to_string())
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
                        row.contributors.erase(reclaimer_record, row.contributors.end());
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

    /**********************************************/
    /***                                        ***/
    /***               STAKE                    ***/
    /***                                        ***/
    /**********************************************/

    ACTION unstake(name & from, name & to, string & origin)
    {
        checkIfOwnerOrWhitelisted(from, origin);

        stakes::unstakeCpuOrNet(from, to, origin);
    }

    ACTION unstakenet(name & from, name & to, string & origin)
    {
        checkIfOwnerOrWhitelisted(from, origin);

        // only unstake for net
        stakes::unstakeCpuOrNet(from, to, origin, true, false);
    }

    ACTION unstakecpu(name & from, name & to, string & origin)
    {
        checkIfOwnerOrWhitelisted(from, origin);

        // only unstake for cpu
        stakes::unstakeCpuOrNet(from, to, origin, false, true);
    }

    /**********************************************/
    /***                                        ***/
    /***               REX                    ***/
    /***                                        ***/
    /**********************************************/

    // finds the existing net loan from createbridge to user account and funds it
    ACTION fundnetloan(name & from, name & to, asset quantity, string & origin)
    {
        checkIfOwnerOrWhitelisted(from, origin);

        fundloan(to, quantity, origin, "net");
        contributions::subCpuOrNetBalance(from.to_string(), origin, quantity, "net");
    }

    // finds the existing cpu loan from createbridge to user account and funds it
    ACTION fundcpuloan(name & from, name & to, asset quantity, string & origin)
    {
        checkIfOwnerOrWhitelisted(from, origin);

        fundloan(to, quantity, origin, "cpu");
        contributions::subCpuOrNetBalance(from.to_string(), origin, quantity, "cpu");
    }

    // creates a new loan from createbridge to the user acount (to)
    ACTION rentnet(name & from, name & to, string & origin)
    {
        checkIfOwnerOrWhitelisted(from, origin);

        auto iterator = dapps.find(common::toUUID(origin));

        rex::rentnet(origin, to);

        asset quantity = iterator->rex->net_loan_payment + iterator->rex->net_loan_fund;
        contributions::subCpuOrNetBalance(from.to_string(), origin, quantity, "net");
    }

    // creates a new loan from createbridge to the user acount (to)
    ACTION rentcpu(name & from, name & to, string & origin)
    {
        checkIfOwnerOrWhitelisted(from, origin);

        auto iterator = dapps.find(common::toUUID(origin));

        rex::rentcpu(origin, to);

        asset quantity = iterator->rex->cpu_loan_payment + iterator->rex->cpu_loan_fund;
        contributions::subCpuOrNetBalance(from.to_string(), origin, quantity, "cpu");
    }

    // topup existing loan balance (cpu & net) for a user up to the given quantity
    // If no existing loan, then create a new loan
    ACTION topuploans(name & from, name & to, asset & cpuquantity, asset & netquantity, string & origin)
    {
        checkIfOwnerOrWhitelisted(from, origin);

        asset required_net_bal, required_cpu_bal;
        tie(required_net_bal, required_cpu_bal) = rex::topup(to, cpuquantity, netquantity, origin);

        contributions::subCpuOrNetBalance(from.to_string(), origin, required_net_bal, "net");
        contributions::subCpuOrNetBalance(from.to_string(), origin, required_cpu_bal, "cpu");
    }

    /**********************************************/
    /***                                        ***/
    /***               Transfers                ***/
    /***                                        ***/
    /**********************************************/

    void transfer(const name &from, const name &to, const asset &quantity, string &memo)
    {
        if (to != _self)
            return;
        if (from == name("eosio.stake"))
        {
            return;
            // addTotalUnstaked(quantity);
        };

        if (quantity.symbol != getCoreSymbol())
            return;
        if (memo.length() > 64)
            return;
        addBalance(from, quantity, memo);
    }
};

extern "C"
{
    void apply(uint64_t receiver, uint64_t code, uint64_t action)
    {
        auto self = receiver;

        if (code == self)
            switch (action)
            {
                EOSIO_DISPATCH_HELPER(createbridge, (init)(clean)(cleanreg)(cleantoken)(create)(define)(whitelist)(reclaim)(unstake)(unstakenet)(unstakecpu)(fundnetloan)(fundcpuloan)(rentnet)(rentcpu)(topuploans))
            }

        else
        {
            if (code == name("eosio.token").value && action == name("transfer").value)
            {
                execute_action(name(receiver), name(code), &createbridge::transfer);
            }
        }
    }
};