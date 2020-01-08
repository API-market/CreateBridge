#!/bin/bash

# eos server node address
# point to the current chain 
eosnode=http://127.0.0.1:8888

#NOTE: This script assumes that you have the keys for the contributor in your unlocked wallet.
#NOTE: Also assumes that the accounts mydappuser11, mydappowner1 and contributor1 exists on the chain and their keys in an unlocked wallet
#NOTE: For airdrops, the script assumes that the dapp token contract is deployed under mydapptoken1, "DP" symbol created under it and issued to mydappowner1

#cleos
cleos="cleos -u $eosnode"

CHAIN_SYMBOL=${1:-EOS}
NAME=${2:-mydappuser11}
ORIGIN=${3:-mydapp.org}
DAPP_OWNER=${6:-oreidfunding}
CREATOR=${7:-contributor1}

# app registration
AIRDROP_JSON='{"contract":"", "tokens":"0 EOS", "limit":"0 EOS"}'
REX_JSON='{"net_loan_payment":"0.0000 EOS","net_loan_fund":"0.0000 EOS","cpu_loan_payment":"0.0000 EOS","cpu_loan_fund":"0.0000 EOS"}'
PARAMS_JSON='{"owner":"'$DAPP_OWNER'", "dapp":"'$ORIGIN'", "ram_bytes":"4096", "net":"1.0000 '$CHAIN_SYMBOL'", "cpu":"1.0000 '$CHAIN_SYMBOL'", "airdrop":'$AIRDROP_JSON', "use_rex": false, "rex":'$REX_JSON'}'
$cleos push action createescrow define "$PARAMS_JSON" -p $DAPP_OWNER

# whitelisting
$cleos push action createescrow whitelist '["'$DAPP_OWNER'","'$CREATOR'","'$ORIGIN'"]' -p $DAPP_OWNER

# contributions
$cleos transfer $DAPP_OWNER createescrow "100.0000 $CHAIN_SYMBOL" "$ORIGIN,50,100,0000,0000" -p $CREATOR

# create account
$cleos push action createescrow create '["'$CREATOR'","'$NAME'","EOS4xJvy2tYU21reKbbq4RPLxgzxNmrLtidVWpio5Ggwisfkgzg2L","EOS4xJvy2tYU21reKbbq4RPLxgzxNmrLtidVWpio5Ggwisfkgzg2L","'$ORIGIN'","'$CREATOR'"]' -p $CREATOR

# get newly created account
$cleos get account $NAME
