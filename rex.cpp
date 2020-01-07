#include "lib/common.h"
#include "models/registry.h"

namespace createescrow {
    void create_escrow::rentnet(string dapp, name account)
    {
        registry::Registry dapps(_self, _self.value);
        auto iterator = dapps.find(common::toUUID(dapp));
        if (iterator != dapps.end())
            dapps.modify(iterator, same_payer, [&](auto &row) {
                // check if the dapp has opted for rex
                if (row.use_rex == true)
                {
                    action(
                        permission_level{_self, "active"_n},
                        common::getNewAccountContract(),
                        name("rentnet"),
                        make_tuple(_self, account, row.rex->net_loan_payment, row.rex->net_loan_fund))
                        .send();
                }
                else
                {
                    eosio_assert(row.use_rex, ("Rex not enabled for" + dapp).c_str());
                }
            });
    }

    void create_escrow::rentcpu(string dapp, name account)
    {
        registry::Registry dapps(_self, _self.value);
        auto iterator = dapps.find(common::toUUID(dapp));
        if (iterator != dapps.end())
            dapps.modify(iterator, same_payer, [&](auto &row) {
                // check if the dapp has opted for rex
                if (row.use_rex == true)
                {
                    action(
                        permission_level{_self, "active"_n},
                        common::getNewAccountContract(),
                        name("rentcpu"),
                        make_tuple(_self, account, row.rex->cpu_loan_payment, row.rex->cpu_loan_fund))
                        .send();
                }
                else
                {
                    eosio_assert(row.use_rex, ("Rex not enabled for" + dapp).c_str());
                }
            });
    }

    void create_escrow::fundloan(name to, asset quantity, string dapp, string type)
    {
        registry::Registry dapps(_self, _self.value);
        auto iterator = dapps.find(common::toUUID(dapp));

        eosio_assert(iterator->use_rex, ("Rex not enabled for" + dapp).c_str());

        if (type == "net")
        {
            auto loan_record = common::getNetLoanRecord(to);

            action(
                permission_level{_self, "active"_n},
                common::getNewAccountContract(),
                name("fundnetloan"),
                make_tuple(_self, loan_record->loan_num, quantity))
                .send();
        }

        if (type == "cpu")
        {
            auto loan_record = common::getCpuLoanRecord(to);

            action(
                permission_level{_self, "active"_n},
                common::getNewAccountContract(),
                name("fundcpuloan"),
                make_tuple(_self, loan_record->loan_num, quantity))
                .send();
        }
    }

