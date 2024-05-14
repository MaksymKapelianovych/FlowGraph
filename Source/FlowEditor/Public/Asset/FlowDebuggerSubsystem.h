// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EditorSubsystem.h"
#include "Logging/TokenizedMessage.h"
#include "FlowDebuggerSubsystem.generated.h"

class UFlowGraphNode;
class UEdGraphPin;
struct FFlowTraitSettings;
class UFlowAsset;
class FFlowMessageLog;

UENUM()
enum class EFlowTraitType
{
	Breakpoint, // default trait type

	// ^ Add new trait types above here ^
	Max
};

ENUM_RANGE_BY_COUNT(EFlowTraitType, EFlowTraitType::Max)

// It can represent any trait added on the specific node instance, i.e. breakpoint
USTRUCT()
struct FLOWEDITOR_API FFlowDebugTrait
{
	GENERATED_USTRUCT_BODY()

protected:	
	/** Pin that the trait is placed on. Zero filled if this trait is for a node, not a pin */
	UPROPERTY()
	FGuid PinId;

	UPROPERTY()
	EFlowTraitType Type;

	UPROPERTY()
	uint8 bEnabled : 1;
	uint8 bHit : 1;

public:
	FFlowDebugTrait()
		: Type(EFlowTraitType::Breakpoint) // default trait type
		, bEnabled(false)
		, bHit(false)
	{
	};

	explicit FFlowDebugTrait(EFlowTraitType InType, const bool bInitialState)
	: Type(InType)
	, bEnabled(bInitialState)
	, bHit(false)
	{
	};
	
	explicit FFlowDebugTrait(EFlowTraitType InType, FGuid InPinId, const bool bInitialState)
	: PinId(InPinId)
	, Type(InType)
	, bEnabled(bInitialState)
	, bHit(false)
	{
	};
	
	bool IsEnabled() const;
	bool IsHit() const;
	
	bool operator==(const FFlowDebugTrait& Other) const
	{
		return Type == Other.Type
			&& PinId == Other.PinId;
	}

	friend class UFlowDebuggerSubsystem;
};


/**
** Persistent subsystem supporting Flow Graph debugging
 */
UCLASS()
class FLOWEDITOR_API UFlowDebuggerSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()
	
public:
	UFlowDebuggerSubsystem();

protected:	
	TMap<TWeakObjectPtr<UFlowAsset>, TSharedPtr<class IMessageLogListing>> RuntimeLogs;

	void OnInstancedTemplateAdded(UFlowAsset* FlowAsset);
	void OnInstancedTemplateRemoved(UFlowAsset* FlowAsset) const;
	
	void OnRuntimeMessageAdded(UFlowAsset* FlowAsset, const TSharedRef<FTokenizedMessage>& Message) const;
	
	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);

