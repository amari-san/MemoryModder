## Compatibility

\* : Anything  
:green_circle: : Supported  
:yellow_circle: : Untested  
:red_circle: : Unsupported

##

<details closed>

<summary>System Compatibility</summary>

|                 | System    | OS            | Details                                                                        | Tested by: |
| :---:           | :---      | :---          | :---                                                                           | :---       |
| :green_circle:  | x64       | Windows       |                                                                                |            |
| :yellow_circle: | * but x64 | Windows       |                                                                                |            |
| :red_circle:    | *         | * but Windows | This library uses the windows API. For now, this is only supported on windows. |            |

</details>

##

<details closed>

<summary>Game Compatibility</summary>

|                               | Game             | Details                                                                                                                                                                                    | Tested by: |
| :---:                         | :---             | :---                                                                                                                                                                                       | :---       |
| :red_circle:                  | * with anticheat | Anticheats do what they're supposed to do, to prevent you from cheating. I probably need to look into DLL injection to figure out how to bypass anticheat, but that's something for later. |            |
| :green_circle::yellow_circle: | * client related | Untested, but presumably in theory, should work with most games. Servers are unreachable in memory because it's not something in your system.                                              |            |
| :green_circle:                | UNDERTALE        | HP works.                                                                                                                                                                                  |            |
| :green_circle:                | Factorio         | Singleplayer, and multiplayer if client is the host, however if item count is directly written without doing whatever is required, it will cause other players to desync.                  |            |
| :green_circle:                | Raft             |                                                                                                                                                                                            |            |
| :green_circle:                | Space Engineers  | Difficult because memory is constantly moving in managed C#, but definitely possible.                                                                                                      |            |
| :green_circle:                | Astroneer        | mmm 999999 bytes                                                                                                                                                                           |            |

</details>

##

Everyone is free to report concluded tests of a system or a game, in the issues tab.
