// Copyright 2020-2022, Roberto De Ioris.

#include "glTFRuntimeDraco.h"

#define LOCTEXT_NAMESPACE "FglTFRuntimeDracoModule"


#include "glTFRuntimeParser.h"

#include "draco/compression/decode.h"
#include "draco/core/decoder_buffer.h"

static const FString IndicesName = "KHR_draco_mesh_compression_primitive_indices";

template<typename T>
static bool DecodeAndAddDracoAttribute(TSharedRef<FglTFRuntimeParser> Parser, FglTFRuntimePrimitive& Primitive, const FString& Name, const uint32 NumPoints, const draco::PointAttribute* AttributePtr)
{
	TArray<T> Data;
	for (draco::PointIndex Index(0); Index < NumPoints; Index++)
	{
		const draco::AttributeValueIndex ValueIndex = AttributePtr->mapped_index(Index);
		Data.Append(reinterpret_cast<const T*>(AttributePtr->GetAddress(ValueIndex)), AttributePtr->num_components());
	}
	Parser->AddAdditionalBufferViewData(Primitive.AdditionalBufferView, Name, Data);
	return true;
}

static void FillPrimitiveAdditionalBufferViewsFromDraco(TSharedRef<FglTFRuntimeParser> Parser, TSharedRef<FJsonObject> JsonPrimitiveObject, FglTFRuntimePrimitive& Primitive)
{

	TSharedPtr<FJsonObject> DracoExtension = Parser->GetJsonObjectExtension(JsonPrimitiveObject, "KHR_draco_mesh_compression");
	if (!DracoExtension)
	{
		return;
	}

	int64 BufferView = Parser->GetJsonObjectIndex(DracoExtension.ToSharedRef(), "bufferView");
	if (BufferView <= INDEX_NONE)
	{
		Parser->AddError("FillPrimitiveAdditionalBufferViewsFromDraco()", "KHR_draco_mesh_compression requires a valid bufferView");
		return;
	}

	Primitive.AdditionalBufferView = BufferView;

	TSharedPtr<FJsonObject> DracoAttributes = Parser->GetJsonObjectFromObject(DracoExtension.ToSharedRef(), "attributes");
	if (!DracoAttributes)
	{
		return;
	}

	TMap<FString, int64> RequiredAttributes;
	Parser->ForEachJsonFieldAsIndex(DracoAttributes.ToSharedRef(), [BufferView, &RequiredAttributes](const FString& Key, const int64 Index, TSharedRef<FglTFRuntimeParser> Parser)
		{
			if (Index > INDEX_NONE)
			{
				if (!Parser->GetAdditionalBufferView(BufferView, Key))
				{
					RequiredAttributes.Add(Key, Index);
				}
			}
		}, Parser);

	// do we need indices?
	bool bRequiresIndices = false;
	if (Parser->GetJsonObjectIndex(JsonPrimitiveObject, "indices") > INDEX_NONE)
	{
		if (!Parser->GetAdditionalBufferView(BufferView, IndicesName))
		{
			bRequiresIndices = true;
		}
	}

	if (RequiredAttributes.Num() == 0 && !bRequiresIndices)
	{
		return;
	}

	FglTFRuntimeBlob DracoBlob;
	int64 Stride;
	if (!Parser->GetBufferView(BufferView, DracoBlob, Stride))
	{
		Parser->AddError("FillPrimitiveAdditionalBufferViewsFromDraco()", "KHR_draco_mesh_compression has an invalid bufferView");
		return;
	}

	draco::DecoderBuffer DecoderBuffer;
	DecoderBuffer.Init(reinterpret_cast<char*>(DracoBlob.Data), static_cast<size_t>(DracoBlob.Num));

	draco::Decoder Decoder;
	auto DecodeResult = Decoder.DecodeMeshFromBuffer(&DecoderBuffer);
	if (!DecodeResult.ok())
	{
		Parser->AddError("FillPrimitiveAdditionalBufferViewsFromDraco()", "KHR_draco_mesh_compression failed to decode compressed buffer");
		return;
	}

	auto& Mesh = DecodeResult.value();

	if (bRequiresIndices)
	{
		TArray<uint32> Indices;
		Indices.AddUninitialized(Mesh->num_faces() * 3);

		for (uint32 FaceIndex = 0; FaceIndex < Mesh->num_faces(); FaceIndex++)
		{
			auto& Face = Mesh->face(draco::FaceIndex(FaceIndex));

			Indices[FaceIndex * 3] = Face[0].value();
			Indices[FaceIndex * 3 + 1] = Face[1].value();
			Indices[FaceIndex * 3 + 2] = Face[2].value();
		}

		Parser->AddAdditionalBufferViewData(BufferView, IndicesName, Indices);
	}

	uint32 NumPoints = Mesh->num_points();

	for (const TPair<FString, int64>& Pair : RequiredAttributes)
	{
		auto AttributePtr = Mesh->GetAttributeByUniqueId(Pair.Value);

		if (!AttributePtr)
		{
			continue;
		}

		draco::DataType DracoDataType = AttributePtr->data_type();
		bool bSuccess = false;

		switch (DracoDataType)
		{
		case(draco::DataType::DT_INT8):
			bSuccess = DecodeAndAddDracoAttribute<int8>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_UINT8):
			bSuccess = DecodeAndAddDracoAttribute<uint8>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_INT16):
			bSuccess = DecodeAndAddDracoAttribute<int16>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_UINT16):
			bSuccess = DecodeAndAddDracoAttribute<uint16>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_INT32):
			bSuccess = DecodeAndAddDracoAttribute<int32>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_UINT32):
			bSuccess = DecodeAndAddDracoAttribute<uint32>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_INT64):
			bSuccess = DecodeAndAddDracoAttribute<int64>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_UINT64):
			bSuccess = DecodeAndAddDracoAttribute<uint64>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_FLOAT32):
			bSuccess = DecodeAndAddDracoAttribute<float>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_FLOAT64):
			bSuccess = DecodeAndAddDracoAttribute<double>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		case(draco::DataType::DT_BOOL):
			bSuccess = DecodeAndAddDracoAttribute<bool>(Parser, Primitive, Pair.Key, NumPoints, AttributePtr);
			break;
		default:
			break;
		}

		if (!bSuccess)
		{
			Parser->AddError("FillPrimitiveAdditionalBufferViewsFromDraco()", FString::Printf(TEXT("KHR_draco_mesh_compression failed to decode compressed attribute \"%s\""), *Pair.Key));
		}
	}
}