public:	
	static void PausePlaySession();
	static bool IsPlaySessionPaused();

	// Flow Traits Functions

	/** Adds trait with provided Type to OwnerNode. Node cannot accept traits of the same type */
	static void CreateTrait(const UFlowGraphNode* OwnerNode, EFlowTraitType Type, bool bEnabled);
	/** Adds trait with provided Type to OwnerPin. Pin cannot accept traits of the same type */
	static void CreateTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bEnabled);

	/** Removes trait with provided Type from OwnerNode */
	static void RemoveTrait(const UFlowGraphNode* OwnerNode, EFlowTraitType Type);
	/** Removes trait with provided Type from OwnerPin */
	static void RemoveTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	/** Removes any OwnerNode's trait that matches the provided Predicate */
	static void RemoveNodeTraitByPredicate(const UFlowGraphNode* OwnerNode, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate);
	/** Removes any pin trait of provided OwnerNode that matches the provided Predicate */
	static void RemovePinTraitByPredicate(const UFlowGraphNode* OwnerNode, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate);
	/** Removes any OwnerPin's trait that matches the provided Predicate */
	static void RemovePinTraitByPredicate(const UEdGraphPin* OwnerPin, const TFunctionRef<bool(const FFlowDebugTrait&)> Predicate);
	
	/** Clears all node traits for provided OwnerNode */
	static void ClearNodeTraits(const UFlowGraphNode* OwnerNode);
	/** Clears all pin traits for provided OwnerNode */
	static void ClearPinTraits(const UFlowGraphNode* OwnerNode);
	/** Clears all traits for provided OwnerPin */
	static void ClearPinTraits(const UEdGraphPin* OwnerPin);
	/** Removes stale pin traits for provided OwnerNode. Pin list can be changed after node reconstructing and traits for removed pins can stay in PerNodeSettings.
	 * There is no need to clean node's traits here, cause all node events can be processed right away and there will be no stale nodes in PerNodeSettings */
	static void CleanupTraits(const UFlowGraphNode* OwnerNode);

	/** Finds OwnerNode's trait with provided Type
	 * returns null if there are no OwnerNode's trait with provided Type */
	static FFlowDebugTrait* FindTrait(const UFlowGraphNode* OwnerNode, EFlowTraitType Type);
	/** Finds OwnerPin's trait with Type
	 * returns null if there are no OwnerPin's trait with provided Type */
	static FFlowDebugTrait* FindTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	/** Sets or clears the enabled flag for the OwnerNode's trait with provided Type */
	static void SetTraitEnabled(const UFlowGraphNode* OwnerNode, EFlowTraitType Type, bool bIsEnabled);
	/** Sets or clears the enabled flag for the OwnerPin's trait with provided Type */
	static void SetTraitEnabled(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bIsEnabled);

	/** Returns the enabled flag for the OwnerNode's trait with provided Type */
	static bool IsTraitEnabled(const UFlowGraphNode* OwnerNode, EFlowTraitType Type);
	/** Returns the enabled flag for the OwnerPin's trait with provided Type */
	static bool IsTraitEnabled(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	/** Creates or removes OwnerNode's trait with provided Type */
	static void ToggleTrait(const UFlowGraphNode* OwnerNode, EFlowTraitType Type);
	/** Creates or removes OwnerPin's trait with provided Type */
	static void ToggleTrait(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	/** Sets the hit flag for the all OwnerNode's traits */
	static TArray<EFlowTraitType> SetAllTraitsHit(const UFlowGraphNode* OwnerNode, bool bHit);
	/** Sets the hit flag for the all OwnerPin's traits */
	static TArray<EFlowTraitType> SetAllTraitsHit(const UEdGraphPin* OwnerPin, bool bHit);

	/** Sets the hit flag for the OwnerNode's trait with provided Type */
	static bool SetTraitHit(const UFlowGraphNode* OwnerNode, EFlowTraitType Type, bool bHit);
	/** Sets the hit flag for the OwnerPin's trait with provided Type */
	static bool SetTraitHit(const UEdGraphPin* OwnerPin, EFlowTraitType Type, bool bHit);
	
	/** Returns the hit flag for the OwnerNode's trait with provided Type */
	static bool IsTraitHit(const UFlowGraphNode* OwnerNode, EFlowTraitType Type);
	/** Returns the hit flag for the OwnerPin's trait with provided Type */
	static bool IsTraitHit(const UEdGraphPin* OwnerPin, EFlowTraitType Type);

	/**	Retrieves the user settings associated with a FlowGraphNode.
	*	returns null if the FlowGraphNode has default settings (no nodes and pins traits) */
	static FFlowTraitSettings* GetPerNodeSettings(const UFlowGraphNode* OwnerNode);

	/**	Retrieves the Array of node's traits associated with a FlowGraphNode.
	*	returns null if there are no node's traits associated with this FlowGraphNode */
	static TArray<FFlowDebugTrait>* GetNodeTraits(const UFlowGraphNode* OwnerNode);

	/**	Retrieves the Array of pins' traits associated with a FlowGraphNode.
	*	returns null if there are no pins' traits associated with this FlowGraphNode */
	static TArray<FFlowDebugTrait>* GetPinTraits(const UFlowGraphNode* OwnerNode);

	/** Saves any modifications made to traits */
	static void SaveFlowGraphEditorSettings();
};
