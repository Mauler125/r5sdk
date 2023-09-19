#include "core/stdafx.h"
#include "public/dt_common.h"
#include "public/datamap.h"

// [ PIXIE ]: Should work out of box when you pass proper datamap.
typedescription_t* DataMapHandler::FindFieldInDataMap(datamap_t* map, const string& fieldName)
{
	while (map)
	{
		for (int i = 0; i < map->dataNumFields; i++)
		{
			string descFieldName = map->dataDesc[i].fieldName;
			if (descFieldName.empty())
				continue;

			if (descFieldName.compare(fieldName) == 0)
				return &map->dataDesc[i];

			if (map->dataDesc[i].fieldType == 10) // FIELD_EMBEDDED
			{
				if (map->dataDesc[i].td)
				{
					typedescription_t* field = FindFieldInDataMap(map->dataDesc[i].td, fieldName);
					if (field)
						return field;
				}
			}
		}
		map = map->baseMap;
	}

	return nullptr;
}

int DataMapHandler::FindOffsetForField(datamap_t* map, const string& fieldName)
{
	typedescription_t* field = FindFieldInDataMap(map, fieldName);
	if (field)
		return field->fieldOffset;

	return 0;
}