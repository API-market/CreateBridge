#pragma once

namespace bandwidth
{

struct delegated_bandwidth
{
    name from;
    name to;
    asset net_weight;
    asset cpu_weight;

    bool is_empty() const { return net_weight.amount == 0 && cpu_weight.amount == 0; }
    uint64_t primary_key() const { return to.value; }
};

typedef eosio::multi_index<"delband"_n, delegated_bandwidth> del_bandwidth_table;

struct [[ eosio::table, eosio::contract("createescrow") ]] unstakedBalStruct
{
    asset balance;

    uint64_t primary_key() const { return balance.symbol.raw(); }
};

typedef eosio::multi_index<"totalreclaim"_n, unstakedBalStruct> Totalreclaim;

struct [[ eosio::table, eosio::contract("createescrow") ]] unstakeStruct
{
    name reclaimer;
    asset net_balance;
    asset cpu_balance;
    string dapp;
    uint64_t origin;

    uint64_t primary_key() const { return origin; }

    EOSLIB_SERIALIZE(unstakeStruct, (reclaimer)(net_balance)(cpu_balance)(dapp)(origin))
};

typedef eosio::multi_index<"unstaked"_n, unstakeStruct> Unstaked;

} // namespace bandwidth