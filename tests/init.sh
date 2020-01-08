#!/bin/bash

# eos server node address
# point to the current chain 
eosnode=http://127.0.0.1:8888

#NOTE: This script assumes that you have the keys for createescrow account in your unlocked wallet 

#cleos
cleos="cleos -u $eosnode"

CHAIN_SYMBOL=${1:-EOS}
SYMBOL_PRECISION=${2:-4}
NEWACCOUNT_CONTRACT=${3:-eosio}
NEWACCOUNT_ACTION=${4:-newaccount}
MINIMUM_RAM=${4:-4096}

# specify the chain symbol and the contract name to call for new account action 
$cleos push action createescrow init '["'$SYMBOL_PRECISION','$CHAIN_SYMBOL'","'$NEWACCOUNT_CONTRACT'","'$NEWACCOUNT_ACTION'","'$MINIMUM_RAM'"]' -p createescrow

symbol=$($cleos get table createescrow createescrow token | jq '.rows[].S_SYS')
echo "$symbol set successfully"
