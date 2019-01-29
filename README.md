# metahash-erc20-exchange

This repository contains C++ converter source code that allows to convert MHC to ERC20 and back. 

## Kinds of conversion:

1. MHC to ERC20. 
This means transferring from MHC wallet to ETH wallet. As a result, ETH wallet will be replenished with ERC20 tokens. In this, funds will be transferred from user’s MHC wallet to the system’s general MHC, ETH wallet is specified as a parameter in the Data field. 

2. ERC20 to MHC.
Back conversion means that there will be two transactions performed: 
MHC are transferred to the general MHC wallet, ETH with ERC20 must be specified in the Data field. 
ERC20 tokens are transferred from the ETH wallet to the system’s general ETH wallet. 

## Build and run 

1. Clone repository:  
```shell
git clone https://github.com/metahashorg/metahash-erc20-exchange
```
2. Build `meta_erc_convert` binary file from source codes. 
3. Copy converter config: `meta_erc_convert.conf` and add it to the binary file.
4. Run converter as binary file without params: 
```shell
./meta_erc_convert
```

