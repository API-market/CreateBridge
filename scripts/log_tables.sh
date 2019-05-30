#!/bin/bash

shopt -s expand_aliases
source ~/.bash_aliases

echo "-------------------"
echo "Balances"
cleos get table aikonworbli1 aikonworbli1 balances
echo ""
echo ""

echo "Contributors"
cleos get table aikonworbli1 aikonworbli1 contributors
echo ""
echo ""

echo "Registry"
cleos get table aikonworbli1 aikonworbli1 registry
echo ""
echo ""

echo "Token"
cleos get table aikonworbli1 aikonworbli1 token
echo ""
echo ""
echo "-------------------"