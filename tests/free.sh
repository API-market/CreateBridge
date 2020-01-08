#!/bin/bash

# eos server node address
# point to the current chain 
eosnode=http://127.0.0.1:8888

#NOTE: This script assumes you have the keys for createescrow in your unlocked wallet
#NOTE: This script also assumes that the accounts mydappowner1 and contributor1 exists on the chain and their keys in an unlocked wallet

#cleos
cleos="cleos -u $eosnode"

NAME=${1:-mydappuser12}
ORIGIN=${2:-free}
MEMO=${3:-asdf}
CONTRIBUTOR=${4:-oreidfunding}
CREATOR=${5:-oreidfunding}

AIRDROP_JSON='{"contract":"", "tokens":"0 EOS", "limit":"0 EOS"}'
REX_JSON='{"net_loan_payment":"0.0000 EOS","net_loan_fund":"0.0000 EOS","cpu_loan_payment":"0.0000 EOS","cpu_loan_fund":"0.0000 EOS"}'
PARAMS_JSON='{"owner":"createescrow", "dapp":"'$ORIGIN'", "ram_bytes":"4096", "net":"0.0000 EOS", "cpu":"0.0000 EOS", "pricekey":0,"airdrop":'$AIRDROP_JSON',"use_rex": false, "rex":'$REX_JSON'}'
$cleos push action createescrow define "$PARAMS_JSON" -p createescrow@active

$cleos transfer $CONTRIBUTOR createescrow "1.0000 EOS" "free,100,-1,00000,00000" -p $CONTRIBUTOR

$cleos push action createescrow create '["'$CREATOR'", "'$NAME'", "EOS4xJvy2tYU21reKbbq4RPLxgzxNmrLtidVWpio5Ggwisfkgzg2L","EOS4xJvy2tYU21reKbbq4RPLxgzxNmrLtidVWpio5Ggwisfkgzg2L", "'$ORIGIN'", "'$CREATOR'"]' -p $CREATOR

$cleos get account $NAME