static void FillPrimitiveIndicesFromDraco(TSharedRef<FglTFRuntimeParser> Parser, TSharedRef<FJsonObject> JsonPrimitiveObject, FglTFRuntimePrimitive& Primitive)
{

	TSharedPtr<FJsonObject> DracoExtension = Parser->GetJsonObjectExtension(JsonPrimitiveObject, "KHR_draco_mesh_compression");
	if (!DracoExtension)
	{
		return;
	}

	const FglTFRuntimeBlob* IndicesBlob = Parser->GetAdditionalBufferView(Primitive.AdditionalBufferView, IndicesName);
	if (!IndicesBlob)
	{
		return;
	}

	const uint32* IndicesPtr = reinterpret_cast<const uint32*>(IndicesBlob->Data);

	for (int32 VertexIndex = 0; VertexIndex < Primitive.Indices.Num(); VertexIndex++)
	{
		Primitive.Indices[VertexIndex] = IndicesPtr[VertexIndex];
	}

}

void FglTFRuntimeDracoModule::StartupModule()
{
	FglTFRuntimeParser::OnPreLoadedPrimitive.AddStatic(FillPrimitiveAdditionalBufferViewsFromDraco);
	FglTFRuntimeParser::OnLoadedPrimitive.AddStatic(FillPrimitiveIndicesFromDraco);
}

void FglTFRuntimeDracoModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FglTFRuntimeDracoModule, glTFRuntimeDraco)