#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/action.hpp>

#include "lib/common.h"

#include "models/accounts.h"
#include "models/balances.h"
#include "models/registry.h"
#include "models/bandwidth.h"

namespace createescrow {

  using namespace eosio;
  using namespace common;
  using namespace accounts;
  using namespace balance;
  using namespace bandwidth;
  using namespace registry;
  using namespace std;

  class [[eosio::contract("createescrow")]] create_escrow : public eosio::contract{

  private:
    Registry dapps;
    Balances balances;
    Token token;


  public:
    using contract::contract;

    create_escrow( name s, name code, datastream<const char*> ds );

    [[eosio::action]]
    void clean();

    [[eosio::action]]
    void cleanreg();

    [[eosio::action]]
    void cleantoken();

    [[eosio::action]]
    void init(const symbol &symbol, name newaccountcontract, name newaccountaction, uint64_t minimumram); 
   
    [[eosio::action]]
    void define(name & owner, string dapp, uint64_t ram_bytes, asset net, asset cpu, uint64_t pricekey, registry::airdropdata & airdrop, bool use_rex, registry::rexdata &rex);

    [[eosio::action]]
    void whitelist(name owner, name account, string dapp);

    [[eosio::action]]
    void create(string & memo, name & account, public_key & ownerkey, public_key & activekey, string & origin, name referral);

    [[eosio::action]]
    void reclaim(name reclaimer, string dapp, string sym);

    [[eosio::action]]
    void refundstakes(name & from, string & origin);

    [[eosio::action]]
    void stake(name & from, name & to, string & origin, asset & net, asset & cpu);

    [[eosio::action]]
    void unstake(name & from, name & to, string & origin);

    [[eosio::action]]
    void unstakenet(name & from, name & to, string & origin);

    [[eosio::action]]
    void unstakecpu(name & from, name & to, string & origin);

    [[eosio::action]]
    void fundnetloan(name & from, name & to, asset quantity, string & origin);

    [[eosio::action]]
    void fundcpuloan(name & from, name & to, asset quantity, string & origin);

    [[eosio::action]]
    void rentnet(name & from, name & to, string & origin);

    [[eosio::action]]
    void rentcpu(name & from, name & to, string & origin);

    [[eosio::action]]
    void topuploans(name & from, name & to, asset & cpuquantity, asset & netquantity, string & origin);

    [[eosio::action]]
    void ping(name & from);

    void transfer(const name &from, const name &to, const asset &quantity, string &memo);

/***
* Checks if an account is whitelisted for a dapp by the owner of the dapp
* @return
*/
bool checkIfWhitelisted(name account, string dapp)
{
    registry::Registry dapps(_self, _self.value);
    auto iterator = dapps.find(common::toUUID(dapp));
    auto position_in_whitelist = std::find(iterator->custodians.begin(), iterator->custodians.end(), account);
    if (position_in_whitelist != iterator->custodians.end())
    {
        return true;
    }
    return false;
}

bool checkIfOwner(name account, string dapp)
{
    registry::Registry dapps(_self, _self.value);
    auto iterator = dapps.find(common::toUUID(dapp));

    if (iterator != dapps.end())
    {
        if (account == iterator->owner)
        {
            return true;
        }
    }
    return false;
}
    
void checkIfOwnerOrWhitelisted(name account, string origin)
{
    registry::Registry dapps(_self, _self.value);
    auto iterator = dapps.find(common::toUUID(origin));

    if (iterator != dapps.end())
    {
        if (account == iterator->owner)
            require_auth(account);
        else if (create_escrow::checkIfWhitelisted(account, origin))
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

    using init_action = eosio::action_wrapper<"init"_n, &create_escrow::init>;
    using define_action = eosio::action_wrapper<"define"_n, &create_escrow::define>;
    using whitelist_action = eosio::action_wrapper<"whitelist"_n, &create_escrow::whitelist>;
    using create_action = eosio::action_wrapper<"create"_n, &create_escrow::create>;


  private:

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

    void airdrop(string dapp, name account);
    asset balanceFor(string &memo);
    bool hasBalance(string memo, const asset &quantity);

    void addBalance(const name &from, const asset &quantity, string &memo);
    void subBalance(string memo, string &origin, const asset &quantity, bool memoIsDapp = false);
    void subCpuOrNetBalance(string memo, string &origin, const asset &quantity, string type);

    asset findContribution(string dapp, name contributor, string type);
    int findRamContribution(string dapp, name contributor);
    vector<balance::chosen_contributors> getContributors(string origin, string memo, uint64_t seed, uint64_t to, asset ram);

    void createJointAccount(string &memo, name &account, string &origin, accounts::authority &ownerAuth, accounts::authority &activeAuth, name referral);
    void createAccount(string dapp, name &account, accounts::authority &ownerauth, accounts::authority &activeauth, asset &ram, asset &net, asset &cpu, uint64_t pricekey, bool use_rex, bool isfixed, name referral);

    void fundloan(name to, asset quantity, string dapp, string type);
    void rentrexnet(string dapp, name account);
    void rentrexcpu(string dapp, name account);
    std::tuple<asset, asset> rextopup(name to, asset cpuquantity, asset netquantity, string dapp);

    void stakeCpuOrNet(name to, asset &net, asset &cpu);
    void addToUnstakedTable(name from, string dapp, asset net, asset cpu);
    void unstakeCpuOrNet(name from, name to, string dapp, bool unstakenet = true, bool unstakecpu = true);
    void addTotalUnstaked(const asset &quantity);
    void reclaimbwbalances(name from, string dapp);

    symbol getCoreSymbol();
    name getNewAccountContract();
    name getNewAccountAction();
    uint64_t getMinimumRAM();
    asset getRamCost(uint64_t ram_bytes, uint64_t priceKey);
    asset getFixedCpu(uint64_t priceKey);
    asset getFixedNet(uint64_t priceKey);
    auto getCpuLoanRecord(name account);
    auto getNetLoanRecord(name account);
};
}

