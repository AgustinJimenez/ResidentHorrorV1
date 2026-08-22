#include "MCPServerHelpers.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "StructUtils/UserDefinedStruct.h"
#include "K2Node_SetFieldsInStruct.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_Composite.h"
#include "AnimGraphNode_StateMachineBase.h"
#include "AnimationStateMachineGraph.h"

// Finds a top-level graph (Ubergraph/Function/Interface) optionally filtered by name, then
// descends through a chain of K2Node_Composite NodeGuids into nested collapsed graphs.
// If NodeIdChain is empty, GraphNameFilter must match a top-level graph exactly (legacy behavior).
UEdGraph* ResolveGraphChain(UBlueprint* Blueprint, const FString& GraphNameFilter, const TArray<FString>& NodeIdChain, FString& OutError)
{
	if (!Blueprint)
	{
		OutError = TEXT("Blueprint is null");
		return nullptr;
	}

	TArray<UEdGraph*> TopGraphs;
	TopGraphs.Append(Blueprint->UbergraphPages);
	TopGraphs.Append(Blueprint->FunctionGraphs);
	for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
	{
		TopGraphs.Append(Interface.Graphs);
	}

	UEdGraph* CurrentGraph = nullptr;

	if (NodeIdChain.Num() == 0)
	{
		for (UEdGraph* Candidate : TopGraphs)
		{
			if (Candidate && Candidate->GetName() == GraphNameFilter)
			{
				CurrentGraph = Candidate;
				break;
			}
		}
		if (!CurrentGraph)
		{
			OutError = FString::Printf(TEXT("Graph not found: %s"), *GraphNameFilter);
		}
		return CurrentGraph;
	}

	FGuid FirstGuid;
	if (!FGuid::Parse(NodeIdChain[0], FirstGuid))
	{
		OutError = FString::Printf(TEXT("Invalid NodeGuid: %s"), *NodeIdChain[0]);
		return nullptr;
	}

	for (UEdGraph* Candidate : TopGraphs)
	{
		if (!Candidate) continue;
		if (!GraphNameFilter.IsEmpty() && Candidate->GetName() != GraphNameFilter) continue;

		for (UEdGraphNode* Node : Candidate->Nodes)
		{
			if (Node && Node->NodeGuid == FirstGuid)
			{
				CurrentGraph = Candidate;
				break;
			}
		}
		if (CurrentGraph) break;
	}

	if (!CurrentGraph)
	{
		OutError = FString::Printf(TEXT("Could not find a top-level graph containing node id: %s"), *NodeIdChain[0]);
		return nullptr;
	}

	for (const FString& IdStr : NodeIdChain)
	{
		FGuid TargetGuid;
		if (!FGuid::Parse(IdStr, TargetGuid))
		{
			OutError = FString::Printf(TEXT("Invalid NodeGuid: %s"), *IdStr);
			return nullptr;
		}

		UEdGraphNode* FoundNode = nullptr;
		for (UEdGraphNode* Node : CurrentGraph->Nodes)
		{
			if (Node && Node->NodeGuid == TargetGuid)
			{
				FoundNode = Node;
				break;
			}
		}

		if (!FoundNode)
		{
			OutError = FString::Printf(TEXT("Node id %s not found in graph '%s'"), *IdStr, *CurrentGraph->GetName());
			return nullptr;
		}

		if (UK2Node_Composite* FoundComposite = Cast<UK2Node_Composite>(FoundNode))
		{
			if (!FoundComposite->BoundGraph)
			{
				OutError = FString::Printf(TEXT("Composite node %s has no BoundGraph"), *IdStr);
				return nullptr;
			}
			CurrentGraph = FoundComposite->BoundGraph;
		}
		else if (UAnimGraphNode_StateMachineBase* FoundStateMachine = Cast<UAnimGraphNode_StateMachineBase>(FoundNode))
		{
			if (!FoundStateMachine->EditorStateMachineGraph)
			{
				OutError = FString::Printf(TEXT("State machine node %s has no EditorStateMachineGraph"), *IdStr);
				return nullptr;
			}
			CurrentGraph = FoundStateMachine->EditorStateMachineGraph;
		}
		else
		{
			OutError = FString::Printf(TEXT("Node id %s is not a K2Node_Composite or AnimGraphNode_StateMachine (class: %s)"), *IdStr, *FoundNode->GetClass()->GetName());
			return nullptr;
		}
	}

	return CurrentGraph;
}

