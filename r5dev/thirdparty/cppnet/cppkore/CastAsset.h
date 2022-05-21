#pragma once

#include <cstdint>
#include "Exporter.h"

namespace Assets::Exporters
{
	class CastAsset : public Exporter
	{
	public:
		CastAsset() = default;
		~CastAsset() = default;

		// Exports the given animation to the provided path.
		virtual bool ExportAnimation(const Animation& Animation, const string& Path);
		// Exports the given model to the provided path.
		virtual bool ExportModel(const Model& Model, const string& Path);

		// Gets the file extension for this exporters model format.
		virtual imstring ModelExtension();
		// Gets the file extension for this exporters animation format.
		virtual imstring AnimationExtension();

		// Gets the required scaling constant for this exporter.
		virtual ExporterScale ExportScale();

		// Gets whether or not the exporter supports animation exporting.
		virtual bool SupportsAnimations();
		// Gets whether or not the exporter supports model exporting.
		virtual bool SupportsModels();
	};
}