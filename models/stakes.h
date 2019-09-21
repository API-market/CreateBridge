#pragma once

namespace stakes {
   struct [[eosio::table, eosio::contract("createbridge")]] unstakedBalStruct {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.raw(); }
    };

    typedef eosio::multi_index<"totalunstake"_n, unstakedBalStruct> Totalunstake;
    
    struct [[eosio::table, eosio::contract("createbridge")]] unstakeStruct {
        uint64_t id;
        name reclaimer;
        asset cpu_balance;
        asset net_balance;
        string dapp;

        uint64_t primary_key() const { return id; }
        uint64_t by_reclaimer() const { return reclaimer.value; }
        EOSLIB_SERIALIZE(unstakeStruct, (id)(reclaimer)(cpu_balance)(net_balance)(dapp))
    };

    typedef eosio::multi_index<"unstaked"_n, unstakeStruct,
    indexed_by<"dapp"_n, const_mem_fun<unstakeStruct, uint64_t, &unstakeStruct::by_reclaimer>>> Unstaked;

}