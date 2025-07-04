// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "EdGraph/EdGraph.h"

#include "FlowAsset.h"
#include "FlowGraph.generated.h"

class FLOWEDITOR_API FFlowGraphInterface : public IFlowGraphInterface
{
public:
	virtual ~FFlowGraphInterface() override {}

	virtual void OnInputTriggered(UEdGraphNode* GraphNode, const int32 Index) const override;
	virtual void OnOutputTriggered(UEdGraphNode* GraphNode, const int32 Index) const override;
};

UCLASS()
class FLOWEDITOR_API UFlowGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

protected:
	/** if set, graph modifications won't cause updates in internal tree structure
	 *  flag allows freezing update during heavy changes like pasting new nodes 
	 */
	uint32 bLockUpdates : 1;

	// is currently loading the Flow Graph (used to suppress some work during load)
	uint32 bIsLoadingGraph : 1;

public:
	static void CreateGraph(UFlowAsset* InFlowAsset);
	void RefreshGraph();

	// UEdGraph
	virtual void NotifyGraphChanged() override;
	// --

	/** Returns the FlowAsset that contains this graph */
	UFlowAsset* GetFlowAsset() const;

	bool IsLocked() const;

	bool IsLoadingGraph() const { return bIsLoadingGraph; }
};
