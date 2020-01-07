#include "lib/common.h"
#include "models/bandwidth.h"

namespace createescrow {
    void create_escrow::stakeCpuOrNet(name to, asset &net, asset &cpu)
    {
        action(
            permission_level{_self, "active"_n},
            common::getNewAccountContract(),
            name("delegatebw"),
            make_tuple(_self, to, net, cpu, false))
            .send();
    }

    void create_escrow::addToUnstakedTable(name from, string dapp, asset net, asset cpu)
    {
        bandwidth::Unstaked total_unstaked(_self, from.value);

        auto origin = common::toUUID(dapp);
        auto itr = total_unstaked.find(origin);

        if (itr == total_unstaked.end())
        {
            itr = total_unstaked.emplace(from, [&](auto &row) {
                row.reclaimer = from;
                row.net_balance = net;
                row.cpu_balance = cpu;
                row.dapp = dapp;
                row.origin = origin;
            });
        }
        else
        {
            total_unstaked.modify(itr, same_payer, [&](auto &row) {
                row.net_balance += net;
                row.cpu_balance += cpu;
            });
        }
    }

    void create_escrow::unstakeCpuOrNet(name from, name to, string dapp, bool unstakenet = true, bool unstakecpu = true)
    {
        asset zero_quantity = asset(0'0000, common::getCoreSymbol());
        asset net = zero_quantity;
        asset cpu = zero_quantity;

        bandwidth::del_bandwidth_table del_tbl(name("eosio"), _self.value);

        auto itr = del_tbl.find(to.value);
        if (itr != del_tbl.end())
        {
            if (unstakenet)
            {
                net = itr->net_weight;
            }

            if (unstakecpu)
            {
                cpu = itr->cpu_weight;
            }
        }

        // can only instake a positive amount
        if (net + cpu > zero_quantity)
        {
            action(
                permission_level{_self, "active"_n},
                common::getNewAccountContract(),
                name("undelegatebw"),
                make_tuple(_self, to, net, cpu))
                .send();
        }

        addToUnstakedTable(from, dapp, net, cpu);
    }

    void create_escrow::addTotalUnstaked(const asset &quantity)
    {
        bandwidth::Totalreclaim total_reclaim(_self, _self.value);
        auto iterator = total_reclaim.find(quantity.symbol.raw());

        if (iterator == total_reclaim.end())
        {
            total_reclaim.emplace(_self, [&](auto &row) {
                row.balance = quantity;
            });
        }
        else
        {
            total_reclaim.modify(iterator, same_payer, [&](auto &row) {
                row.balance += quantity;
            });
        }
    }

    void create_escrow::reclaimbwbalances(name from, string dapp)
    {
        bandwidth::Totalreclaim total_reclaim(_self, _self.value);

        symbol coreSymbol = common::getCoreSymbol();
        auto iterator = total_reclaim.find(coreSymbol.raw());

        bandwidth::Unstaked total_unstaked(_self, from.value);
        auto origin = common::toUUID(dapp);
        auto itr = total_unstaked.find(origin);

        if (iterator == total_reclaim.end())
        {
            eosio_assert(false, "Unstaking is still in progress. No balance available to be reclaimed");
        }

        if (itr == total_unstaked.end())
        {
            auto msg = ("No balance left to reclaim for " + dapp + " by " + from.to_string()).c_str();
            eosio_assert(false, msg);
        }
        else
        {
            asset reclaimed_quantity = itr->net_balance + itr->cpu_balance;

            auto memo = "reimburse the unstaked balance to " + from.to_string();

            action(
                permission_level{_self, "active"_n},
                name("eosio.token"),
                name("transfer"),
                make_tuple(_self, from, reclaimed_quantity, memo))
                .send();

            total_unstaked.erase(itr);

            total_reclaim.modify(iterator, same_payer, [&](auto &row) {
                row.balance -= reclaimed_quantity;
                if (row.balance.amount <= 0)
                {
                    total_reclaim.erase(iterator);
                }
            });
        }
    }
}

// class stakes
// {
// public:
//     name newAccountContract = common::getNewAccountContract();
//     name createescrow = common::createescrowContractName;

//     void stakeCpuOrNet(name to, asset &net, asset &cpu)
//     {
//         action(
//             permission_level{createescrow, "active"_n},
//             newAccountContract,
//             name("delegatebw"),
//             make_tuple(createescrow, to, net, cpu, false))
//             .send();
//     }

//     void addToUnstakedTable(name from, string dapp, asset net, asset cpu)
//     {
//         bandwidth::Unstaked total_unstaked(createescrow, from.value);

//         auto origin = common::toUUID(dapp);
//         auto itr = total_unstaked.find(origin);

//         if (itr == total_unstaked.end())
//         {
//             itr = total_unstaked.emplace(from, [&](auto &row) {
//                 row.reclaimer = from;
//                 row.net_balance = net;
//                 row.cpu_balance = cpu;
//                 row.dapp = dapp;
//                 row.origin = origin;
//             });
//         }
//         else
//         {
//             total_unstaked.modify(itr, same_payer, [&](auto &row) {
//                 row.net_balance += net;
//                 row.cpu_balance += cpu;
//             });
//         }
//     }

//     void unstakeCpuOrNet(name from, name to, string dapp, bool unstakenet = true, bool unstakecpu = true)
//     {
//         asset zero_quantity = asset(0'0000, common::getCoreSymbol());
//         asset net = zero_quantity;
//         asset cpu = zero_quantity;

//         bandwidth::del_bandwidth_table del_tbl(name("eosio"), createescrow.value);

//         auto itr = del_tbl.find(to.value);
//         if (itr != del_tbl.end())
//         {
//             if (unstakenet)
//             {
//                 net = itr->net_weight;
//             }

//             if (unstakecpu)
//             {
//                 cpu = itr->cpu_weight;
//             }
//         }

//         // can only instake a positive amount
//         if (net + cpu > zero_quantity)
//         {
//             action(
//                 permission_level{createescrow, "active"_n},
//                 newAccountContract,
//                 name("undelegatebw"),
//                 make_tuple(createescrow, to, net, cpu))
//                 .send();
//         }

//         addToUnstakedTable(from, dapp, net, cpu);
//     }

//     void addTotalUnstaked(const asset &quantity)
//     {
//         bandwidth::Totalreclaim total_reclaim(createescrow, createescrow.value);
//         auto iterator = total_reclaim.find(quantity.symbol.raw());

//         if (iterator == total_reclaim.end())
//         {
//             total_reclaim.emplace(createescrow, [&](auto &row) {
//                 row.balance = quantity;
//             });
//         }
//         else
//         {
//             total_reclaim.modify(iterator, same_payer, [&](auto &row) {
//                 row.balance += quantity;
//             });
//         }
//     }

//     void reclaimbwbalances(name from, string dapp)
//     {
//         bandwidth::Totalreclaim total_reclaim(createescrow, createescrow.value);

//         symbol coreSymbol = common::getCoreSymbol();
//         auto iterator = total_reclaim.find(coreSymbol.raw());

//         bandwidth::Unstaked total_unstaked(createescrow, from.value);
//         auto origin = common::toUUID(dapp);
//         auto itr = total_unstaked.find(origin);

//         if (iterator == total_reclaim.end())
//         {
//             eosio_assert(false, "Unstaking is still in progress. No balance available to be reclaimed");
//         }

//         if (itr == total_unstaked.end())
//         {
//             auto msg = ("No balance left to reclaim for " + dapp + " by " + from.to_string()).c_str();
//             eosio_assert(false, msg);
//         }
//         else
//         {
//             asset reclaimed_quantity = itr->net_balance + itr->cpu_balance;

//             auto memo = "reimburse the unstaked balance to " + from.to_string();

//             action(
//                 permission_level{createescrow, "active"_n},
//                 name("eosio.token"),
//                 name("transfer"),
//                 make_tuple(createescrow, from, reclaimed_quantity, memo))
//                 .send();

//             total_unstaked.erase(itr);

//             total_reclaim.modify(iterator, same_payer, [&](auto &row) {
//                 row.balance -= reclaimed_quantity;
//                 if (row.balance.amount <= 0)
//                 {
//                     total_reclaim.erase(iterator);
//                 }
//             });
//         }
//     }
// };
