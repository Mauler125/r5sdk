#pragma once

#include <cstdint>
#include "Model.h"
#include "Animation.h"
#include "StringBase.h"

namespace Assets::Exporters
{
	// This enumeration represents the possible exporter scale constants.
	enum class ExporterScale
	{
		// This exporter requires no scale modifications.
		Default,
		// This exporter requires conversion to inches.
		Inch,
		// This exporter requires conversion to centimeters.
		CM
	};

	// The interface for all asset exporter types.
	class Exporter
	{
	public:
		// Exports the given animation to the provided path.
		virtual bool ExportAnimation(const Animation& Animation, const string& Path) = 0;
		// Exports the given model to the provided path.
		virtual bool ExportModel(const Model& Model, const string& Path) = 0;

		// Gets the file extension for this exporters model format.
		virtual imstring ModelExtension() = 0;
		// Gets the file extension for this exporters animation format.
		virtual imstring AnimationExtension() = 0;

		// Gets the required scaling constant for this exporter.
		virtual ExporterScale ExportScale() = 0;

		// Gets whether or not the exporter supports animation exporting.
		virtual bool SupportsAnimations() = 0;
		// Gets whether or not the exporter supports model exporting.
		virtual bool SupportsModels() = 0;
	};
}