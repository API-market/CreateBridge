# This script deploys latest version of createescrow on EOS jungle, EOS kylin, EOS mainnet, ORE staging and ORE mainnet networks.
# Pre-requisite: Requires createescrow keys in your local cleos wallet. Get the keys from lastpass.

#### EOS NETWORK ####
mainnetnode=https://api.cypherglass.com:443
junglenode=https://jungle2.cryptolions.io:443
kylinnode=https://api-kylin.eosasia.one:443

#### ORE NETWORK ####
orestagingnode=https://ore-staging.openrights.exchange:443
oremainnetnode=https://ore.openrights.exchange:443

cleosmainnet="cleos -u $mainnetnode"
cleosjungle="cleos -u $junglenode"
cleoskylin="cleos -u $kylinnode"

cleosorestaging="cleos -u $orestagingnode"
cleosoremain="cleos -u $oremainnetnode"

CONTRACT_NAME=${1:-createescrow}
# change the CONTRACT_DIR to point to createescrow repo in you local repository
CONTRACT_DIR=${2:-createescrow}
CONTRACT_ACCOUNT=${3:-createescrow}

echo ${CONTRACT_NAME}
echo ${CONTRACT_DIR}
echo ${CONTRACT_ACCOUNT}

$cleosmainnet set contract ${CONTRACT_ACCOUNT} ${CONTRACT_DIR}/ ${CONTRACT_NAME}.wasm ${CONTRACT_NAME}.abi -p ${CONTRACT_ACCOUNT}@active

$cleosjungle set contract ${CONTRACT_ACCOUNT} ${CONTRACT_DIR}/ ${CONTRACT_NAME}.wasm ${CONTRACT_NAME}.abi -p ${CONTRACT_ACCOUNT}@active
$cleoskylin set contract ${CONTRACT_ACCOUNT} ${CONTRACT_DIR}/ ${CONTRACT_NAME}.wasm ${CONTRACT_NAME}.abi -p ${CONTRACT_ACCOUNT}@active

$cleosorestaging set contract ${CONTRACT_ACCOUNT} ${CONTRACT_DIR}/ ${CONTRACT_NAME}.wasm ${CONTRACT_NAME}.abi -p ${CONTRACT_ACCOUNT}@active
$cleosoremain set contract ${CONTRACT_ACCOUNT} ${CONTRACT_DIR}/ ${CONTRACT_NAME}.wasm ${CONTRACT_NAME}.abi -p ${CONTRACT_ACCOUNT}@active