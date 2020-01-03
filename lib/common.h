#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosio/time.hpp>

using namespace eosio;
using std::string;
using std::vector;

namespace common
{
static const symbol S_RAM = symbol("RAMCORE", 4);
static const name createescrowContractName = name("createescrow");

inline static uint64_t toUUID(string username)
{
    return std::hash<string>{}(username);
}

std::vector<std::string> split(const string &str, const string &delim)
{
    vector<string> parts;
    if (str.size() == 0)
        return parts;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos)
            pos = str.length();
        string token = str.substr(prev, pos - prev);
        if (!token.empty())
            parts.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return parts;
}

uint64_t generate_random(uint64_t seed, uint64_t val)
{
    const uint64_t a = 1103515245;
    const uint64_t c = 12345;

    // using LCG alogrithm : https://en.wikipedia.org/wiki/Linear_congruential_generator
    // used by standard c++ rand function : http://pubs.opengroup.org/onlinepubs/009695399/functions/rand.html

    uint64_t seed2 = (uint32_t)((a * seed + c) % 0x7fffffff);
    uint64_t value = ((uint64_t)seed2 * val) >> 31;

    return value;
}

/**********************************************/
/***                                        ***/
/***            Chain constants             ***/
/***                                        ***/
/**********************************************/

struct [[ eosio::table, eosio::contract("createescrow") ]] token
{
    symbol S_SYS;
    name newaccountcontract;
    name newaccountaction;
    uint64_t min_ram;
    uint64_t primary_key() const { return S_SYS.raw(); }
};

typedef eosio::multi_index<"token"_n, token> Token;

/***
     * Returns the symbol of the core token of the chain or the token used to pay for new account creation
     * @return
     */
symbol getCoreSymbol()
{
    Token token(createescrowContractName, createescrowContractName.value);
    return token.begin()->S_SYS;
}

/***
     * Returns the contract name for new account action
     */
name getNewAccountContract()
{
    Token token(createescrowContractName, createescrowContractName.value);
    return token.begin()->newaccountcontract;
}

name getNewAccountAction()
{
    Token token(createescrowContractName, createescrowContractName.value);
    return token.begin()->newaccountaction;
}

/**
     * Returns the minimum bytes of RAM for new account creation
     */
uint64_t getMinimumRAM()
{
    Token token(createescrowContractName, createescrowContractName.value);
    return token.begin()->min_ram;
}

/**********************************************/
/***                                        ***/
/***            RAM calculations            ***/
/***                                        ***/
/**********************************************/

struct rammarket
{
    asset supply;

    struct connector
    {
        asset balance;
        double weight = .5;
    };

    connector base;
    connector quote;

    uint64_t primary_key() const { return supply.symbol.raw(); }
};

struct pricetable
{
    uint64_t key;
    asset createprice; // newaccount price as ORE
    uint64_t rambytes; // initial amount of ram
    asset netamount;   // initial amount of net
    asset cpuamount;   // initial amount of cpu

    uint64_t primary_key() const { return key; }
};

typedef eosio::multi_index<"pricetable"_n, pricetable> priceTable;

typedef eosio::multi_index<"rammarket"_n, rammarket> RamInfo;

/***
     * Returns the price of ram for given bytes
     */

asset getRamCost(uint64_t ram_bytes, uint64_t priceKey)
{
    asset ramcost;
    if (ram_bytes > 0)
    {
        RamInfo ramInfo(name("eosio"), name("eosio").value);
        auto ramData = ramInfo.find(S_RAM.raw());
        symbol coreSymbol = getCoreSymbol();
        eosio_assert(ramData != ramInfo.end(), "Could not get RAM info");

        uint64_t base = ramData->base.balance.amount;
        uint64_t quote = ramData->quote.balance.amount;
        ramcost = asset((((double)quote / base)) * ram_bytes, coreSymbol);
    }
    else
    { //if account is tier fixed
        Token token(createescrowContractName, createescrowContractName.value);
        name newaccountcontract = getNewAccountContract();
        priceTable price(newaccountcontract, newaccountcontract.value);
        auto priceItr = price.find(priceKey);
        ramcost.amount = priceItr->createprice.amount - (priceItr->netamount.amount + priceItr->cpuamount.amount);
        ramcost.symbol = priceItr->createprice.symbol;
    }
    return ramcost;
}

asset getFixedCpu(uint64_t priceKey)
{
    name newaccountcontract = getNewAccountContract();
    priceTable price(newaccountcontract, newaccountcontract.value);
    auto priceItr = price.find(priceKey);
    return priceItr->cpuamount;
}

asset getFixedNet(uint64_t priceKey)
{
    name newaccountcontract = getNewAccountContract();
    priceTable price(newaccountcontract, newaccountcontract.value);
    auto priceItr = price.find(priceKey);
    return priceItr->netamount;
}

struct rex_loan
{
    uint8_t version = 0;
    name from;
    name receiver;
    asset payment;
    asset balance;
    asset total_staked;
    uint64_t loan_num;
    eosio::time_point expiration;

    uint64_t primary_key() const { return loan_num; }
    uint64_t by_expr() const { return expiration.elapsed.count(); }
    uint64_t by_owner() const { return from.value; }
};

// get rex loan number for the user account
typedef eosio::multi_index<"cpuloan"_n, rex_loan,
                           indexed_by<"byexpr"_n, const_mem_fun<rex_loan, uint64_t, &rex_loan::by_expr>>,
                           indexed_by<"byowner"_n, const_mem_fun<rex_loan, uint64_t, &rex_loan::by_owner>>>
    rex_cpu_loan_table;

typedef eosio::multi_index<"netloan"_n, rex_loan,
                           indexed_by<"byexpr"_n, const_mem_fun<rex_loan, uint64_t, &rex_loan::by_expr>>,
                           indexed_by<"byowner"_n, const_mem_fun<rex_loan, uint64_t, &rex_loan::by_owner>>>
    rex_net_loan_table;

auto getCpuLoanRecord(name account)
{
    rex_cpu_loan_table cpu_loans(name("eosio"), name("eosio").value);
    auto cpu_idx = cpu_loans.get_index<"byowner"_n>();
    auto loans = cpu_idx.find(createescrowContractName.value);

    auto i = cpu_idx.lower_bound(createescrowContractName.value);

    while (i != cpu_idx.end())
    {
        if (i->receiver == account)
        {
            return i;
        };
        i++;
    };

    eosio_assert(false, ("No existing loan found for" + account.to_string()).c_str());
}

auto getNetLoanRecord(name account)
{
    rex_net_loan_table net_loans(name("eosio"), name("eosio").value);
    auto net_idx = net_loans.get_index<"byowner"_n>();
    auto loans = net_idx.find(createescrowContractName.value);

    auto i = net_idx.lower_bound(createescrowContractName.value);

    while (i != net_idx.end())
    {
        if (i->receiver == account)
        {
            return i;
        };
        i++;
    };

    eosio_assert(false, ("No existing loan found for" + account.to_string()).c_str());
}

}; // namespace common