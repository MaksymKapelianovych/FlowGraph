// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Asset/FlowDebuggerSubsystem.h"
#include "Asset/FlowAssetEditor.h"
#include "Asset/FlowMessageLogListing.h"

#include "FlowSubsystem.h"

#include "Editor/UnrealEdEngine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Templates/Function.h"
#include "UnrealEdGlobals.h"
#include "Graph/FlowGraphEditorSettings.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowDebuggerSubsystem)

#define LOCTEXT_NAMESPACE "FlowDebuggerSubsystem"

//////////////////////////////////////////////////////////////////////////
// Flow Debug Trait

bool FFlowDebugTrait::IsEnabled() const
{
	return bEnabled;
}

bool FFlowDebugTrait::IsHit() const
{
	return bHit;
}

//////////////////////////////////////////////////////////////////////////
// UFlowDebuggerSubsystem

UFlowDebuggerSubsystem::UFlowDebuggerSubsystem()
{
	FEditorDelegates::BeginPIE.AddUObject(this, &UFlowDebuggerSubsystem::OnBeginPIE);
	FEditorDelegates::EndPIE.AddUObject(this, &UFlowDebuggerSubsystem::OnEndPIE);

	UFlowSubsystem::OnInstancedTemplateAdded.BindUObject(this, &UFlowDebuggerSubsystem::OnInstancedTemplateAdded);
	UFlowSubsystem::OnInstancedTemplateRemoved.BindUObject(this, &UFlowDebuggerSubsystem::OnInstancedTemplateRemoved);
}

void UFlowDebuggerSubsystem::OnInstancedTemplateAdded(UFlowAsset* FlowAsset)
{
	if (!RuntimeLogs.Contains(FlowAsset))
	{
		RuntimeLogs.Add(FlowAsset, FFlowMessageLogListing::GetLogListing(FlowAsset, EFlowLogType::Runtime));
		FlowAsset->OnRuntimeMessageAdded().AddUObject(this, &UFlowDebuggerSubsystem::OnRuntimeMessageAdded);
	}
}

void UFlowDebuggerSubsystem::OnInstancedTemplateRemoved(UFlowAsset* FlowAsset) const
{
	FlowAsset->OnRuntimeMessageAdded().RemoveAll(this);
}

void UFlowDebuggerSubsystem::OnRuntimeMessageAdded(const UFlowAsset* FlowAsset, const TSharedRef<FTokenizedMessage>& Message) const
{
	const TSharedPtr<class IMessageLogListing> Log = RuntimeLogs.FindRef(FlowAsset);
	if (Log.IsValid())
	{
		Log->AddMessage(Message);
		Log->OnDataChanged().Broadcast();
	}
}

void UFlowDebuggerSubsystem::OnBeginPIE(const bool bIsSimulating)
{
	// clear all logs from a previous session
	RuntimeLogs.Empty();
}

void UFlowDebuggerSubsystem::OnEndPIE(const bool bIsSimulating)
{
	for (const TPair<TWeakObjectPtr<UFlowAsset>, TSharedPtr<class IMessageLogListing>>& Log : RuntimeLogs)
	{
		if (Log.Key.IsValid() && Log.Value->NumMessages(EMessageSeverity::Warning) > 0)
		{
			FNotificationInfo Info{FText::FromString(TEXT("Flow Graph reported in-game issues"))};
			Info.ExpireDuration = 15.0;
			
			Info.HyperlinkText = FText::Format(LOCTEXT("OpenFlowAssetHyperlink", "Open {0}"), FText::FromString(Log.Key->GetName()));
			Info.Hyperlink = FSimpleDelegate::CreateLambda([this, Log]()
			{
				UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
				if (AssetEditorSubsystem->OpenEditorForAsset(Log.Key.Get()))
				{
					AssetEditorSubsystem->FindEditorForAsset(Log.Key.Get(), true)->InvokeTab(FFlowAssetEditor::RuntimeLogTab);
				}
			});

			const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Fail);
			}
		}
	}
}

void ForEachGameWorld(const TFunction<void(UWorld*)>& Func)
{
	for (const FWorldContext& PieContext : GUnrealEd->GetWorldContexts())
	{
		UWorld* PlayWorld = PieContext.World();
		if (PlayWorld && PlayWorld->IsGameWorld())
		{
			Func(PlayWorld);
		}
	}
}

