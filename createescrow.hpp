#pragma once

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/action.hpp>
#include <eosio/crypto.hpp>

#include "lib/common.h"

#include "models/accounts.h"
#include "models/balances.h"
#include "models/registry.h"
#include "models/bandwidth.h"

namespace createescrow
{

using namespace eosio;
using namespace common;
using namespace accounts;
using namespace balance;
using namespace bandwidth;
using namespace registry;
using namespace std;

class[[eosio::contract("createescrow")]] create_escrow : public eosio::contract
{

private:
  Registry dapps;
  Balances balances;
  Token token;

public:
  using contract::contract;

  create_escrow(name s, name code, datastream<const char *> ds);

  /**********************************************/
  /***                                        ***/
  /***                Actions                 ***/
  /***                                        ***/
  /**********************************************/

  [[eosio::action]] void clean();

  [[eosio::action]] void cleanreg();

  [[eosio::action]] void cleantoken();

  [[eosio::action]] void ping(name & from);

  /**********************************************/
  /***                                        ***/
  /***                Primary actions         ***/
  /***                                        ***/
  /**********************************************/

  [[eosio::action]] void init(const symbol &symbol, name newaccountcontract, name newaccountaction, uint64_t minimumram);

  [[eosio::action]] void define(name & owner, string dapp, uint64_t ram_bytes, asset net, asset cpu, uint64_t pricekey, registry::airdropdata & airdrop, bool use_rex, registry::rexdata &rex);

  [[eosio::action]] void whitelist(name owner, name account, string dapp);

  [[eosio::action]] void create(string & memo, name & account, public_key & ownerkey, public_key & activekey, string & origin, name referral);

  [[eosio::action]] void reclaim(name reclaimer, string dapp, string sym);

  /**********************************************/
  /***                                        ***/
  /***                Stakes                  ***/
  /***                                        ***/
  /**********************************************/

  [[eosio::action]] void refundstakes(name & from, string & origin);

  [[eosio::action]] void stake(name & from, name & to, string & origin, asset & net, asset & cpu);

  [[eosio::action]] void unstake(name & from, name & to, string & origin);

  [[eosio::action]] void unstakenet(name & from, name & to, string & origin);

  [[eosio::action]] void unstakecpu(name & from, name & to, string & origin);

  /**********************************************/
  /***                                        ***/
  /***                Rex                     ***/
  /***                                        ***/
  /**********************************************/

  [[eosio::action]] void fundnetloan(name & from, name & to, asset quantity, string & origin);

  [[eosio::action]] void fundcpuloan(name & from, name & to, asset quantity, string & origin);

  [[eosio::action]] void rentnet(name & from, name & to, string & origin);

  [[eosio::action]] void rentcpu(name & from, name & to, string & origin);

  [[eosio::action]] void topuploans(name & from, name & to, asset & cpuquantity, asset & netquantity, string & origin);

  void transfer(const name &from, const name &to, const asset &quantity, string &memo);

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

  /**********************************************/
  /***                                        ***/
  /***                Balances                ***/
  /***                                        ***/
  /**********************************************/

  void airdrop(string dapp, name account);
  asset balanceFor(string & memo);
  bool hasBalance(string memo, const asset &quantity);

  void addBalance(const name &from, const asset &quantity, string &memo);
  void subBalance(string memo, string & origin, const asset &quantity, bool memoIsDapp = false);
  void subCpuOrNetBalance(string memo, string & origin, const asset &quantity, string type);

  asset findContribution(string dapp, name contributor, string type);
  int findRamContribution(string dapp, name contributor);
  vector<balance::chosen_contributors> getContributors(string origin, string memo, uint64_t seed, uint64_t to, asset ram);

  /**********************************************/
  /***                                        ***/
  /***              Account creation          ***/
  /***                                        ***/
  /**********************************************/

  void createJointAccount(string & memo, name & account, string & origin, accounts::authority & ownerAuth, accounts::authority & activeAuth, name referral);
  void createAccount(string dapp, name & account, accounts::authority & ownerauth, accounts::authority & activeauth, asset & ram, asset & net, asset & cpu, uint64_t pricekey, bool use_rex, bool isfixed, name referral);

  /**********************************************/
  /***                                        ***/
  /***                Rex helpers             ***/
  /***                                        ***/
  /**********************************************/

  void fundloan(name to, asset quantity, string dapp, string type);
  void rentrexnet(string dapp, name account);
  void rentrexcpu(string dapp, name account);
  std::tuple<asset, asset> rextopup(name to, asset cpuquantity, asset netquantity, string dapp);

  /**********************************************/
  /***                                        ***/
  /***                Stakes                 ***/
  /***                                        ***/
  /**********************************************/

  void stakeCpuOrNet(name to, asset & net, asset & cpu);
  void addToUnstakedTable(name from, string dapp, asset net, asset cpu);
  void unstakeCpuOrNet(name from, name to, string dapp, bool unstakenet = true, bool unstakecpu = true);
  void addTotalUnstaked(const asset &quantity);
  void reclaimbwbalances(name from, string dapp);

  /**********************************************/
  /***                                        ***/
  /***                Helpers                 ***/
  /***                                        ***/
  /**********************************************/

  symbol getCoreSymbol();
  name getNewAccountContract();
  name getNewAccountAction();
  uint64_t getMinimumRAM();
  asset getRamCost(uint64_t ram_bytes, uint64_t priceKey);
  asset getFixedCpu(uint64_t priceKey);
  asset getFixedNet(uint64_t priceKey);

  auto getCpuLoanRecord(name account);
  auto getNetLoanRecord(name account);

  bool checkIfWhitelisted(name account, string dapp);
  bool checkIfOwner(name account, string dapp);
  bool checkIfOwnerOrWhitelisted(name account, string origin);
};
} // namespace createescrow
