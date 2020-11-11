# VECTRUM.CONTRACTS

## Version : 0.1.0

The design of the VECTRUM blockchain calls for a number of smart contracts that are run at a privileged permission level in order to support functions such as block producer registration and voting, token staking for CPU and network bandwidth, RAM purchasing, multi-sig, etc.  These smart contracts are referred to as the bios, system, msig, wrap (formerly known as sudo) and token contracts.

This repository contains examples of these privileged contracts that are useful when deploying, managing, and/or using an VECTRUM blockchain.  They are provided for reference purposes:

   * [eosio.bios](./contracts/eosio.bios)
   * [eosio.system](./contracts/eosio.system)
   * [eosio.msig](./contracts/eosio.msig)
   * [eosio.wrap](./contracts/eosio.wrap)

The following unprivileged contract(s) are also part of the system.
   * [eosio.token](./contracts/eosio.token)

Dependencies:
* [VECTRUM.CDT v0.1.x](https://github.com/vectrum-core/vectrum.cdt/releases/tag/v0.1.0)
* [VECTRUM v0.1.x](https://github.com/vectrum-core/vectrum/releases/tag/v0.1.0) (optional dependency only needed to build unit tests)

To build the contracts follow the instructions in [`Build and deploy` section](./docs/02_build-and-deploy.md).

## License
[LICENSE](./LICENSE)
[EOSIO LICENSE](./eosio.contracts.license)