bool AreAllGameWorldPaused()
{
	bool bPaused = true;
	ForEachGameWorld([&](const UWorld* World)
	{
		bPaused = bPaused && World->bDebugPauseExecution;
	});
	return bPaused;
}

void UFlowDebuggerSubsystem::PausePlaySession()
{
	bool bPaused = false;
	ForEachGameWorld([&](UWorld* World)
	{
		if (!World->bDebugPauseExecution)
		{
			World->bDebugPauseExecution = true;
			bPaused = true;
		}
	});
	if (bPaused)
	{
		GUnrealEd->PlaySessionPaused();
	}
}

bool UFlowDebuggerSubsystem::IsPlaySessionPaused()
{
	return AreAllGameWorldPaused();
}

void UFlowDebuggerSubsystem::CreateTrait(const UFlowGraphNode* OwnerNode, EFlowTraitType Type, bool bEnabled)
{
	UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
	check(Settings);
	FFlowTraitSettings& TraitSettings = Settings->PerNodeTraits.FindOrAdd(OwnerNode->NodeGuid);

	// ensure that this node doesn't already contain a trait with provided type
	checkSlow(!TraitSettings.NodeTraits.ContainsByPredicate([Type](const FFlowDebugTrait& Trait)
	{
		return Trait.Type == Type;
	}));

	TraitSettings.NodeTraits.Emplace(Type, bEnabled);
	SaveFlowGraphEditorSettings();
}

void UFlowDebuggerSubsystem::CreateTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bEnabled)
{
	UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
	check(Settings);
	FFlowTraitSettings& TraitSettings = Settings->PerNodeTraits.FindOrAdd(OwnerPin->GetOwningNode()->NodeGuid);

	// ensure that this pin doesn't already contain a trait with provided type
	checkSlow(!TraitSettings.PinTraits.ContainsByPredicate([OwnerPin, Type](const FFlowDebugTrait& Trait)
	{
		return Trait.PinId == OwnerPin->PinId && Trait.Type == Type;
	}));

	TraitSettings.PinTraits.Emplace(Type, OwnerPin->PinId, bEnabled);
	SaveFlowGraphEditorSettings();
}

void UFlowDebuggerSubsystem::RemoveTrait(const UFlowGraphNode* OwnerNode, EFlowTraitType Type)
{
	RemoveNodeTraitByPredicate(OwnerNode, [Type](const FFlowDebugTrait& Trait)
		{
			return Trait.Type == Type;
		});
}

void UFlowDebuggerSubsystem::RemoveTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	RemovePinTraitByPredicate(OwnerPin, [OwnerPin, Type](const FFlowDebugTrait& Trait)
		{
			return Trait.PinId == OwnerPin->PinId && Trait.Type == Type;
		});
}

void UFlowDebuggerSubsystem::RemoveNodeTraitByPredicate(const UFlowGraphNode* OwnerNode,
	const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate)
{
	if(TArray<FFlowDebugTrait>* Traits = GetNodeTraits(OwnerNode))
	{
		if (Traits->RemoveAllSwap(Predicate, EAllowShrinking::No))
		{
			if(Traits->IsEmpty())
			{
				ClearNodeTraits(OwnerNode);
			}
			SaveFlowGraphEditorSettings();
		}
	}
}

void UFlowDebuggerSubsystem::RemovePinTraitByPredicate(const UFlowGraphNode* OwnerNode,
	const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate)
{
	if(TArray<FFlowDebugTrait>* Traits = GetPinTraits(OwnerNode))
	{
		if (Traits->RemoveAllSwap(Predicate, EAllowShrinking::No))
		{
			if(Traits->IsEmpty())
			{
				ClearPinTraits(OwnerNode);
			}
			SaveFlowGraphEditorSettings();
		}
	}
}

void UFlowDebuggerSubsystem::RemovePinTraitByPredicate(const UEdGraphPin* OwnerPin,
                                                       const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate)
{
	const UFlowGraphNode* OwnerNode = Cast<UFlowGraphNode>(OwnerPin->GetOwningNode());
	check(OwnerNode);
	
	if(TArray<FFlowDebugTrait>* Traits = GetPinTraits(OwnerNode))
	{
		if (Traits->RemoveAllSwap(Predicate, EAllowShrinking::No))
		{
			if(Traits->IsEmpty())
			{
				ClearPinTraits(OwnerPin);
			}
			SaveFlowGraphEditorSettings();
		}
	}
}

