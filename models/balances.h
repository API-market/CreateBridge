#pragma once

namespace balance
{

struct [[ eosio::table, eosio::contract("createescrow") ]] contributors
{
    name contributor;
    asset balance; // used to pay for ram
    int ram;       // percentage of ram cost the contributor wants to fund
    asset net_balance;
    asset cpu_balance;
    int totalaccounts;   // specify the limit for the number of accounts to be created by the given contribution
    int createdaccounts; // number of accounts created using the contributor's funds
};

struct chosen_contributors
{
    name contributor;
    asset rampay; // amount the contributor is contributing towards RAM cost for new account
};

struct [[ eosio::table, eosio::contract("createescrow") ]] balances
{
    uint64_t memo;
    vector<contributors> contributors;
    asset balance;
    string origin;
    uint64_t timestamp;

    uint64_t primary_key() const { return memo; }
};

typedef eosio::multi_index<"balances"_n, balances> Balances;
} // namespace balance
