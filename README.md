# DLLRavendawn

A simple DLL that, when injected to Ravendawn's process, do a detour to get the player's structure pointer and then make the player always face north

Just a proof of concept

## Compile

`cl /LD DLLRavendawn.cpp User32.lib DbgHelp.lib`

## Inject

Just use the [DLLInjector](https://github.com/barongello/DLLInjector) that can be found in my repositories
