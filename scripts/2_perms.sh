#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

# This script adds the code level permission to the active permission of createbridge 
# Arguments: 1. PKEY: active key of createbridge

PKEY=${1:-EOS7CNE3XxTMndjVQhnppDpLJ3upj6J4fukz2KamEtXiUkE4q4Fat}
alias cleosworbli='cleos -u https://worbli-testnet.eosblocksmith.io'
cleosworbli set account permission aikonworbli1 active \
'{"threshold": 1,"keys": [{"key": "'$PKEY'","weight": 1}],"accounts": [{"permission":{"actor":"aikonworbli1","permission":"eosio.code"},"weight":1}]}' owner