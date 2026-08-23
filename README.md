# UE5-CrazyWorld-Multiplayer Online Game Framework

# UE5 CrazyWorld 多人联机游戏框架

**Project Codename / 项目代号：** CrazyWorld
**Engine Version / 引擎版本：** Unreal Engine 5.7
**Producer / 制作人：** CyberPenguinXD

## Project Overview / 项目简介

CrazyWorld is a multiplayer online game framework developed with Unreal Engine 5.7. The framework includes a multiplayer room system, skill and skill tree system, item and inventory system, support for firearms, melee weapons and magic, character model switching system, identity system, quest system, time system, and other multiplayer gameplay modules. These systems can be flexibly extended to develop different types of multiplayer online games.

CrazyWorld 是一个基于 **Unreal Engine 5.7** 开发的多人联机游戏框架。框架包含多人联机房间系统、技能与技能树系统、物品及背包系统，并支持枪械、近战和魔法，同时还包含模型切换系统、身份系统、任务系统、时间系统等多人游戏模块。基于C++和蓝图共同实现


Users can flexibly develop other types of games based on this framework, including multiplayer social party games, shooting games, RPGs, and other multiplayer game projects. Jointly implemented based on C++and blueprint

使用者可以在此框架基础上灵活开发其他类型的游戏，例如各种多人社交小游戏、枪战游戏、RPG，以及其他多人联机游戏项目。



## Gameplay Design / 玩法设计

At the beginning of each match, one player is randomly assigned the **Demon** role, while all other players become **Humans**. Each player receives three Daily Quests and five Main Quests. Different identities have different skill trees, and players can choose and learn skills according to their role and gameplay strategy.

游戏开始时，系统会从所有玩家中随机分配一名玩家成为 **恶魔**，其他玩家则获得 **人类** 身份。每名玩家会获得三个每日任务和五个主线任务。不同身份拥有不同的技能树，玩家可以根据自己的身份和游戏策略选择学习并使用不同技能。

The game contains a complete day and night system. Daytime lasts from **06:00 to 18:00**. During the day, the Demon can kill only one player. If the Demon kills more than one player, the Demon will die. Human players are not allowed to kill others during daytime, otherwise they will also die. If the Demon is killed during the day, another surviving player will be randomly selected to become the new Demon.

游戏中设有完整的昼夜系统。白天时间为 **06:00 至 18:00**。白天期间，恶魔只能击杀一名其他玩家，如果击杀超过一人则恶魔自身死亡。人类在白天击杀其他玩家时同样会受到死亡惩罚。如果恶魔在白天死亡，系统会随机选择另一名存活玩家成为新的恶魔。

Nighttime lasts from **18:00 to 06:00 the next morning**. During the night, the Demon can kill one additional player, while all Human players share one opportunity to kill a player. The game does not use a mandatory voting or trial system. Players instead investigate, collect evidence, complete tasks, and decide by themselves whether another player should be executed. Finding a dead body does not automatically trigger a meeting or report.

夜晚时间为 **18:00 至次日 06:00**。夜晚期间，恶魔可以再次击杀一名玩家，而所有人类整体共享一次击杀机会。游戏不设置强制投票或审判系统，玩家需要自行调查、收集证据、完成任务，并决定是否处决其他玩家。发现尸体后也不会自动触发报警或会议。

Daily Quests are refreshed every morning at **06:00**. If a player has not completed all Daily Quests by **18:00**, the player will lose **50 HP** at the end of the day. Main Quests are connected to an additional role transformation system. If all Human players complete their Main Quests, they become **Clerics**. Clerics can attack other players without the normal punishment, gain access to an exclusive skill tree, and can permanently kill the Demon during daytime without causing another player to become the new Demon.

每日任务会在每天早上 **06:00** 刷新。如果玩家在 **18:00** 前没有完成所有每日任务，则在当天结束时扣除 **50 点生命值**。主线任务则与特殊身份转换机制相关。如果所有人类玩家都完成了自己的主线任务，则会转变为 **圣职**。圣职可以在不受到普通惩罚的情况下攻击其他玩家，同时拥有专属技能树，并且可以在白天真正击杀恶魔，而不会触发新的恶魔身份转移。

Skills are an important part of the gameplay. Some skills are designed for investigation and evidence collection, such as checking a corpse to obtain the time or cause of death, inspecting an item to determine who has used it, or investigating which items a player has previously held. Other skills can affect multiplayer interaction and map control, including model swapping, muting players, locking or unlocking doors, and allowing dead players to communicate. Demon and Cleric identities also have their own special abilities and skill trees.

技能系统是游戏玩法的重要组成部分。部分技能用于调查和证据收集，例如检查尸体获得死亡时间和死亡方式，检查物品以确认曾经被哪些玩家使用，或者调查某名玩家曾经持有过哪些物品。其他技能则可以影响玩家互动和地图控制，例如交换模型、禁言玩家、锁门或撬门，以及让死亡玩家重新进行交流等。恶魔和圣职身份也拥有各自独特的能力和专属技能树。

