#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

# This script calls the DEFINE action of createbridge to register an dapp with createbridge
# Arguments: 1. DAPP_OWNER: account name to be registered as the owner of the dapp
#            2. ORIGIN:                     a unique string to register the dapp
#            3. RAM_BYTES:                  bytes of RAM to put in the new user accounts created for the dapp
#            4. NET:                        amount of core tokens to be staked for net bandwidth for the new user accounts
#            5. CPU:                        amount of core tokens to be staked for cpu for the new user accounts
#            6. AIRDROP_TOKEN_CONTRACT:     the contract name under which the dapp token is deployed 
#            7. AIRDROP_TOKEN_TOTAL:        the total number of dapp tokens to be airdropped
#            8. AIRDROP_TOKEN_LIMIT:        the number of dapp tokens to be airdropped to every new account created
#            9. CUSTODIAN_ACCOUNT:          the account to whitelist to create new user accounts for the dapp on behalf of the owner
#            10.PRICEKEY:                   the pricing tier for the new account (only relevent for accounts on ORE chain - an alternative to specifing RAM_BYTES)
#            11.REX_USE_REX:                whether REX should be used to automatically buy REX loans for CPU and NET (for new account)
#            12.REX_NET_LOAN_PAYMENT:       tokens paid for the loan of NET resources
#            13.REX_NET_LOAN_FUND:          additional tokens added to REX loan fund and used later (by REX) for NET loan renewal
#            14.REX_CPU_LOAN_PAYMENT:       tokens paid for the loan of CPU resources
#            15.REX_CPU_LOAN_FUND:          additional tokens added to REX loan fund and used later (by REX) for CPU loan renewal

#NOTE: This script assumes that you have the keys for the DAPP_OWNER in your unlocked wallet

CREATE_BRIDGE_CONTRACT='createbridge'
DAPP_OWNER=${1:-eosio}
ORIGIN=${2:-test.com}
RAM_BYTES=${3:-5120}
NET=${4:-"0.0000 EOS"}
CPU=${5:-"0.0000 EOS"}
AIRDROP_TOKEN_CONTRACT=${6:-exampletoken}
AIRDROP_TOKEN_TOTAL=${7:-'10000000.000 EX'}
AIRDROP_TOKEN_LIMIT=${8:-'10.000 EX'}
CUSTODIAN_ACCOUNT=${9:-appcustodian}
PRICEKEY=${10:-1}
REX_USE_REX=${11:0}
REX_NET_LOAN_PAYMENT=${12:'0.0000 EOS'}
REX_NET_LOAN_FUND=${13:'0.0000 EOS'}
REX_CPU_LOAN_PAYMENT=${14:'0.0000 EOS'}
REX_CPU_LOAN_FUND=${15:'0.0000 EOS'}

# app registration
AIRDROP_JSON='{"contract":"'$AIRDROP_TOKEN_CONTRACT'", "tokens":"'$AIRDROP_TOKEN_TOTAL'", "limit":"'$AIRDROP_TOKEN_LIMIT'"}'
REX_JSON='{"net_loan_payment":"'$REX_NET_LOAN_PAYMENT'", "net_loan_fund":"'$REX_NET_LOAN_FUND'", "cpu_loan_payment":"'$REX_CPU_LOAN_PAYMENT'", "cpu_loan_fund":"'$REX_CPU_LOAN_FUND'"}'
PARAMS_JSON='{"owner":"'$DAPP_OWNER'", "dapp":"'$ORIGIN'", "ram_bytes":"'$RAM_BYTES'", "net":"'$NET'", "cpu":"'$CPU'", "airdrop":'$AIRDROP_JSON', "pricekey":'$PRICEKEY', "use_rex":"'$REX_USE_REX'", "rex":'$REX_JSON'}'

#PARAMS_JSON='{"owner":"'$DAPP_OWNER'", "dapp":"'$ORIGIN'", "ram":"'$RAM'", "net":"'$NET'", "cpu":"'$CPU'", "airdrop":null}'
cleos push action "$CREATE_BRIDGE_CONTRACT" define "$PARAMS_JSON" -p $DAPP_OWNER

# send the airdrop tokens to createbridge
TRANSFER_JSON='{"from":"'$DAPP_OWNER'","to":"'$CREATE_BRIDGE_CONTRACT'","quantity":"'$AIRDROP_TOKEN_TOTAL'","memo":"transfer airdrop tokens"}'
cleos push action $AIRDROP_TOKEN_CONTRACT transfer "$TRANSFER_JSON" -p $DAPP_OWNER

# whitelist other accounts
cleos push action "$CREATE_BRIDGE_CONTRACT" whitelist '["'$DAPP_OWNER'","'$CUSTODIAN_ACCOUNT'","'$ORIGIN'"]' -p $DAPP_OWNER