UClass* ResolveParentClass(const FString& ParentClassPath)
{
	if (ParentClassPath.IsEmpty())
	{
		return nullptr;
	}

	UClass* NewParent = FindFirstObject<UClass>(*ParentClassPath, EFindFirstObjectOptions::ExactClass);
	if (!NewParent && !ParentClassPath.EndsWith(TEXT("_C")))
	{
		NewParent = FindFirstObject<UClass>(*(ParentClassPath + TEXT("_C")), EFindFirstObjectOptions::ExactClass);
	}

	if (!NewParent)
	{
		NewParent = LoadObject<UClass>(nullptr, *ParentClassPath);
	}
	if (!NewParent)
	{
		NewParent = LoadClass<UObject>(nullptr, *ParentClassPath);
	}
	if (!NewParent && !ParentClassPath.EndsWith(TEXT("_C")))
	{
		NewParent = LoadObject<UClass>(nullptr, *(ParentClassPath + TEXT("_C")));
		if (!NewParent)
		{
			NewParent = LoadClass<UObject>(nullptr, *(ParentClassPath + TEXT("_C")));
		}
	}

	if (!NewParent)
	{
		if (UBlueprint* ParentBP = LoadObject<UBlueprint>(nullptr, *ParentClassPath))
		{
			NewParent = ParentBP->GeneratedClass;
		}
	}

	return NewParent;
}

void SerializePinType(const FEdGraphPinType& PinType, TSharedPtr<FJsonObject>& OutObj)
{
	OutObj->SetStringField(TEXT("category"), PinType.PinCategory.ToString());
	OutObj->SetStringField(TEXT("subcategory"), PinType.PinSubCategory.ToString());
	if (PinType.PinSubCategoryObject.IsValid())
	{
		OutObj->SetStringField(TEXT("subcategory_object"), PinType.PinSubCategoryObject->GetName());
	}
	OutObj->SetBoolField(TEXT("is_array"), PinType.ContainerType == EPinContainerType::Array);
	OutObj->SetBoolField(TEXT("is_set"), PinType.ContainerType == EPinContainerType::Set);
	OutObj->SetBoolField(TEXT("is_map"), PinType.ContainerType == EPinContainerType::Map);
	OutObj->SetBoolField(TEXT("is_reference"), PinType.bIsReference);
	OutObj->SetBoolField(TEXT("is_const"), PinType.bIsConst);
}

void SerializeProperty(const FProperty* Prop, TSharedPtr<FJsonObject>& OutObj)
{
	OutObj->SetStringField(TEXT("name"), Prop->GetName());
	OutObj->SetStringField(TEXT("property_class"), Prop->GetClass()->GetName());

	bool bIsArray = false;
	if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
	{
		bIsArray = true;
		OutObj->SetStringField(TEXT("inner_property_class"), ArrayProp->Inner->GetClass()->GetName());
		if (const FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner))
		{
			OutObj->SetStringField(TEXT("inner_struct"), InnerStruct->Struct->GetName());
		}
		if (const FObjectPropertyBase* InnerObj = CastField<FObjectPropertyBase>(ArrayProp->Inner))
		{
			OutObj->SetStringField(TEXT("inner_object_class"), InnerObj->PropertyClass->GetName());
		}
	}
	OutObj->SetBoolField(TEXT("is_array"), bIsArray);

	if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
	{
		OutObj->SetStringField(TEXT("struct_type"), StructProp->Struct->GetName());
	}
	else if (const FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop))
	{
		OutObj->SetStringField(TEXT("object_class"), ObjProp->PropertyClass->GetName());
	}
	else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
	{
		OutObj->SetStringField(TEXT("enum_type"), EnumProp->GetEnum() ? EnumProp->GetEnum()->GetName() : TEXT(""));
	}
	else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
	{
		if (ByteProp->Enum)
		{
			OutObj->SetStringField(TEXT("enum_type"), ByteProp->Enum->GetName());
		}
	}
}

static bool IsMatchingStruct(UObject* PinObj, UUserDefinedStruct* OldStruct)
{
	if (!PinObj) return false;
	if (PinObj == OldStruct) return true;
	// Name-based fallback: UE can have multiple loaded instances of the same struct
	return PinObj->GetName() == OldStruct->GetName();
}

