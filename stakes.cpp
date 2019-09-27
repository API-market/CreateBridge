#include "lib/common.h"
#include "models/bandwidth.h"

class stakes
{
public:
    name createbridge = common::createbridgeName;

    void addToUnstakedTable(name from, string dapp, asset net, asset cpu)
    {
        bandwidth::Unstaked unstakes(createbridge, from.value);

        auto origin = common::toUUID(dapp);
        auto itr = unstakes.find(origin);

        if (itr == unstakes.end())
        {
            itr = unstakes.emplace(from, [&](auto &row) {
                row.reclaimer = from;
                row.net_balance = net;
                row.cpu_balance = cpu;
                row.dapp = dapp;
                row.origin = origin;
            });
        }
        else
        {
            unstakes.modify(itr, same_payer, [&](auto &row) {
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

        name newAccountContract = common::getNewAccountContract();

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

    void addTotalUnstaked(asset quantity)
    {
        bandwidth::Totalreclaim total_unstaked(createbridge, createbridge.value);
        auto iterator = total_unstaked.find(quantity.symbol.raw());

        if (iterator == total_unstaked.end())
            total_unstaked.emplace(createbridge, [&](auto &row) {
                row.balance = quantity;
            });
        else
            total_unstaked.modify(iterator, same_payer, [&](auto &row) {
                row.balance += quantity;
            });
    }
};