void UFlowDebuggerSubsystem::ClearNodeTraits(const UFlowGraphNode* OwnerNode)
{
	if (FFlowTraitSettings* TraitSettings = GetPerNodeSettings(OwnerNode))
	{
		TraitSettings->NodeTraits.Empty();

		// if all settings data is default, we can remove it from the map
		if(*TraitSettings == FFlowTraitSettings())
		{
			UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
			Settings->PerNodeTraits.Remove(OwnerNode->NodeGuid);
		}

		SaveFlowGraphEditorSettings();
	}
}

void UFlowDebuggerSubsystem::ClearPinTraits(const UFlowGraphNode* OwnerNode)
{
	if (FFlowTraitSettings* TraitSettings = GetPerNodeSettings(OwnerNode))
	{
		TraitSettings->PinTraits.Empty();
		
		// if all settings data is default, we can remove it from the map
		if(*TraitSettings == FFlowTraitSettings())
		{
			UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
			Settings->PerNodeTraits.Remove(OwnerNode->NodeGuid);
		}

		SaveFlowGraphEditorSettings();
	}
}

void UFlowDebuggerSubsystem::ClearPinTraits(const UEdGraphPin* OwnerPin)
{
	UFlowGraphNode* OwnerNode = Cast<UFlowGraphNode>(OwnerPin->GetOwningNode());
	check(OwnerNode);
	
	if (FFlowTraitSettings* TraitSettings = GetPerNodeSettings(OwnerNode))
	{
		TraitSettings->PinTraits.RemoveAllSwap([OwnerPin](const FFlowDebugTrait& Trait)
		{
			return Trait.PinId == OwnerPin->PinId;
		});
		
		// if all settings data is default, we can remove it from the map
		if(*TraitSettings == FFlowTraitSettings())
		{
			UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
			Settings->PerNodeTraits.Remove(OwnerNode->NodeGuid);
		}

		SaveFlowGraphEditorSettings();
	}
}

void UFlowDebuggerSubsystem::CleanupTraits(const UFlowGraphNode* OwnerNode)
{
	RemovePinTraitByPredicate(OwnerNode, [OwnerNode](const FFlowDebugTrait& Trait)
	{
		auto Predicate = [Trait](const UEdGraphPin* Pin)
		{
			return Pin->PinId == Trait.PinId;
		};
		return OwnerNode->Pins.ContainsByPredicate(Predicate) == false;
	});
}

FFlowDebugTrait* UFlowDebuggerSubsystem::FindTrait(const UFlowGraphNode* OwnerNode, EFlowTraitType Type)
{
	if(TArray<FFlowDebugTrait>* Traits = GetNodeTraits(OwnerNode))
	{
		for(FFlowDebugTrait& Trait  : *Traits)
		{
			if(Trait.Type == Type)
			{
				return &Trait;
			}
		}
	}
	
	return nullptr;
}

FFlowDebugTrait* UFlowDebuggerSubsystem::FindTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	const UFlowGraphNode* OwnerNode = Cast<UFlowGraphNode>(OwnerPin->GetOwningNode());
	check(OwnerNode);

	if(TArray<FFlowDebugTrait>* Traits = GetPinTraits(OwnerNode))
	{
		for(FFlowDebugTrait& Trait  : *Traits)
		{
			if(Trait.PinId == OwnerPin->PinId && Trait.Type == Type)
			{
				return &Trait;
			}
		}
	}
	
	return nullptr;
}

void UFlowDebuggerSubsystem::SetTraitEnabled(const UFlowGraphNode* OwnerNode, EFlowTraitType Type, bool bIsEnabled)
{
	if (FFlowDebugTrait* Trait = FindTrait(OwnerNode, Type))
	{
		Trait->bEnabled = bIsEnabled;
		SaveFlowGraphEditorSettings();
	}
}

void UFlowDebuggerSubsystem::SetTraitEnabled(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bIsEnabled)
{
	if (FFlowDebugTrait* Trait = FindTrait(OwnerPin, Type))
	{
		Trait->bEnabled = bIsEnabled;
		SaveFlowGraphEditorSettings();
	}
}

