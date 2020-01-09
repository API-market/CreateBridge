#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

# This script calls the DEFINE action of createescrow to register an dapp with createescrow
# Arguments: 1. DAPP_OWNER: account name to be registered as the owner of the dapp
#            2. ORIGIN:                     a unique string to register the dapp
#            3. CORE_SYMBOL:                core symbol of the chain 
#            4. RAM_BYTES:                  bytes of RAM to put in the new user accounts created for the dapp
#            5. NET:                        amount of core tokens to be staked for net bandwidth for the new user accounts
#            6. CPU:                        amount of core tokens to be staked for cpu for the new user accounts
#            7. AIRDROP_TOKEN_CONTRACT:     the contract name under which the dapp token is deployed 
#            8. AIRDROP_SYMBOL:             symbol of dapp tokens
#            9. AIRDROP_TOKEN_TOTAL:        the total number of dapp tokens to be airdropped
#            10.AIRDROP_TOKEN_LIMIT:        the number of dapp tokens to be airdropped to every new account created
#            11.CUSTODIAN_ACCOUNT:          the account to whitelist to create new user accounts for the dapp on behalf of the owner

#NOTE: This script assumes that you have the keys for the DAPP_OWNER in your unlocked wallet


DAPP_OWNER=${1:-eosio}
ORIGIN=${2:-test.com}
RAM_BYTES=${3:-5120}
NET=${4:-"1.0000 EOS"}
CPU=${5:-"1.0000 EOS"}
AIRDROP_TOKEN_CONTRACT=${6:-exampletoken}
AIRDROP_TOKEN_TOTAL=${7:-'1000.0000 EX'}
AIRDROP_TOKEN_LIMIT=${8:-'10.0000 EX'}
CUSTODIAN_ACCOUNT=${9:-appcustodian}
PRICE_KEY=${10:-1}

# app registration
AIRDROP_JSON='{"contract":"'$AIRDROP_TOKEN_CONTRACT'", "tokens":"'$AIRDROP_TOKEN_TOTAL'", "limit":"'$AIRDROP_TOKEN_LIMIT'"}'
REX_JSON='{"net_loan_payment":"0.0000 EOS","net_loan_fund":"0.0000 EOS","cpu_loan_payment":"0.0000 EOS","cpu_loan_fund":"0.0000 EOS"}'

PARAMS_JSON='{"owner":"'$DAPP_OWNER'", "dapp":"'$ORIGIN'", "ram_bytes":"'$RAM_BYTES'", "net":"'$NET'", "cpu":"'$CPU'", "airdrop":'$AIRDROP_JSON', "pricekey":'$PRICE_KEY', "use_rex": false, "rex": '$REX_JSON'}'

cleos push action createescrow define "$PARAMS_JSON" -p $DAPP_OWNER

# send the airdrop tokens to createescrow
TRANSFER_JSON='{"from":"'$DAPP_OWNER'","to":"createescrow","quantity":"'$AIRDROP_TOKEN_TOTAL'","memo":"transfer airdrop tokens"}'
cleos push action $AIRDROP_TOKEN_CONTRACT transfer "$TRANSFER_JSON" -p $DAPP_OWNER

# whitelist other accounts
cleos push action createescrow whitelist '["'$DAPP_OWNER'","'$CUSTODIAN_ACCOUNT'","'$ORIGIN'"]' -p $DAPP_OWNER
