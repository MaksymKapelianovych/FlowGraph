// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "NativeGameplayTags.h"

namespace FlowNodeStyle
{
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CategoryName);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Custom);

	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Node);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Default);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Condition);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Deprecated);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Developer);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InOut);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Latent);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Logic);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SubGraph);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Terminal);

	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AddOn);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AddOn_PerSpawnedActor);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AddOn_Predicate);
	FLOW_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(AddOn_Predicate_Composite);
}
