#include "lib/common.h"
#include "models/registry.h"

class rex
{

public:
    name createbridge = common::createbridgeName;
    name newAccountContract = common::getNewAccountContract();
    name newAccountAction = common::getNewAccountAction();

    void rentnet(string dapp, name account)
    {
        registry::Registry dapps(createbridge, createbridge.value);
        auto iterator = dapps.find(common::toUUID(dapp));
        if (iterator != dapps.end())
            dapps.modify(iterator, same_payer, [&](auto &row) {
                // check if the dapp has opted for rex
                if (row.use_rex == true)
                {
                    action(
                        permission_level{createbridge, "active"_n},
                        newAccountContract,
                        name("rentnet"),
                        make_tuple(createbridge, account, row.rex->net_loan_payment, row.rex->net_loan_fund))
                        .send();
                }
                else
                {
                    eosio_assert(row.use_rex, ("Rex not enabled for" + dapp).c_str());
                }
            });
    }

    void rentcpu(string dapp, name account)
    {
        registry::Registry dapps(createbridge, createbridge.value);
        auto iterator = dapps.find(common::toUUID(dapp));
        if (iterator != dapps.end())
            dapps.modify(iterator, same_payer, [&](auto &row) {
                // check if the dapp has opted for rex
                if (row.use_rex == true)
                {
                    action(
                        permission_level{createbridge, "active"_n},
                        newAccountContract,
                        name("rentcpu"),
                        make_tuple(createbridge, account, row.rex->cpu_loan_payment, row.rex->cpu_loan_fund))
                        .send();
                }
                else
                {
                    eosio_assert(row.use_rex, ("Rex not enabled for" + dapp).c_str());
                }
            });
    }

    void fundloan(name from, name to, asset quantity, string dapp, string type)
    {
        registry::Registry dapps(createbridge, createbridge.value);
        auto iterator = dapps.find(common::toUUID(dapp));

        eosio_assert(iterator->use_rex, ("Rex not enabled for" + dapp).c_str());

        if (type == "net")
        {
            uint64_t loan_num = common::getNetLoanNumber(to);

            action(
                permission_level{createbridge, "active"_n},
                newAccountContract,
                name("fundnetloan"),
                make_tuple(createbridge, loan_num, quantity))
                .send();
        }

        if (type == "cpu")
        {
            uint64_t loan_num = common::getCpuLoanNumber(to);

            action(
                permission_level{createbridge, "active"_n},
                newAccountContract,
                name("fundcpuloan"),
                make_tuple(createbridge, loan_num, quantity))
                .send();
        }
    }
};