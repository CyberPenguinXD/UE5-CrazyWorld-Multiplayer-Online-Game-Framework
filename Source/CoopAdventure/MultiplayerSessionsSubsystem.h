// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"    // 游戏实例子系统基类
#include "Interfaces/OnlineSessionInterface.h"   // 在线会话接口
#include "OnlineSessionSettings.h"                 // 在线会话设置
#include "MultiplayerSessionsSubsystem.generated.h"

// 声明动态多播委托：服务器创建结果回调，携带是否成功的布尔值
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FServerCreateDelegate, bool, bWasSuccessful);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FServerJoinDelegate, bool, bWasSuccessful);

/**
 * 多人游戏会话子系统
 * 负责管理游戏会话的创建、查找和销毁
 * 可以在蓝图中通过GameInstance获取此子系统
 */
UCLASS()
class COOPADVENTURE_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()


public:
	// 构造函数
	UMultiplayerSessionsSubsystem();

	// 子系统初始化（重写自UGameInstanceSubsystem）
	void Initialize(FSubsystemCollectionBase& Collection) override;

	// 子系统反初始化（重写自UGameInstanceSubsystem）
	void Deinitialize() override;

	// 在线会话接口指针（用于创建、查找、销毁会话）
	IOnlineSessionPtr SessionInterface;

	/**
	 * 创建游戏服务器（主机）
	 * @param ServerName - 服务器显示名称，将存储在会话设置中供其他玩家查看
	 */
	UFUNCTION(BlueprintCallable)
	void CreateServer(FString ServerName);

	/**
	 * 查找游戏服务器
	 * @param ServerName - 要搜索的服务器名称（当前实现中传入但未实际用于过滤）
	 */
	UFUNCTION(BlueprintCallable)
	void FindServer(FString ServerName);

	/**
	 * 创建会话完成回调（由OnlineSubsystem触发）
	 * @param SessionName - 创建的会话名称
	 * @param bWasSuccessful - 是否创建成功
	 */
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	/**
	 * 销毁会话完成回调（由OnlineSubsystem触发）
	 * @param SessionName - 销毁的会话名称
	 * @param bWasSuccessful - 是否销毁成功
	 */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/**
	 * 查找会话完成回调（由OnlineSubsystem触发）
	 * @param bWasSuccessful - 查找操作是否成功完成
	 */
	void OnFindSessionComplete(bool bWasSuccessful);

	/**
	 * 加入会话完成回调（由OnlineSubsystem触发）
	 * @param bWasSuccessful - 查找操作是否成功完成
	 */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);


	// 标记：是否在销毁旧会话后创建新服务器（处理同名会话冲突）
	bool CreateServerAfterDestroy;

	// 存储待创建的服务器名称（当CreateServerAfterDestroy为true时使用）
	FString DestroyServerName;

	//存储查找的服务器名称
	FString ServerNameToFind;

	// 会话搜索对象（共享指针，用于存储查找结果）
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	//会话名称
	FName MySessionName;


	// 声明一个 Blueprint 可分配的多播委托变量
	// BlueprintAssignable: 允许在蓝图中绑定（Bind）和解绑（Unbind）此委托
	// 用于服务器创建完成后的回调通知
	UPROPERTY(BlueprintAssignable)
	FServerCreateDelegate ServerCreateDel;

	UPROPERTY(BlueprintAssignable)
	FServerJoinDelegate ServerJoinDel;

	FString GetRadminIP() const;


public:
	// 新增成员变量
	FString ServerIPToConnect;

};