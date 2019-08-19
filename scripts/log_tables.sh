#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

echo "-------------------"
echo "Balances"
cleos get table oreorebridge oreorebridge balances
echo ""
echo ""

echo "Contributors"
cleos get table oreorebridge oreorebridge contributors
echo ""
echo ""

echo "Registry"
cleos get table oreorebridge oreorebridge registry
echo ""
echo ""

echo "Token"
cleos get table oreorebridge oreorebridge token
echo ""
echo ""
echo "-------------------"