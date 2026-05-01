# UE5-CrazyWorld-Multiplayer Online Game Framework
项目代号CrazyWorld，虚幻5.7版本多人联机游戏框架，包含多人联机房间系统，技能和技能树系统，物品及背包系统（支持枪械近战魔法），模型切换系统，身份系统，任务系统，时间系统等等，可以在此框架上灵活开发多人联机游戏
代号：CrazyWorld
制作人：
程序：CyberPenguin

玩法设计：
本游戏是一款类似狼人杀，鹅鸭杀，among us，魔法少女的魔女审判，弹丸论破的游戏，逆转裁判的联机派对游戏。
游戏开始时，在所有玩家里面分配一个恶魔身份，其他玩家分配人类身份，并给所有玩家分配三个每日任务和五个主线任务。不同身份拥有不同的技能树，玩家可以自行选择技能学习并使用。
游戏里有时钟系统，其中恶魔每个白天（早上6点开始到晚上18点结束）只能击杀一名其他玩家，恶魔若击杀超过一名的玩家则自身死亡，人类若在白天击杀其他玩家则自身死亡，到了夜晚（18点到第二天早上6点），恶魔可以再次击杀一名玩家，所有人类整体计数可以击杀一名玩家，无强制审判系统，改为玩家自行处决。注意，只能在晚上击杀恶魔，恶魔如果在白天死亡，会随机选择一名其他玩家变成恶魔。其他时间收集证据，做任务，碰到尸体也不能报警。第二天以此类推。
如果有玩家没有做完每日任务（18点时未完成），则在每日结束时扣除该玩家血50点，每日任务每天早上6点刷新。如果所有人类做完了自己的主线任务，则变身圣职，圣职可以随意击杀其他玩家不受惩罚，还拥有圣职专属技能树，并且圣职白天击杀恶魔也不会随机选择下一名玩家变成恶魔。
灵活使用技能，一些技能可以辅助获取证据，比如检查尸体之后可以获取死亡时间，死亡方式。也有检查物品的技能，可以看到物品被谁使用过，或者检测玩家手持过什么物品。还有许多互换模型，禁言，锁门撬门，让死者说话等等各种各样的技能。恶魔和圣职拥有许多独特技能。
胜利条件：当恶魔全死之后人类胜利，当人类全死之后恶魔胜利。
（目前未做具体技能和具体任务，其余框架和玩法均搭建完毕）
游戏操作：
WASD空格移动，k按键打开技能树可以学习技能，b按键打开背包，f按键拾取，p键菜单，e按键做任务。
按住鼠标右键控制模型转向，鼠标左键控制镜头视角。

联机方法：首先打开radmin，让主机和客户端都连接到同一个局域网内。
之后主机进入主界面点击开始游戏然后输入自定义游戏房间名称，其他玩家点击加入游戏然后输入对应的房间名称即可进入。
进入游戏后，走到模型边上即可切换模型，走到蘑菇边上等待一会儿之后进入游戏对局。

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
