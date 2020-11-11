## Goal

Stake resource for your account

## Before you begin

* Install the currently supported version of vectrum-cli

* Ensure the reference system contracts from `vectrum.contracts` repository is deployed and used to manage system resources

* Understand the following:
  * What is an account
  * What is network bandwidth
  * What is CPU bandwidth

## Steps

Stake 0.01 VTM network bandwidth for `alice`

```shell
vectrum-cli system delegatebw alice alice "0 VTM" "0.01 VTM"
```

Stake 0.01 VTM CPU bandwidth for `alice`:

```shell
vectrum-cli system delegatebw alice alice "0.01 VTM" "0 VTM"
```