// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/World/FlowNode_OnNotifyFromActor.h"
#include "FlowComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_OnNotifyFromActor)

UFlowNode_OnNotifyFromActor::UFlowNode_OnNotifyFromActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bRetroactive(false)
{
#if WITH_EDITOR
	Category = TEXT("Notifies");
	NodeStyle = EFlowNodeStyle::Condition;
#endif
}

void UFlowNode_OnNotifyFromActor::ObserveActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	if (!RegisteredActors.Contains(Actor))
	{
		RegisteredActors.Emplace(Actor, Component);
		Component->OnNotifyFromComponent.AddUObject(this, &UFlowNode_OnNotifyFromActor::OnNotifyFromComponent);

		if (bRetroactive && Component->GetRecentlySentNotifyTags().HasAnyExact(NotifyTags))
		{
			OnEventReceived();
		}
	}
}

void UFlowNode_OnNotifyFromActor::ForgetActor(TWeakObjectPtr<AActor> Actor, TWeakObjectPtr<UFlowComponent> Component)
{
	Component->OnNotifyFromComponent.RemoveAll(this);
}

void UFlowNode_OnNotifyFromActor::OnNotifyFromComponent(UFlowComponent* Component, const FGameplayTag& Tag)
{
	bool bIdentityMatches = false;

	switch (IdentityMatchType)
	{
	case EFlowTagContainerMatchType::HasAny:
		bIdentityMatches = Component->IdentityTags.HasAny(IdentityTags);
		break;
	case EFlowTagContainerMatchType::HasAnyExact:
		bIdentityMatches = Component->IdentityTags.HasAnyExact(IdentityTags);
		break;
	case EFlowTagContainerMatchType::HasAll:
		bIdentityMatches = Component->IdentityTags.HasAll(IdentityTags);
		break;
	case EFlowTagContainerMatchType::HasAllExact:
		bIdentityMatches = Component->IdentityTags.HasAllExact(IdentityTags);
		break;
	}
	
	if (bIdentityMatches && (!NotifyTags.IsValid() || NotifyTags.HasTagExact(Tag)))
	{
		OnEventReceived();
	}
}

#if WITH_EDITOR
FString UFlowNode_OnNotifyFromActor::GetNodeDescription() const
{
	return GetIdentityTagsDescription(IdentityTags) + LINE_TERMINATOR + GetNotifyTagsDescription(NotifyTags);
}
#endif
