#pragma once

using namespace eosio;
using namespace std;

namespace registry
{

struct airdropdata
{
    name contract;
    asset balance;
    asset amount;
};

struct rexdata
{
    asset net_loan_payment;
    asset net_loan_fund;
    asset cpu_loan_payment;
    asset cpu_loan_fund;
};

struct [[ eosio::table, eosio::contract("createescrow") ]] registryStruct
{
    name owner;
    string dapp;
    uint64_t ram_bytes;
    asset net;
    asset cpu;
    vector<name> custodians;
    uint64_t pricekey;
    bool use_rex;

    std::optional<airdropdata> airdrop;
    std::optional<rexdata> rex;

    uint64_t primary_key() const { return common::toUUID(dapp); }

    EOSLIB_SERIALIZE(registryStruct, (owner)(dapp)(ram_bytes)(net)(cpu)(custodians)(pricekey)(use_rex)(airdrop)(rex))
};

typedef eosio::multi_index<"registry"_n, registryStruct> Registry;
} // namespace registry
