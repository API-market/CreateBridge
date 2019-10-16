#include "lib/common.h"
#include "models/bandwidth.h"

class stakes
{
public:
    name newAccountContract = common::getNewAccountContract();
    name createbridge = common::createbridgeName;

    void stakeCpuOrNet(name to, asset &net, asset &cpu)
    {
        action(
            permission_level{createbridge, "active"_n},
            newAccountContract,
            name("delegatebw"),
            make_tuple(createbridge, to, net, cpu, false))
            .send();
    }

    void addToUnstakedTable(name from, string dapp, asset net, asset cpu)
    {
        bandwidth::Unstaked total_unstaked(createbridge, from.value);

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

    void unstakeCpuOrNet(name from, name to, string dapp, bool unstakenet = true, bool unstakecpu = true)
    {
        asset zero_quantity = asset(0'0000, common::getCoreSymbol());
        asset net = zero_quantity;
        asset cpu = zero_quantity;

        bandwidth::del_bandwidth_table del_tbl(name("eosio"), createbridge.value);

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
                permission_level{createbridge, "active"_n},
                newAccountContract,
                name("undelegatebw"),
                make_tuple(createbridge, to, net, cpu))
                .send();
        }

        addToUnstakedTable(from, dapp, net, cpu);
    }

    void addTotalUnstaked(const asset &quantity)
    {
        bandwidth::Totalreclaim total_reclaim(createbridge, createbridge.value);
        auto iterator = total_reclaim.find(quantity.symbol.raw());

        if (iterator == total_reclaim.end())
        {
            total_reclaim.emplace(createbridge, [&](auto &row) {
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

    void reclaimbwbalances(name from, string dapp)
    {
        asset zero_quantity = asset(0'0000, common::getCoreSymbol());

        bandwidth::Totalreclaim total_reclaim(createbridge, createbridge.value);

        symbol coreSymbol = common::getCoreSymbol();
        auto iterator = total_reclaim.find(coreSymbol.raw());

        bandwidth::Unstaked total_unstaked(createbridge, from.value);
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
                permission_level{createbridge, "active"_n},
                name("eosio.token"),
                name("transfer"),
                make_tuple(createbridge, from, reclaimed_quantity, memo))
                .send();

            total_unstaked.modify(itr, same_payer, [&](auto &row) {
                row.net_balance = zero_quantity;
                row.cpu_balance = zero_quantity;
            });

            total_reclaim.modify(iterator, same_payer, [&](auto &row) {
                row.balance -= reclaimed_quantity;
            });
        }
    }
};
