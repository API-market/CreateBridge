#include <eosiolib/eosio.hpp>

#include "lib/common.h"

#include "models/accounts.h"
#include "models/balances.h"
#include "models/registry.h"
#include "models/bandwidth.h"

#include "airdrops.cpp"
#include "contributions.cpp"
#include "rex.cpp"
#include "stakes.cpp"

using namespace eosio;
using namespace std;

class createaccounts : public airdrops, public contributions, public rex, public stakes
{

public:
    name createescrow = common::createescrowContractName;
    symbol coreSymbol = common::getCoreSymbol();

    /***
     * Checks if an account is whitelisted for a dapp by the owner of the dapp
     * @return
     */
    void createJointAccount(string &memo, name &account, string &origin, accounts::authority &ownerAuth, accounts::authority &activeAuth, name referral)
    {
        // memo is the account that pays the remaining balance i.e
        // balance needed for new account creation - (balance contributed by the contributors)
        vector<balances::chosen_contributors> contributors;
        name freeContributor;

        asset balance;
        asset requiredBalance;

        bool useOwnerCpuBalance = false;
        bool useOwnerNetBalance = false;

        asset ramFromDapp = asset(0'0000, coreSymbol);

        balances::Balances balances(createescrow, createescrow.value);
        registry::Registry dapps(createescrow, createescrow.value);

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
        asset ram = common::getRamCost(ram_bytes, iterator->pricekey);

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
            net_balance = contributions::findContribution(origin, name(memo), "net");
            if (net_balance == asset(0'0000, coreSymbol))
            {
                net_balance = contributions::findContribution(origin, iterator->owner, "net");
                useOwnerNetBalance = true;
            }

            cpu_balance = contributions::findContribution(origin, name(memo), "cpu");
            if (cpu_balance == asset(0'0000, coreSymbol))
            {
                cpu_balance = contributions::findContribution(origin, iterator->owner, "cpu");
                useOwnerCpuBalance = true;
            }

            if (cpu > cpu_balance || net > net_balance)
            {
                eosio_assert(false, ("Not enough cpu or net balance in " + memo + "for " + origin + " to pay for account's bandwidth.").c_str());
            }

            if (useOwnerNetBalance)
            {
                subCpuOrNetBalance(owner, origin, net, "net");
            }
            else
            {
                subCpuOrNetBalance(memo, origin, net, "net");
            }

            if (useOwnerCpuBalance)
            {
                subCpuOrNetBalance(owner, origin, cpu, "cpu");
            }
            else
            {
                subCpuOrNetBalance(memo, origin, cpu, "cpu");
            }
        }
        else
        {
            net = common::getFixedNet(iterator->pricekey);
            cpu = common::getFixedCpu(iterator->pricekey);
        }

        asset ramFromPayer = ram;

        if (memo != origin && contributions::hasBalance(origin, ram))
        {
            uint64_t originId = common::toUUID(origin);
            auto dapp = balances.find(originId);

            if (dapp != balances.end())
            {
                uint64_t seed = account.value;
                uint64_t value = name(memo).value;
                contributors = getContributors(origin, memo, seed, value, ram);

                for (std::vector<balances::chosen_contributors>::iterator itr = contributors.begin(); itr != contributors.end(); ++itr)
                {
                    ramFromDapp += itr->rampay;
                }

                ramFromPayer -= ramFromDapp;
            }
        }

        // find the balance of the "memo" account for the origin and check if it has balance >= total balance for RAM, CPU and net - (balance payed by the contributors)
        if (ramFromPayer > asset(0'0000, coreSymbol))
        {
            asset balance = contributions::findContribution(origin, name(memo), "ram");
            requiredBalance = ramFromPayer;

            // if the "memo" account doesn't have enough fund, check globally available "free" pool
            if (balance < requiredBalance)
            {
                eosio_assert(false, ("Not enough balance in " + memo + " or donated by the contributors for " + origin + " to pay for account creation.").c_str());
            }
        }

        createAccount(origin, account, ownerAuth, activeAuth, ram, net, cpu, iterator->pricekey, iterator->use_rex, isfixed, referral);

        // subtract the used balance
        if (ramFromPayer.amount > 0)
        {
            subBalance(memo, origin, requiredBalance);
        }

        if (ramFromDapp.amount > 0)
        {
            for (std::vector<balances::chosen_contributors>::iterator itr = contributors.begin(); itr != contributors.end(); ++itr)
            {
                // check if the memo account and the dapp contributor is the same. If yes, only increament accounts created by 1
                if (itr->contributor == name{memo} && ramFromPayer.amount > 0)
                {
                    subBalance(itr->contributor.to_string(), origin, itr->rampay, true);
                }
                else
                {
                    subBalance(itr->contributor.to_string(), origin, itr->rampay);
                }
            }
        }

        // airdrop dapp tokens if requested
        airdrop(origin, account);
    }

    /***
     * Checks if an account is whitelisted for a dapp by the owner of the dapp
     * @return
     */
    bool checkIfWhitelisted(name account, string dapp)
    {
        registry::Registry dapps(createescrow, createescrow.value);
        auto iterator = dapps.find(common::toUUID(dapp));
        auto position_in_whitelist = std::find(iterator->custodians.begin(), iterator->custodians.end(), account);
        if (position_in_whitelist != iterator->custodians.end())
        {
            return true;
        }
        return false;
    }

    void checkIfOwnerOrWhitelisted(name account, string origin)
    {
        registry::Registry dapps(createescrow, createescrow.value);
        auto iterator = dapps.find(common::toUUID(origin));

        if (iterator != dapps.end())
        {
            if (account == iterator->owner)
                require_auth(account);
            else if (checkIfWhitelisted(account, origin))
                require_auth(account);
            else if (origin == "free")
                print("using globally available free funds to create account");
            else
                eosio_assert(false, ("only owner or whitelisted accounts can call this action for " + origin).c_str());
        }
        else
        {
            eosio_assert(false, ("no owner account found for " + origin).c_str());
        }
    }

    /***
     * Calls the chain to create a new account
     */
    void createAccount(string dapp, name &account, accounts::authority &ownerauth, accounts::authority &activeauth, asset &ram, asset &net, asset &cpu, uint64_t pricekey, bool use_rex, bool isfixed, name referral)
    {
        accounts::newaccount new_account = accounts::newaccount{
            .creator = createescrow,
            .name = account,
            .owner = ownerauth,
            .active = activeauth};

        name newAccountContract = common::getNewAccountContract();
        name newAccountAction = common::getNewAccountAction();
        if (isfixed)
        { // check if the account creation is fixed
            action(
                permission_level{createescrow, "active"_n},
                newAccountContract,
                newAccountAction,
                make_tuple(createescrow, account, ownerauth.keys[0].key, activeauth.keys[0].key, pricekey, referral))
                .send();
        }
        else
        {
            action(
                permission_level{createescrow, "active"_n},
                newAccountContract,
                name("newaccount"),
                new_account)
                .send();

            action(
                permission_level{createescrow, "active"_n},
                newAccountContract,
                name("buyram"),
                make_tuple(createescrow, account, ram))
                .send();

            if (use_rex == true)
            {
                rentnet(dapp, account);
                rentcpu(dapp, account);
            }
            else
            {
                if(net + cpu > asset(0'0000,coreSymbol)){
                    action(
                        permission_level{createescrow, "active"_n},
                        newAccountContract,
                        name("delegatebw"),
                        make_tuple(createescrow, account, net, cpu, false))
                        .send();
                }
            }
        }
    };
};