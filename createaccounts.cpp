#include <eosiolib/eosio.hpp>

#include "createescrow.hpp"

#include "lib/common.h"

#include "models/accounts.h"
#include "models/balances.h"
#include "models/registry.h"
#include "models/bandwidth.h"

#include "airdrops.cpp"
#include "contributions.cpp"
#include "constants.cpp"
#include "rex.cpp"

namespace createescrow
{
using namespace eosio;
using namespace std;

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
void create_escrow::create(string &memo, name &account, public_key &ownerkey, public_key &activekey, string &origin, name referral)
{
    auto iterator = dapps.find(toUUID(origin));

    // Only owner/whitelisted account for the dapp can create accounts
    if (iterator != dapps.end())
    {
        if (name(memo) == iterator->owner)
            require_auth(iterator->owner);
        else if (create_escrow::checkIfWhitelisted(name(memo), origin))
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
    create_escrow::createJointAccount(memo, account, origin, owner, active, referral);
}

/***
     * Checks if an account is whitelisted for a dapp by the owner of the dapp
     * @return
     */
void create_escrow::createJointAccount(string &memo, name &account, string &origin, accounts::authority &ownerAuth, accounts::authority &activeAuth, name referral)
{
    // memo is the account that pays the remaining balance i.e
    // balance needed for new account creation - (balance contributed by the contributors)
    vector<balance::chosen_contributors> contributors;
    name freeContributor;

    asset balance;
    asset requiredBalance;

    bool useOwnerCpuBalance = false;
    bool useOwnerNetBalance = false;

    symbol coreSymbol = create_escrow::getCoreSymbol();
    asset ramFromDapp = asset(0'0000, coreSymbol);

    balance::Balances balances(_self, _self.value);
    registry::Registry dapps(_self, _self.value);

    // gets the ram, net and cpu requirements for the new user accounts from the dapp registry
    auto iterator = dapps.find(common::toUUID(origin));
    string owner = iterator->owner.to_string();
    uint64_t ram_bytes = iterator->ram_bytes;

    bool isfixed = false;
    if (ram_bytes == 0)
    {
        isfixed = true;
    }

    // cost of required ram
    asset ram = create_escrow::getRamCost(ram_bytes, iterator->pricekey);

    asset net;
    asset net_balance;
    asset cpu;
    asset cpu_balance;

    // isfixed - if a fixed tier pricing is offered for accounts. For ex - 5 SYS for 4096 bytes RAM, 1 SYS CPU and 1 SYS net
    if (!isfixed)
    {
        net = iterator->use_rex ? iterator->rex->net_loan_payment + iterator->rex->net_loan_fund : iterator->net;
        cpu = iterator->use_rex ? iterator->rex->cpu_loan_payment + iterator->rex->cpu_loan_fund : iterator->cpu;

        // if using rex, then the net balance to be deducted will be the same as net_loan_payment + net_loan_fund. Similar for cpu
        net_balance = create_escrow::findContribution(origin, name(memo), "net");
        if (net_balance == asset(0'0000, coreSymbol))
        {
            net_balance = create_escrow::findContribution(origin, iterator->owner, "net");
            useOwnerNetBalance = true;
        }

        cpu_balance = create_escrow::findContribution(origin, name(memo), "cpu");
        if (cpu_balance == asset(0'0000, coreSymbol))
        {
            cpu_balance = create_escrow::findContribution(origin, iterator->owner, "cpu");
            useOwnerCpuBalance = true;
        }

        if (cpu > cpu_balance || net > net_balance)
        {
            eosio_assert(false, ("Not enough cpu or net balance in " + memo + "for " + origin + " to pay for account's bandwidth.").c_str());
        }

        if (useOwnerNetBalance)
        {
            create_escrow::subCpuOrNetBalance(owner, origin, net, "net");
        }
        else
        {
            create_escrow::subCpuOrNetBalance(memo, origin, net, "net");
        }

        if (useOwnerCpuBalance)
        {
            create_escrow::subCpuOrNetBalance(owner, origin, cpu, "cpu");
        }
        else
        {
            create_escrow::subCpuOrNetBalance(memo, origin, cpu, "cpu");
        }
    }
    else
    {
        net = create_escrow::getFixedNet(iterator->pricekey);
        cpu = create_escrow::getFixedCpu(iterator->pricekey);
    }

    asset ramFromPayer = ram;

    if (memo != origin && create_escrow::hasBalance(origin, ram))
    {
        uint64_t originId = common::toUUID(origin);
        auto dapp = balances.find(originId);

        if (dapp != balances.end())
        {
            uint64_t seed = account.value;
            uint64_t value = name(memo).value;
            contributors = create_escrow::getContributors(origin, memo, seed, value, ram);

            for (std::vector<balance::chosen_contributors>::iterator itr = contributors.begin(); itr != contributors.end(); ++itr)
            {
                ramFromDapp += itr->rampay;
            }

            ramFromPayer -= ramFromDapp;
        }
    }

    // find the balance of the "memo" account for the origin and check if it has balance >= total balance for RAM, CPU and net - (balance payed by the contributors)
    if (ramFromPayer > asset(0'0000, coreSymbol))
    {
        asset balance = create_escrow::findContribution(origin, name(memo), "ram");
        requiredBalance = ramFromPayer;

        // if the "memo" account doesn't have enough fund, check globally available "free" pool
        if (balance < requiredBalance)
        {
            eosio_assert(false, ("Not enough balance in " + memo + " or donated by the contributors for " + origin + " to pay for account creation.").c_str());
        }
    }

    create_escrow::createAccount(origin, account, ownerAuth, activeAuth, ram, net, cpu, iterator->pricekey, iterator->use_rex, isfixed, referral);

    // subtract the used balance
    if (ramFromPayer.amount > 0)
    {
        create_escrow::subBalance(memo, origin, requiredBalance);
    }

    if (ramFromDapp.amount > 0)
    {
        for (std::vector<balance::chosen_contributors>::iterator itr = contributors.begin(); itr != contributors.end(); ++itr)
        {
            // check if the memo account and the dapp contributor is the same. If yes, only increament accounts created by 1
            if (itr->contributor == name{memo} && ramFromPayer.amount > 0)
            {
                create_escrow::subBalance(itr->contributor.to_string(), origin, itr->rampay, true);
            }
            else
            {
                create_escrow::subBalance(itr->contributor.to_string(), origin, itr->rampay);
            }
        }
    }

    // airdrop dapp tokens if requested
    create_escrow::airdrop(origin, account);
}

/***
     * Calls the chain to create a new account
     */
void create_escrow::createAccount(string dapp, name &account, accounts::authority &ownerauth, accounts::authority &activeauth, asset &ram, asset &net, asset &cpu, uint64_t pricekey, bool use_rex, bool isfixed, name referral)
{
    accounts::newaccount new_account = accounts::newaccount{
        .creator = _self,
        .name = account,
        .owner = ownerauth,
        .active = activeauth};

    name newAccountContract = create_escrow::getNewAccountContract();
    name newAccountAction = create_escrow::getNewAccountAction();
    if (isfixed)
    { // check if the account creation is fixed
        action(
            permission_level{_self, "active"_n},
            newAccountContract,
            newAccountAction,
            make_tuple(_self, account, ownerauth.keys[0].key, activeauth.keys[0].key, pricekey, referral))
            .send();
    }
    else
    {
        action(
            permission_level{_self, "active"_n},
            newAccountContract,
            name("newaccount"),
            new_account)
            .send();

        action(
            permission_level{_self, "active"_n},
            newAccountContract,
            name("buyram"),
            make_tuple(_self, account, ram))
            .send();

        if (use_rex == true)
        {
            create_escrow::rentrexnet(dapp, account);
            create_escrow::rentrexcpu(dapp, account);
        }
        else
        {
            if (net + cpu > asset(0'0000, create_escrow::getCoreSymbol()))
            {
                action(
                    permission_level{_self, "active"_n},
                    newAccountContract,
                    name("delegatebw"),
                    make_tuple(_self, account, net, cpu, false))
                    .send();
            }
        }
    }
};

} // namespace createescrow