bool DoesBlueprintReferenceStruct(UBlueprint* Blueprint, UUserDefinedStruct* OldStruct)
{
	// Check variables
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		if (Var.VarType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
			IsMatchingStruct(Var.VarType.PinSubCategoryObject.Get(), OldStruct))
		{
			return true;
		}
	}

	// Check graph nodes
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;

			// Check struct nodes directly
			if (UK2Node_BreakStruct* BreakNode = Cast<UK2Node_BreakStruct>(Node))
			{
				if (BreakNode->StructType && BreakNode->StructType->GetName() == OldStruct->GetName()) return true;
			}
			else if (UK2Node_MakeStruct* MakeNode = Cast<UK2Node_MakeStruct>(Node))
			{
				if (MakeNode->StructType && MakeNode->StructType->GetName() == OldStruct->GetName()) return true;
			}
			else if (UK2Node_SetFieldsInStruct* SetNode = Cast<UK2Node_SetFieldsInStruct>(Node))
			{
				if (SetNode->StructType && SetNode->StructType->GetName() == OldStruct->GetName()) return true;
			}

			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
					IsMatchingStruct(Pin->PinType.PinSubCategoryObject.Get(), OldStruct))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool DoesBlueprintReferenceEnum(UBlueprint* Blueprint, UEnum* EnumToFind)
{
	// Check variables
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		if (Var.VarType.PinSubCategoryObject.Get() == EnumToFind)
		{
			return true;
		}
	}

	// Check all graphs including sub-graphs
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	TArray<UEdGraph*> GraphsToProcess = AllGraphs;
	while (GraphsToProcess.Num() > 0)
	{
		UEdGraph* Graph = GraphsToProcess.Pop();
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->PinType.PinSubCategoryObject.Get() == EnumToFind)
				{
					return true;
				}
			}
			for (UEdGraph* SubGraph : Node->GetSubGraphs())
			{
				if (SubGraph && !AllGraphs.Contains(SubGraph))
				{
					AllGraphs.Add(SubGraph);
					GraphsToProcess.Add(SubGraph);
				}
			}
		}
	}
	return false;
}

void SaveNodeConnections(UEdGraphNode* Node, TArray<FSavedPinConnection>& OutConnections)
{
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin) continue;
		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (!LinkedPin || !LinkedPin->GetOwningNodeUnchecked()) continue;
			FSavedPinConnection Conn;
			Conn.PinName = Pin->PinName;
			Conn.Direction = Pin->Direction;
			Conn.RemoteNodeGuid = LinkedPin->GetOwningNode()->NodeGuid;
			Conn.RemotePinName = LinkedPin->PinName;
			OutConnections.Add(Conn);
		}
	}
}

void RestoreNodeConnections(
	UEdGraphNode* Node,
	const TArray<FSavedPinConnection>& SavedConnections,
	const TMap<FName, FName>& FieldNameMap,
	UEdGraph* Graph)
{
	for (const FSavedPinConnection& Conn : SavedConnections)
	{
		// Map old GUID-suffixed pin name to clean C++ name
		FName MappedPinName = Conn.PinName;
		if (const FName* NewName = FieldNameMap.Find(Conn.PinName))
		{
			MappedPinName = *NewName;
		}

		// Find our pin by mapped name
		UEdGraphPin* OurPin = Node->FindPin(MappedPinName, Conn.Direction);
		if (!OurPin)
		{
			// Try exact old name (for non-struct pins like exec, struct input/output)
			OurPin = Node->FindPin(Conn.PinName, Conn.Direction);
		}
		if (!OurPin) continue;

		// Find the remote node and pin
		for (UEdGraphNode* OtherNode : Graph->Nodes)
		{
			if (OtherNode && OtherNode->NodeGuid == Conn.RemoteNodeGuid)
			{
				EEdGraphPinDirection RemoteDir = (Conn.Direction == EGPD_Input) ? EGPD_Output : EGPD_Input;
				// Try exact remote pin name first
				UEdGraphPin* RemotePin = OtherNode->FindPin(Conn.RemotePinName, RemoteDir);
				if (!RemotePin)
				{
					// Remote pin may also have been remapped
					FName MappedRemoteName = Conn.RemotePinName;
					if (const FName* NewRemoteName = FieldNameMap.Find(Conn.RemotePinName))
					{
						MappedRemoteName = *NewRemoteName;
					}
					RemotePin = OtherNode->FindPin(MappedRemoteName, RemoteDir);
				}
				if (RemotePin && !OurPin->LinkedTo.Contains(RemotePin))
				{
					OurPin->MakeLinkTo(RemotePin);
				}
				break;
			}
		}
	}
}