The victory condition is straightforward: Humans win when all Demons are eliminated, while the Demon side wins when all Humans are eliminated. At present, the overall gameplay framework, multiplayer systems, identity logic, quest framework, skill framework, inventory system, time system, and major game rules have been completed. Specific skills and detailed quest content are still under development.

游戏的胜利条件为：当所有恶魔全部死亡时，人类阵营获胜；当所有人类全部死亡时，恶魔阵营获胜。目前整体玩法框架、多人联机系统、身份逻辑、任务框架、技能框架、背包系统、时间系统以及主要游戏规则已经搭建完成，具体技能内容和具体任务仍在继续开发。

## Controls / 游戏操作

The basic controls are **WASD** for movement and **Space** for jumping. Press **K** to open the skill tree, **B** to open the inventory, **F** to pick up items, **P** to open the menu, and **E** to interact with or complete tasks. Holding the right mouse button controls character rotation, while the left mouse button is used to control the camera view.

基础操作为使用 **WASD** 控制移动，使用 **Space** 跳跃。按 **K** 打开技能树，按 **B** 打开背包，按 **F** 拾取物品，按 **P** 打开菜单，按 **E** 进行交互或执行任务。按住鼠标右键可以控制角色模型转向，鼠标左键用于控制镜头视角。

## Multiplayer Setup / 联机方法

The current multiplayer testing method uses **Radmin VPN**. The host and all clients must first connect to the same virtual LAN. The host then enters the main menu, selects **Start Game**, and creates a custom room name. Other players select **Join Game** and enter the same room name to join the multiplayer lobby.

目前联机测试使用 **Radmin VPN**。主机与所有客户端首先需要连接到同一个虚拟局域网。之后主机进入游戏主界面，点击 **Start Game** 并输入自定义房间名称。其他玩家点击 **Join Game**，输入相同的房间名称后即可进入多人联机大厅。

After entering the lobby, players can walk next to a character model to switch their current model. When all players are ready, they can move near the mushroom and wait for a short period to enter the actual match.

进入大厅后，玩家可以走到角色模型附近切换当前使用的模型。准备完成后，玩家走到蘑菇附近并等待一段时间，即可进入正式游戏对局。

## Development Status / 开发状态

CrazyWorld is currently a functional multiplayer gameplay framework rather than a fully completed game. Most of the major systems and gameplay logic have already been implemented, while specific skills, quest content, game balance, additional maps, characters, items, and visual improvements are planned for further development.

CrazyWorld 当前主要是一个已经具备完整核心逻辑的多人联机游戏框架，而并非最终完成版本。大部分主要系统和玩法逻辑已经实现，后续将继续完善具体技能、任务内容、游戏平衡、地图、角色、物品以及视觉表现。

## Developer / 开发者

**CyberPenguinXD**

The project is too large and requires source files. Please contact me
项目过大，需要源文件请联系我


<img width="1920" height="1040" alt="f08eb63b5b70d6ae97320f80994c4181" src="https://github.com/user-attachments/assets/d8d3e395-d9dd-4556-91c1-538a817076a3" />
<img width="1920" height="1040" alt="e2335cd6358352cd4e2e4928ccbba5f3" src="https://github.com/user-attachments/assets/3e3dd1ea-a507-4945-92a0-8bd8089d1057" />
<img width="1920" height="1040" alt="d2074d68695b2315d8a7fa4a3b87b46c" src="https://github.com/user-attachments/assets/5e8a7257-7417-4adb-be42-0de01ed37e9b" />
<img width="1920" height="1040" alt="ca41cc02dfc89c4ff99b22fd828b6ee5" src="https://github.com/user-attachments/assets/fe5c0553-6e83-4823-badb-d34bea761a5c" />
<img width="1920" height="1040" alt="c206ac8f89d31788d332780be3f43179" src="https://github.com/user-attachments/assets/2d20656c-3343-4c0f-b521-8000e51ffa1d" />
<img width="1920" height="1040" alt="77943f4f15999ac647df52362e98a207" src="https://github.com/user-attachments/assets/be6060dd-c712-43be-b50b-9d303cda7487" />
<img width="1920" height="1040" alt="76097f83dccdd6097c3109d18790b273" src="https://github.com/user-attachments/assets/fc2fbe71-eb17-4c39-aab1-3910e130e5ac" />
<img width="1920" height="1040" alt="69510f57e16b2f21761d45040da536e7" src="https://github.com/user-attachments/assets/b1cb5e44-617b-40f6-a1ce-57f3afafb022" />
<img width="1920" height="1040" alt="5212c0ad5ebce6500dae3b11d81b13e1" src="https://github.com/user-attachments/assets/2415a149-850f-4f23-82d1-aadf827f33d5" />
<img width="1920" height="1040" alt="48e92a8be0ee3128829543cecff91fb8" src="https://github.com/user-attachments/assets/928a755e-4385-42ad-b78b-349d995bd982" />