    std::tuple<asset, asset> create_escrow::topup(name to, asset cpuquantity, asset netquantity, string dapp)
    {
        asset required_cpu_loan_bal;
        asset required_net_loan_bal;

        registry::Registry dapps(_self, _self.value);
        auto iterator = dapps.find(common::toUUID(dapp));

        eosio_assert(iterator->use_rex, ("Rex not enabled for" + dapp).c_str());

        auto net_loan_record = common::getNetLoanRecord(to);
        auto cpu_loan_record = common::getCpuLoanRecord(to);

        if (net_loan_record->receiver == to)
        {
            asset existing_net_loan_bal = net_loan_record->balance;
            required_net_loan_bal = netquantity - existing_net_loan_bal;

            if (required_net_loan_bal > asset(0'0000, common::getCoreSymbol()))
            {
                action(
                    permission_level{_self, "active"_n},
                    common::getNewAccountContract(),
                    name("fundnetloan"),
                    make_tuple(_self, net_loan_record->loan_num, required_net_loan_bal))
                    .send();
            }
        }
        else
        {
            // creates a new rex loan with the loan fund/balance provided in app registration
            create_escrow::rentnet(dapp, to);
        }

        if (cpu_loan_record->receiver == to)
        {
            asset existing_cpu_loan_bal = cpu_loan_record->balance;
            required_cpu_loan_bal = cpuquantity - existing_cpu_loan_bal;

            if (required_cpu_loan_bal > asset(0'0000, common::getCoreSymbol()))
            {
                action(
                    permission_level{_self, "active"_n},
                    common::getNewAccountContract(),
                    name("fundcpuloan"),
                    make_tuple(_self, cpu_loan_record->loan_num, required_cpu_loan_bal))
                    .send();
            }
        }
        else
        {
            // creates a new rex loan with the loan fund/balance provided in app registration
            required_cpu_loan_bal = iterator->rex->cpu_loan_payment + iterator->rex->cpu_loan_fund;
            required_net_loan_bal = iterator->rex->net_loan_payment + iterator->rex->net_loan_fund;
            create_escrow::rentcpu(dapp, to);
        }

        return make_tuple(required_cpu_loan_bal, required_net_loan_bal);
    }
}
// class rex
// {

// public:
//     name createescrow = common::createescrowContractName;
//     name newAccountContract = common::getNewAccountContract();
//     name newAccountAction = common::getNewAccountAction();

//     void rentnet(string dapp, name account)
//     {
//         registry::Registry dapps(createescrow, createescrow.value);
//         auto iterator = dapps.find(common::toUUID(dapp));
//         if (iterator != dapps.end())
//             dapps.modify(iterator, same_payer, [&](auto &row) {
//                 // check if the dapp has opted for rex
//                 if (row.use_rex == true)
//                 {
//                     action(
//                         permission_level{createescrow, "active"_n},
//                         newAccountContract,
//                         name("rentnet"),
//                         make_tuple(createescrow, account, row.rex->net_loan_payment, row.rex->net_loan_fund))
//                         .send();
//                 }
//                 else
//                 {
//                     eosio_assert(row.use_rex, ("Rex not enabled for" + dapp).c_str());
//                 }
//             });
//     }

//     void rentcpu(string dapp, name account)
//     {
//         registry::Registry dapps(createescrow, createescrow.value);
//         auto iterator = dapps.find(common::toUUID(dapp));
//         if (iterator != dapps.end())
//             dapps.modify(iterator, same_payer, [&](auto &row) {
//                 // check if the dapp has opted for rex
//                 if (row.use_rex == true)
//                 {
//                     action(
//                         permission_level{createescrow, "active"_n},
//                         newAccountContract,
//                         name("rentcpu"),
//                         make_tuple(createescrow, account, row.rex->cpu_loan_payment, row.rex->cpu_loan_fund))
//                         .send();
//                 }
//                 else
//                 {
//                     eosio_assert(row.use_rex, ("Rex not enabled for" + dapp).c_str());
//                 }
//             });
//     }

//     void fundloan(name to, asset quantity, string dapp, string type)
//     {
//         registry::Registry dapps(createescrow, createescrow.value);
//         auto iterator = dapps.find(common::toUUID(dapp));

//         eosio_assert(iterator->use_rex, ("Rex not enabled for" + dapp).c_str());

//         if (type == "net")
//         {
//             auto loan_record = common::getNetLoanRecord(to);

//             action(
//                 permission_level{createescrow, "active"_n},
//                 newAccountContract,
//                 name("fundnetloan"),
//                 make_tuple(createescrow, loan_record->loan_num, quantity))
//                 .send();
//         }

//         if (type == "cpu")
//         {
//             auto loan_record = common::getCpuLoanRecord(to);

//             action(
//                 permission_level{createescrow, "active"_n},
//                 newAccountContract,
//                 name("fundcpuloan"),
//                 make_tuple(createescrow, loan_record->loan_num, quantity))
//                 .send();
//         }
//     }

//     std::tuple<asset, asset> topup(name to, asset cpuquantity, asset netquantity, string dapp)
//     {
//         asset required_cpu_loan_bal;
//         asset required_net_loan_bal;

//         registry::Registry dapps(createescrow, createescrow.value);
//         auto iterator = dapps.find(common::toUUID(dapp));

//         eosio_assert(iterator->use_rex, ("Rex not enabled for" + dapp).c_str());

//         auto net_loan_record = common::getNetLoanRecord(to);
//         auto cpu_loan_record = common::getCpuLoanRecord(to);

//         if (net_loan_record->receiver == to)
//         {
//             asset existing_net_loan_bal = net_loan_record->balance;
//             required_net_loan_bal = netquantity - existing_net_loan_bal;

//             if (required_net_loan_bal > asset(0'0000, common::getCoreSymbol()))
//             {
//                 action(
//                     permission_level{createescrow, "active"_n},
//                     newAccountContract,
//                     name("fundnetloan"),
//                     make_tuple(createescrow, net_loan_record->loan_num, required_net_loan_bal))
//                     .send();
//             }
//         }
//         else
//         {
//             // creates a new rex loan with the loan fund/balance provided in app registration
//             rentnet(dapp, to);
//         }

//         if (cpu_loan_record->receiver == to)
//         {
//             asset existing_cpu_loan_bal = cpu_loan_record->balance;
//             required_cpu_loan_bal = cpuquantity - existing_cpu_loan_bal;

//             if (required_cpu_loan_bal > asset(0'0000, common::getCoreSymbol()))
//             {
//                 action(
//                     permission_level{createescrow, "active"_n},
//                     newAccountContract,
//                     name("fundcpuloan"),
//                     make_tuple(createescrow, cpu_loan_record->loan_num, required_cpu_loan_bal))
//                     .send();
//             }
//         }
//         else
//         {
//             // creates a new rex loan with the loan fund/balance provided in app registration
//             required_cpu_loan_bal = iterator->rex->cpu_loan_payment + iterator->rex->cpu_loan_fund;
//             required_net_loan_bal = iterator->rex->net_loan_payment + iterator->rex->net_loan_fund;
//             rentcpu(dapp, to);
//         }

//         return make_tuple(required_cpu_loan_bal, required_net_loan_bal);
//     }
// };