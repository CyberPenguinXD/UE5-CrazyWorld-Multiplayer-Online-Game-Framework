// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "SocketSubsystem.h"          // 用于获取本地 IP
#include "Interfaces/OnlineSessionInterface.h"

#define SEARCH_PRESENCE         FName(TEXT("PRESENCESEARCH"))

// 调试打印函数：在屏幕上显示调试信息
void PrintString(const FString& Str)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, Str);
	}
}

// 构造函数：初始化成员变量
UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
{
	CreateServerAfterDestroy = false;
	DestroyServerName = "";
	ServerNameToFind = "";
	MySessionName = FName("Co-op Adventure Session Name");
}

// 子系统初始化：获取在线子系统并设置会话接口回调
void UMultiplayerSessionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		FString SubsystemName = OnlineSubsystem->GetSubsystemName().ToString();
		PrintString(SubsystemName);

		SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			PrintString("Session interface is valid!");

			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UMultiplayerSessionsSubsystem::OnCreateSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UMultiplayerSessionsSubsystem::OnDestroySessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UMultiplayerSessionsSubsystem::OnFindSessionComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UMultiplayerSessionsSubsystem::OnJoinSessionComplete);
		}
	}
}

void UMultiplayerSessionsSubsystem::Deinitialize()
{
}

// ========== 辅助函数：获取 Radmin VPN 虚拟 IP ==========
FString UMultiplayerSessionsSubsystem::GetRadminIP() const
{
	TArray<TSharedPtr<FInternetAddr>> Addresses;
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalAdapterAddresses(Addresses);
	for (const auto& Addr : Addresses)
	{
		FString IP = Addr->ToString(false);
		// Radmin VPN 的 IP 通常以 26 或 5 开头
		if (IP.StartsWith(TEXT("26.")) || IP.StartsWith(TEXT("5.")))
		{
			return IP;
		}
	}
	return FString(); // 未找到则返回空
}

// 创建游戏服务器（主机）
void UMultiplayerSessionsSubsystem::CreateServer(FString ServerName)
{
	PrintString("CreateServer");
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (ServerName.IsEmpty())
	{
		PrintString("Server name cannot be empty");
		ServerCreateDel.Broadcast(false);
		return;
	}

	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(MySessionName);
	if (ExistingSession)
	{
		FString Msg = FString::Printf(TEXT("Session with name %s already exists, destroying it."), *MySessionName.ToString());
		PrintString(Msg);
		CreateServerAfterDestroy = true;
		DestroyServerName = ServerName;
		SessionInterface->DestroySession(MySessionName);
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.NumPublicConnections = 3;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bAllowJoinViaPresence = true;

	bool bIsLAN = false;
	if (OnlineSubsystem->GetSubsystemName() == "NULL")
	{
		bIsLAN = true;
	}
	SessionSettings.bIsLANMatch = bIsLAN;

	// 存储服务器名称（用于搜索）
	SessionSettings.Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// ===== 新增：存储 Radmin VPN 虚拟 IP =====
	FString RadminIP = GetRadminIP();
	if (!RadminIP.IsEmpty())
	{
		SessionSettings.Set(FName("SERVER_IP"), RadminIP, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		PrintString(FString::Printf(TEXT("Radmin IP detected and stored: %s"), *RadminIP));
	}
	else
	{
		PrintString("Warning: No Radmin IP found, server may not be reachable via VPN.");
	}
	// =========================================

	SessionInterface->CreateSession(0, MySessionName, SessionSettings);
}

// 查找游戏服务器
void UMultiplayerSessionsSubsystem::FindServer(FString ServerName)
{
	PrintString("FindServer");

	if (ServerName.IsEmpty())
	{
		PrintString("Server name can not be empty!");
		ServerJoinDel.Broadcast(false);
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	bool IsLAN = false;
	if (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
	{
		IsLAN = true;
	}

	SessionSearch->bIsLanQuery = IsLAN;
	SessionSearch->MaxSearchResults = 9999;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	ServerNameToFind = ServerName;
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

// 创建会话完成回调
void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	PrintString(FString::Printf(TEXT("OnCreateSessionComplete: %d"), bWasSuccessful));
	ServerCreateDel.Broadcast(bWasSuccessful);

	if (bWasSuccessful)
	{
		GetWorld()->ServerTravel("/Game/ThirdPerson/Lvl_ThirdPerson?listen");
	}
}

// 销毁会话完成回调
void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	FString Msg = FString::Printf(TEXT("OnDestroySessionComplete, SessionName:%s, Success: %d"), *SessionName.ToString(), bWasSuccessful);
	PrintString(Msg);

	if (CreateServerAfterDestroy)
	{
		CreateServerAfterDestroy = false;
		CreateServer(DestroyServerName);
	}
}

// 查找会话完成回调
void UMultiplayerSessionsSubsystem::OnFindSessionComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful) return;
	if (ServerNameToFind.IsEmpty()) return;

	TArray<FOnlineSessionSearchResult> Results = SessionSearch->SearchResults;
	FOnlineSessionSearchResult* CorrectResult = nullptr;

	if (Results.Num() > 0)
	{
		FString Msg = FString::Printf(TEXT("%d sessions found."), Results.Num());
		PrintString(Msg);
		for (FOnlineSessionSearchResult& Result : Results)
		{
			if (Result.IsValid())
			{
				FString ServerName = "No-name";
				Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);

				if (ServerName.Equals(ServerNameToFind))
				{
					CorrectResult = &Result;
					FString Msg2 = FString::Printf(TEXT("Found server name: %s"), *ServerName);
					PrintString(Msg2);

					// ===== 新增：读取 Radmin IP 并保存，供后续加入使用 =====
					FString ServerIP;
					Result.Session.SessionSettings.Get(FName("SERVER_IP"), ServerIP);
					ServerIPToConnect = ServerIP;
					// =====================================================
					break;
				}
			}
		}

		if (CorrectResult)
		{
			SessionInterface->JoinSession(0, MySessionName, *CorrectResult);
		}
		else
		{
			FString Msg3 = FString::Printf(TEXT("ServerName %s not found"), *ServerNameToFind);
			PrintString(Msg3);
			ServerNameToFind = "";
			ServerJoinDel.Broadcast(false);
		}
	}
	else
	{
		PrintString("Zero sessions found.");
		ServerJoinDel.Broadcast(false);
	}
}

// 加入会话完成回调
void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	ServerJoinDel.Broadcast(Result == EOnJoinSessionCompleteResult::Success);

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString Address;

		// ===== 优先使用之前保存的 Radmin IP =====
		if (!ServerIPToConnect.IsEmpty())
		{
			// 使用 Radmin IP + 默认端口 7777（如果非默认需调整）
			Address = ServerIPToConnect + TEXT(":7777");
			PrintString(FString::Printf(TEXT("Using Radmin IP: %s"), *Address));
		}
		else
		{
			// 回退到引擎解析（例如局域网 IP）
			bool Success = SessionInterface->GetResolvedConnectString(MySessionName, Address);
			if (!Success)
			{
				PrintString("GetResolvedConnectString returned false!");
				return;
			}
		}
		// =========================================

		PrintString(FString::Printf(TEXT("Address: %s"), *Address));

		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
		if (PlayerController)
		{
			PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
		}
	}
	else
	{
		PrintString("OnJoinSessionCompleteResult failed");
	}
}