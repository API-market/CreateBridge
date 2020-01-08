#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

echo "-------------------"
echo "Balances"
cleos get table createescrow createescrow balances
echo ""
echo ""

echo "Contributors"
cleos get table createescrow createescrow contributors
echo ""
echo ""

echo "Registry"
cleos get table createescrow createescrow registry
echo ""
echo ""

echo "Token"
cleos get table createescrow createescrow token
echo ""
echo ""
echo "-------------------"