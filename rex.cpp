#include "lib/common.h"
#include "models/registry.h"

#include "createescrow.hpp"

#include "constants.cpp"

namespace createescrow
{
// finds the existing net loan from createescrow to user account and funds it
void create_escrow::fundnetloan(name &from, name &to, asset quantity, string &origin)
{
    create_escrow::checkIfOwnerOrWhitelisted(from, origin);

    create_escrow::fundloan(to, quantity, origin, "net");
    create_escrow::subCpuOrNetBalance(from.to_string(), origin, quantity, "net");
}

// finds the existing cpu loan from createescrow to user account and funds it
void create_escrow::fundcpuloan(name &from, name &to, asset quantity, string &origin)
{
    create_escrow::checkIfOwnerOrWhitelisted(from, origin);

    create_escrow::fundloan(to, quantity, origin, "cpu");
    create_escrow::subCpuOrNetBalance(from.to_string(), origin, quantity, "cpu");
}

// creates a new loan from createescrow to the user acount (to)
void create_escrow::rentnet(name &from, name &to, string &origin)
{
    create_escrow::checkIfOwnerOrWhitelisted(from, origin);

    auto iterator = dapps.find(common::toUUID(origin));

    create_escrow::rentrexnet(origin, to);

    asset quantity = iterator->rex->net_loan_payment + iterator->rex->net_loan_fund;
    create_escrow::subCpuOrNetBalance(from.to_string(), origin, quantity, "net");
}

// creates a new loan from createescrow to the user acount (to)
void create_escrow::rentcpu(name &from, name &to, string &origin)
{
    create_escrow::checkIfOwnerOrWhitelisted(from, origin);

    auto iterator = dapps.find(common::toUUID(origin));

    create_escrow::rentrexcpu(origin, to);

    asset quantity = iterator->rex->cpu_loan_payment + iterator->rex->cpu_loan_fund;
    create_escrow::subCpuOrNetBalance(from.to_string(), origin, quantity, "cpu");
}

// topup existing loan balance (cpu & net) for a user up to the given quantity
// If no existing loan, then create a new loan
void create_escrow::topuploans(name &from, name &to, asset &cpuquantity, asset &netquantity, string &origin)
{
    create_escrow::checkIfOwnerOrWhitelisted(from, origin);

    asset required_net_bal, required_cpu_bal;
    tie(required_net_bal, required_cpu_bal) = create_escrow::rextopup(to, cpuquantity, netquantity, origin);

    create_escrow::subCpuOrNetBalance(from.to_string(), origin, required_net_bal, "net");
    create_escrow::subCpuOrNetBalance(from.to_string(), origin, required_cpu_bal, "cpu");
}

void create_escrow::rentrexnet(string dapp, name account)
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
                    create_escrow::getNewAccountContract(),
                    name("rentnet"),
                    make_tuple(_self, account, row.rex->net_loan_payment, row.rex->net_loan_fund))
                    .send();
            }
            else
            {
                check(row.use_rex, ("Rex not enabled for" + dapp).c_str());
            }
        });
}

void create_escrow::rentrexcpu(string dapp, name account)
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
                    create_escrow::getNewAccountContract(),
                    name("rentcpu"),
                    make_tuple(_self, account, row.rex->cpu_loan_payment, row.rex->cpu_loan_fund))
                    .send();
            }
            else
            {
                check(row.use_rex, ("Rex not enabled for" + dapp).c_str());
            }
        });
}

void create_escrow::fundloan(name to, asset quantity, string dapp, string type)
{
    registry::Registry dapps(_self, _self.value);
    auto iterator = dapps.find(common::toUUID(dapp));

    check(iterator->use_rex, ("Rex not enabled for" + dapp).c_str());

    if (type == "net")
    {
        auto loan_record = create_escrow::getNetLoanRecord(to);

        action(
            permission_level{_self, "active"_n},
            create_escrow::getNewAccountContract(),
            name("fundnetloan"),
            make_tuple(_self, loan_record->loan_num, quantity))
            .send();
    }

    if (type == "cpu")
    {
        auto loan_record = create_escrow::getCpuLoanRecord(to);

        action(
            permission_level{_self, "active"_n},
            create_escrow::getNewAccountContract(),
            name("fundcpuloan"),
            make_tuple(_self, loan_record->loan_num, quantity))
            .send();
    }
}

std::tuple<asset, asset> create_escrow::rextopup(name to, asset cpuquantity, asset netquantity, string dapp)
{
    asset required_cpu_loan_bal;
    asset required_net_loan_bal;

    registry::Registry dapps(_self, _self.value);
    auto iterator = dapps.find(common::toUUID(dapp));

    check(iterator->use_rex, ("Rex not enabled for" + dapp).c_str());

    auto net_loan_record = create_escrow::getNetLoanRecord(to);
    auto cpu_loan_record = create_escrow::getCpuLoanRecord(to);

    if (net_loan_record->receiver == to)
    {
        asset existing_net_loan_bal = net_loan_record->balance;
        required_net_loan_bal = netquantity - existing_net_loan_bal;

        if (required_net_loan_bal > asset(0'0000, create_escrow::getCoreSymbol()))
        {
            action(
                permission_level{_self, "active"_n},
                create_escrow::getNewAccountContract(),
                name("fundnetloan"),
                make_tuple(_self, net_loan_record->loan_num, required_net_loan_bal))
                .send();
        }
    }
    else
    {
        // creates a new rex loan with the loan fund/balance provided in app registration
        create_escrow::rentrexnet(dapp, to);
    }

    if (cpu_loan_record->receiver == to)
    {
        asset existing_cpu_loan_bal = cpu_loan_record->balance;
        required_cpu_loan_bal = cpuquantity - existing_cpu_loan_bal;

        if (required_cpu_loan_bal > asset(0'0000, create_escrow::getCoreSymbol()))
        {
            action(
                permission_level{_self, "active"_n},
                create_escrow::getNewAccountContract(),
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
        create_escrow::rentrexcpu(dapp, to);
    }

    return make_tuple(required_cpu_loan_bal, required_net_loan_bal);
}
} // namespace createescrow