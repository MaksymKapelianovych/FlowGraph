// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"

#include "Nodes/FlowNode.h"

#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowGraph)

UFlowGraph::UFlowGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bLockUpdates = false;
	bIsLoadingGraph = false;
}

void UFlowGraph::CreateGraph(UFlowAsset* InFlowAsset)
{
	UFlowGraph* NewGraph = CastChecked<UFlowGraph>(FBlueprintEditorUtils::CreateNewGraph(InFlowAsset, NAME_None, StaticClass(), UFlowGraphSchema::StaticClass()));
	NewGraph->bAllowDeletion = false;

	// Ensure we mapped relation between UFlowNode and UFlowGraphNode classes
	// Otherwise generating graph wouldn't assign proper UFlowGraphNode class to default nodes generated below
	// Issue only occurred if somebody would generate graph programatically without opening Flow Asset editor at least once
	UFlowGraphSchema::GatherNodes();

	InFlowAsset->FlowGraph = NewGraph;
	InFlowAsset->FlowGraph->GetSchema()->CreateDefaultNodesForGraph(*InFlowAsset->FlowGraph);
}

void UFlowGraph::RefreshGraph()
{
	// don't run fixup in PIE
	if (GEditor && !GEditor->PlayWorld)
	{
		// Locking updates to the graph while we update it
		bLockUpdates = true;
		
		// check if all Graph Nodes have expected, up-to-date type
		CastChecked<UFlowGraphSchema>(GetSchema())->GatherNativeNodes();
		for (const TPair<FGuid, UFlowNode*>& Node : GetFlowAsset()->GetNodes())
		{
			if (UFlowNode* FlowNode = Node.Value)
			{
				const UClass* ExpectGraphNodeClass = UFlowGraphSchema::GetAssignedGraphNodeClass(FlowNode->GetClass());
				if (FlowNode->GetGraphNode() && FlowNode->GetGraphNode()->GetClass() != ExpectGraphNodeClass)
				{
					// Create a new Flow Graph Node of proper type
					FFlowGraphSchemaAction_NewNode::RecreateNode(this, FlowNode->GetGraphNode(), FlowNode);
				}
			}
		}

		bLockUpdates = false;

		// refresh nodes
		TArray<UFlowGraphNode*> FlowGraphNodes;
		GetNodesOfClass<UFlowGraphNode>(FlowGraphNodes);
		for (UFlowGraphNode* GraphNode : FlowGraphNodes)
		{
			GraphNode->OnGraphRefresh();
		}
	}
}

void UFlowGraph::NotifyGraphChanged()
{
	GetFlowAsset()->HarvestNodeConnections();

	Super::NotifyGraphChanged();
}

UFlowAsset* UFlowGraph::GetFlowAsset() const
{
	return GetTypedOuter<UFlowAsset>();
}

bool UFlowGraph::IsLocked() const
{
	return bLockUpdates;
}