bool UFlowDebuggerSubsystem::IsTraitEnabled(const UFlowGraphNode* OwnerNode, EFlowTraitType Type)
{
	if (const FFlowDebugTrait* Trait = FindTrait(OwnerNode, Type))
	{
		return Trait->IsEnabled();
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsTraitEnabled(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	if (const FFlowDebugTrait* Trait = FindTrait(OwnerPin, Type))
	{
		return Trait->IsEnabled();
	}

	return false;
}

void UFlowDebuggerSubsystem::ToggleTrait(const UFlowGraphNode* OwnerNode, EFlowTraitType Type)
{
	if (FindTrait(OwnerNode, Type))
	{
		RemoveTrait(OwnerNode, Type);
	}
	else
	{
		CreateTrait(OwnerNode, Type, true);
	}
}

void UFlowDebuggerSubsystem::ToggleTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	if (FindTrait(OwnerPin, Type))
	{
		RemoveTrait(OwnerPin, Type);
	}
	else
	{
		CreateTrait(OwnerPin, Type, true);
	}
}

TArray<EFlowTraitType> UFlowDebuggerSubsystem::SetAllTraitsHit(const UFlowGraphNode* OwnerNode, bool bHit)
{
	TArray<EFlowTraitType> HitTraitTypes;
	for (EFlowTraitType Type : TEnumRange<EFlowTraitType>())
	{
		if (SetTraitHit(OwnerNode, Type, bHit))
		{
			HitTraitTypes.Add(Type);
		}
	}

	return HitTraitTypes;
}

TArray<EFlowTraitType> UFlowDebuggerSubsystem::SetAllTraitsHit(const UEdGraphPin* OwnerPin, bool bHit)
{
	TArray<EFlowTraitType> HitTraitTypes;
	for (EFlowTraitType Type : TEnumRange<EFlowTraitType>())
	{
		if (SetTraitHit(OwnerPin, Type, bHit))
		{
			HitTraitTypes.Add(Type);
		}
	}

	return HitTraitTypes;
}


bool UFlowDebuggerSubsystem::SetTraitHit(const UFlowGraphNode* OwnerNode, EFlowTraitType Type, bool bHit)
{
	if (FFlowDebugTrait* Trait = FindTrait(OwnerNode, Type))
	{
		Trait->bHit = bHit;
		return true;
	}

	return false;
}

bool UFlowDebuggerSubsystem::SetTraitHit(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bHit)
{
	if (FFlowDebugTrait* Trait = FindTrait(OwnerPin, Type))
	{
		Trait->bHit = bHit;
		return true;
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsTraitHit(const UFlowGraphNode* OwnerNode, EFlowTraitType Type)
{
	if (const FFlowDebugTrait* Trait = FindTrait(OwnerNode, Type))
	{
		return Trait->IsHit();
	}

	return false;
}

bool UFlowDebuggerSubsystem::IsTraitHit(const UEdGraphPin* OwnerPin, EFlowTraitType Type)
{
	if (const FFlowDebugTrait* Trait = FindTrait(OwnerPin, Type))
	{
		return Trait->IsHit();
	}

	return false;
}

FFlowTraitSettings* UFlowDebuggerSubsystem::GetPerNodeSettings(const UFlowGraphNode* OwnerNode)
{
	UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
	check(Settings);
	return Settings->PerNodeTraits.Find(OwnerNode->NodeGuid);
}

TArray<FFlowDebugTrait>* UFlowDebuggerSubsystem::GetNodeTraits(const UFlowGraphNode* OwnerNode)
{
	FFlowTraitSettings* Settings = GetPerNodeSettings(OwnerNode);

	// return nullptr if there's no node traits associated w/ this flow node
	return (!Settings || Settings->NodeTraits.IsEmpty()) ?
		nullptr :
		&Settings->NodeTraits;
}

TArray<FFlowDebugTrait>* UFlowDebuggerSubsystem::GetPinTraits(const UFlowGraphNode* OwnerNode)
{
	FFlowTraitSettings* Settings = GetPerNodeSettings(OwnerNode);

	// return nullptr if there's no pin traits associated w/ this flow node
	return (!Settings || Settings->PinTraits.IsEmpty()) ?
		nullptr :
		&Settings->PinTraits;
}

void UFlowDebuggerSubsystem::SaveFlowGraphEditorSettings()
{
	UFlowGraphEditorSettings* Settings = GetMutableDefault<UFlowGraphEditorSettings>();
	check(Settings);
	Settings->SaveConfig();
}

#undef LOCTEXT_NAMESPACE
