#include <iostream>
#include <stdint.h>
#include <windows.h>
#include "parson.h"

#if defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef struct iot_pnp {
	JSON_Value *el_definitions_value;
	JSON_Object *el_definitions_object;
	JSON_Value *dt_root_value;
	JSON_Array *dt_root_array;
	JSON_Value *dt_contents_value;
	JSON_Array *dt_contents_array;
} iot_pnp;

typedef enum access_rule {
	ACCESS_RULE_NONE,
	ACCESS_RULE_REQUIRED,
	ACCESS_RULE_BY_CASE,
	ACCESS_RULE_OPTIONAL,
	ACCESS_RULE_NA,
} access_rule;

typedef enum data_type {
	DATA_TYPE_NONE,
	DATA_TYPE_STATE,
	DATA_TYPE_OBJECT,
	DATA_TYPE_DATE_TIME,
	DATA_TYPE_TIME,
	DATA_TYPE_RAW,
	DATA_TYPE_ARRAY,
	DATA_TYPE_BITMAP,
	DATA_TYPE_LEVEL,
	DATA_TYPE_NUMBER,
	DATA_TYPE_NUMERIC_VALUE,
	DATA_TYPE_ONE_OF,
} data_type;

typedef enum number_format {
	NUMBER_FORMAT_NONE,
	NUMBER_FORMAT_INT8,
	NUMBER_FORMAT_INT16,
	NUMBER_FORMAT_INT32,
	NUMBER_FORMAT_UINT8,
	NUMBER_FORMAT_UINT16,
	NUMBER_FORMAT_UINT32,
} number_format;

typedef struct edt_info {
	int edt;
	const char *stateJa, *stateEn;
	double numericValue;
	bool edt_readOnly;
} edt_info;

typedef struct data_info {
	data_type type;
	int size;
	const char *name;
	const char *ref;
	const char *unit;
	double multipleOf, minSize, maxSize;
	int itemSize, minItems, maxItems, minimum, maximum;
	const char *base;
	number_format numFormat;
	void *number_enum;
	int edtCount;
	edt_info *edts;
	int coefficientEpcCount;
	const char **coefficientEpcs;
	int dataInfoCount;
	data_info *dataInfos;
	int bitmapInfoCount;
	struct bitmap_info *bitmapInfos;
};

typedef struct bitmap_info {
	const char *name;
	const char *descriptionsJa;
	const char *descriptionsEn;
	int index;
	const char *bitMask;
	data_info value;
} bitmap_info;

typedef enum dt_if_type {
	DT_IF_TYPE_NONE,
	DT_IF_TYPE_TELEMETRY,
	DT_IF_TYPE_PROPERTY,
	DT_IF_TYPE_COMMAND,
} dt_if_type;

access_rule get_access_rule(const char *rule)
{
	if (strcmp(rule, "required") == 0) {
		return ACCESS_RULE_REQUIRED;
	}
	else if (strcmp(rule, "required_c") == 0) {
		return ACCESS_RULE_BY_CASE;
	}
	else if (strcmp(rule, "optional") == 0) {
		return ACCESS_RULE_OPTIONAL;
	}
	else if (strcmp(rule, "notApplicable") == 0) {
		return ACCESS_RULE_NA;
	}
	else {
		return ACCESS_RULE_NONE;
	}
}

int set_digital_twin_id(char *temp, const char *id, int len)
{
	int i = 0;

	for (char c = *id; ((c = *id) != '\0') && (i < len); id++, i++) {
		if (((c >= '0') && (c <= '9')) || (c == '_')
			|| ((c >= 'A') && (c <= 'Z'))
			|| ((c >= 'a') && (c <= 'z'))) {
			temp[i] = c;
		}
		else {
			temp[i] = '_';
		}
	}

	if (i >= len - 1)
		i = len - 1;

	temp[i] = '\0';

	return i;
}

void free_data_info(data_info *dataInfo)
{
	free(dataInfo->edts);
	free(dataInfo->number_enum);
	free(dataInfo->coefficientEpcs);

	data_info *dataInfo2 = dataInfo->dataInfos;
	for (int i = 0; i < dataInfo->dataInfoCount; i++, dataInfo2++) {
		free_data_info(dataInfo2);
	}
	free(dataInfo->dataInfos);

	bitmap_info *bitmapInfo = dataInfo->bitmapInfos;
	for (int i = 0; i < dataInfo->bitmapInfoCount; i++, bitmapInfo++) {
		free_data_info(&bitmapInfo->value);
	}
	free(dataInfo->bitmapInfos);
}

void parse_data(iot_pnp *iot_pnp, JSON_Object *data, data_info *dataInfo)
{
	if (dataInfo->type != DATA_TYPE_NONE)
		DebugBreak();

	for (int j = 0; j < json_object_get_count(data); j++) {
		const char *member = json_object_get_name(data, j);

		if (strcmp(member, "type") == 0) {
			const char *type_str;

			type_str = json_object_get_string(data, "type");
			if (type_str == NULL) {
				DebugBreak();
				continue;
			}

			if (strcmp(type_str, "state") == 0) {
				dataInfo->type = DATA_TYPE_STATE;
			}
			else if (strcmp(type_str, "object") == 0) {
				dataInfo->type = DATA_TYPE_OBJECT;
			}
			else if (strcmp(type_str, "date-time") == 0) {
				dataInfo->type = DATA_TYPE_DATE_TIME;
			}
			else if (strcmp(type_str, "time") == 0) {
				dataInfo->type = DATA_TYPE_TIME;
			}
			else if (strcmp(type_str, "raw") == 0) {
				dataInfo->type = DATA_TYPE_RAW;
			}
			else if (strcmp(type_str, "array") == 0) {
				dataInfo->type = DATA_TYPE_ARRAY;
			}
			else if (strcmp(type_str, "bitmap") == 0) {
				dataInfo->type = DATA_TYPE_BITMAP;
			}
			else if (strcmp(type_str, "level") == 0) {
				dataInfo->type = DATA_TYPE_LEVEL;
			}
			else if (strcmp(type_str, "number") == 0) {
				dataInfo->type = DATA_TYPE_NUMBER;
			}
			else if (strcmp(type_str, "numericValue") == 0) {
				dataInfo->type = DATA_TYPE_NUMERIC_VALUE;
			}
			else {
				DebugBreak();
				continue;
			}
		}
		else if (strcmp(member, "size") == 0) {
			JSON_Value *size_value;

			size_value = json_object_get_value(data, "size");
			if (size_value == NULL) {
				DebugBreak();
				continue;
			}

			if (json_value_get_type(size_value) == JSONString) {
				dataInfo->size = atoi(json_value_get_string(size_value));
			}
			else if (json_value_get_type(size_value) == JSONNumber) {
				dataInfo->size = (int)json_value_get_number(size_value);
			}
			else {
				DebugBreak();
				continue;
			}
		}
		else if (strcmp(member, "enum") == 0) {
			JSON_Array *data_enum;

			data_enum = json_object_get_array(data, "enum");
			if (data_enum == NULL) {
				DebugBreak();
				continue;
			}

			if ((dataInfo->type == DATA_TYPE_STATE) || (dataInfo->type == DATA_TYPE_NUMERIC_VALUE)) {
				int enumCount = json_array_get_count(data_enum);
				edt_info *edt = (edt_info *)malloc(sizeof(edt_info) * enumCount);
				if (dataInfo->edts != NULL)
					DebugBreak();
				dataInfo->edtCount = enumCount;
				dataInfo->edts = edt;

				for (int k = 0; k < enumCount; k++, edt++) {
					JSON_Object *edtInfo = json_array_get_object(data_enum, k);
					if (edtInfo == NULL) {
						DebugBreak();
						continue;
					}

					memset(edt, 0, sizeof(*edt));

					for (int l = 0; l < json_object_get_count(edtInfo); l++) {
						const char *info_member = json_object_get_name(edtInfo, l);
						const char *edt_str;

						if (strcmp(info_member, "edt") == 0) {
							JSON_Value *edt_value = json_object_get_value(edtInfo, "edt");
							if (json_value_get_type(edt_value) == JSONString) {
								edt_str = json_object_get_string(edtInfo, "edt");
								if (_strnicmp(edt_str, "0x", 2) == 0) {
									edt->edt = strtol(&edt_str[2], NULL, 16);
								}
								else {
									edt->edt = atoi(edt_str);
								}
							}
							else if (json_value_get_type(edt_value) == JSONNumber) {
								edt->edt = (int)json_value_get_number(edt_value);
							}
							else {
								DebugBreak();
								continue;
							}
						}
						else if (strcmp(info_member, "state") == 0) {
							JSON_Object *edt_state;

							edt_state = json_object_get_object(edtInfo, "state");
							edt->stateJa = json_object_get_string(edt_state, "ja");
							edt->stateEn = json_object_get_string(edt_state, "en");
						}
						else if (strcmp(info_member, "numericValue") == 0) {
							edt->numericValue = json_object_get_number(edtInfo, "numericValue");
						}
						else if (strcmp(info_member, "readOnly") == 0) {
							edt->edt_readOnly = json_object_get_boolean(edtInfo, "readOnly");
						}
						else {
							DebugBreak();
							continue;
						}
					}
				}
			}
			else if (dataInfo->type == DATA_TYPE_NUMBER) {
				size_t count = json_array_get_count(data_enum);

				if (dataInfo->number_enum != NULL)
					DebugBreak();

				switch (dataInfo->numFormat) {
				case NUMBER_FORMAT_INT8: {
					dataInfo->number_enum = malloc(sizeof(int8_t) * count);
					if (dataInfo->number_enum == NULL) {
						DebugBreak();
						break;
					}

					int8_t *item = (int8_t *)dataInfo->number_enum;
					for (int k = 0; k < count; k++, item++) {
						*item = (int8_t)json_array_get_number(data_enum, k);
					}
					break;
				}
				case NUMBER_FORMAT_INT16: {
					dataInfo->number_enum = malloc(sizeof(int16_t) * count);
					if (dataInfo->number_enum == NULL) {
						DebugBreak();
						break;
					}

					int16_t *item = (int16_t *)dataInfo->number_enum;
					for (int k = 0; k < count; k++, item++) {
						*item = (int16_t)json_array_get_number(data_enum, k);
					}
					break;
				}
				case NUMBER_FORMAT_INT32: {
					dataInfo->number_enum = malloc(sizeof(int32_t) * count);
					if (dataInfo->number_enum == NULL) {
						DebugBreak();
						break;
					}

					int32_t *item = (int32_t *)dataInfo->number_enum;
					for (int k = 0; k < count; k++, item++) {
						*item = (int32_t)json_array_get_number(data_enum, k);
					}
					break;
				}
				case NUMBER_FORMAT_UINT8: {
					dataInfo->number_enum = malloc(sizeof(uint8_t) * count);
					if (dataInfo->number_enum == NULL) {
						DebugBreak();
						break;
					}

					uint8_t *item = (uint8_t *)dataInfo->number_enum;
					for (int k = 0; k < count; k++, item++) {
						*item = (uint8_t)json_array_get_number(data_enum, k);
					}
					break;
				}
				case NUMBER_FORMAT_UINT16: {
					dataInfo->number_enum = malloc(sizeof(uint16_t) * count);
					if (dataInfo->number_enum == NULL) {
						DebugBreak();
						break;
					}

					uint16_t *item = (uint16_t *)dataInfo->number_enum;
					for (int k = 0; k < count; k++, item++) {
						*item = (uint16_t)json_array_get_number(data_enum, k);
					}
					break;
				}
				case NUMBER_FORMAT_UINT32: {
					dataInfo->number_enum = malloc(sizeof(uint32_t) * count);
					if (dataInfo->number_enum == NULL) {
						DebugBreak();
						break;
					}

					uint32_t *item = (uint32_t *)dataInfo->number_enum;
					for (int k = 0; k < count; k++, item++) {
						*item = (uint32_t)json_array_get_number(data_enum, k);
					}
					break;
				}
				default: {
					DebugBreak();
					continue;
				}
				}
			}
			else {
				DebugBreak();
				continue;
			}
		}
		else if (strcmp(member, "properties") == 0) {
			JSON_Array *data_properties;

			data_properties = json_object_get_array(data, "properties");
			if (data_properties == NULL) {
				DebugBreak();
				continue;
			}

			int dataInfoCount = json_array_get_count(data_properties);
			data_info *dataInfo2 = (data_info *)malloc(sizeof(data_info) * dataInfoCount);
			if (dataInfo->dataInfos != NULL)
				DebugBreak();

			dataInfo->dataInfoCount = dataInfoCount;
			dataInfo->dataInfos = dataInfo2;

			for (int k = 0; k < dataInfoCount; k++, dataInfo2++) {
				JSON_Object *propInfo;

				propInfo = json_array_get_object(data_properties, k);
				if (propInfo == NULL) {
					DebugBreak();
					continue;
				}

				memset(dataInfo2, 0, sizeof(*dataInfo2));

				for (int l = 0; l < json_object_get_count(propInfo); l++) {
					const char *info_member = json_object_get_name(propInfo, l);

					if (strcmp(info_member, "name") == 0) {
						dataInfo2->name = json_object_get_string(propInfo, "name");
					}
					else if (strcmp(info_member, "element") == 0) {
						JSON_Object *element = json_object_get_object(propInfo, "element");

						parse_data(iot_pnp, element, dataInfo2);
					}
					else {
						DebugBreak();
						continue;
					}
				}
			}
		}
		else if (strcmp(member, "unit") == 0) {
			dataInfo->unit = json_object_get_string(data, "unit");
		}
		else if (strcmp(member, "multipleOf") == 0) {
			dataInfo->multipleOf = json_object_get_number(data, "multipleOf");
		}
		else if (strcmp(member, "minSize") == 0) {
			dataInfo->minSize = json_object_get_number(data, "minSize");
		}
		else if (strcmp(member, "maxSize") == 0) {
			dataInfo->maxSize = json_object_get_number(data, "maxSize");
		}
		else if (strcmp(member, "itemSize") == 0) {
			dataInfo->itemSize = (int)json_object_get_number(data, "itemSize");
		}
		else if (strcmp(member, "minItems") == 0) {
			dataInfo->minItems = (int)json_object_get_number(data, "minItems");
		}
		else if (strcmp(member, "maxItems") == 0) {
			dataInfo->maxItems = (int)json_object_get_number(data, "maxItems");
		}
		else if (strcmp(member, "base") == 0) {
			dataInfo->base = json_object_get_string(data, "base");
		}
		else if (strcmp(member, "minimum") == 0) {
			dataInfo->minimum = (int)json_object_get_number(data, "minimum");
		}
		else if (strcmp(member, "maximum") == 0) {
			dataInfo->maximum = (int)json_object_get_number(data, "maximum");
		}
		else if (strcmp(member, "coefficient") == 0) {
			JSON_Array *coefficient = json_object_get_array(data, "coefficient");
			if (coefficient == NULL) {
				DebugBreak();
				continue;
			}

			int count = json_array_get_count(coefficient);
			const char **epcs = (const char **)malloc(sizeof(const char *) * count);
			if (dataInfo->coefficientEpcs != NULL)
				DebugBreak();
			dataInfo->coefficientEpcCount = count;
			dataInfo->coefficientEpcs = epcs;

			for (int j = 0; j < count; j++, epcs++) {
				const char *epc;

				epc = json_array_get_string(coefficient, j);
				if (epc == NULL) {
					DebugBreak();
					continue;
				}

				*epcs = epc;
			}
		}
		else if (strcmp(member, "items") == 0) {
			JSON_Object *data2;

			data2 = json_object_get_object(data, "items");
			if (data2 == NULL) {
				DebugBreak();
				continue;
			}

			int dataInfoCount = 1;
			data_info *dataInfo2 = (data_info *)malloc(sizeof(data_info) * dataInfoCount);
			if (dataInfo->dataInfos != NULL)
				DebugBreak();

			dataInfo->dataInfoCount = dataInfoCount;
			dataInfo->dataInfos = dataInfo2;

			memset(dataInfo2, 0, sizeof(*dataInfo2));

			parse_data(iot_pnp, data2, dataInfo2);
		}
		else if (strcmp(member, "bitmaps") == 0) {
			JSON_Array *bitmaps;

			bitmaps = json_object_get_array(data, "bitmaps");
			if (bitmaps == NULL) {
				DebugBreak();
				continue;
			}

			int bitmapInfoCount = json_array_get_count(bitmaps);
			bitmap_info *bitmapInfo = (bitmap_info *)malloc(sizeof(bitmap_info) * bitmapInfoCount);
			if (dataInfo->bitmapInfos != NULL)
				DebugBreak();

			dataInfo->bitmapInfoCount = bitmapInfoCount;
			dataInfo->bitmapInfos = bitmapInfo;

			for (int k = 0; k < bitmapInfoCount; k++, bitmapInfo++) {
				JSON_Object *bitmap;

				bitmap = json_array_get_object(bitmaps, k);
				if (bitmap == NULL) {
					DebugBreak();
					continue;
				}

				memset(bitmapInfo, 0, sizeof(*bitmapInfo));

				for (int l = 0; l < json_object_get_count(bitmap); l++) {
					const char *info_member = json_object_get_name(bitmap, l);

					if (strcmp(info_member, "name") == 0) {
						bitmapInfo->name = json_object_get_string(bitmap, "name");
					}
					else if (strcmp(info_member, "descriptions") == 0) {
						JSON_Object *description;

						description = json_object_get_object(bitmap, "descriptions");
						if (description == NULL) {
							DebugBreak();
							continue;
						}

						bitmapInfo->descriptionsJa = json_object_get_string(description, "ja");
						bitmapInfo->descriptionsEn = json_object_get_string(description, "en");
					}
					else if (strcmp(info_member, "position") == 0) {
						JSON_Object *position;

						position = json_object_get_object(bitmap, "position");
						if (position == NULL) {
							DebugBreak();
							continue;
						}

						bitmapInfo->index = (int)json_object_get_number(position, "index");
						bitmapInfo->bitMask = json_object_get_string(position, "bitMask");
					}
					else if (strcmp(info_member, "value") == 0) {
						JSON_Object *value = json_object_get_object(bitmap, "value");

						parse_data(iot_pnp, value, &bitmapInfo->value);
					}
					else {
						DebugBreak();
						continue;
					}
				}
			}
		}
		else if (strcmp(member, "oneOf") == 0) {
			if (dataInfo->type != DATA_TYPE_NONE) {
				DebugBreak();
				continue;
			}

			dataInfo->type = DATA_TYPE_ONE_OF;

			JSON_Array *oneOf = json_object_get_array(data, "oneOf");
			if (oneOf == NULL) {
				DebugBreak();
				continue;
			}

			int dataInfoCount = json_array_get_count(oneOf);
			data_info *dataInfo2 = (data_info *)malloc(sizeof(data_info) * dataInfoCount);
			if (dataInfo->dataInfos != NULL)
				DebugBreak();

			dataInfo->dataInfoCount = dataInfoCount;
			dataInfo->dataInfos = dataInfo2;

			for (int j = 0; j < dataInfoCount; j++, dataInfo2++) {
				JSON_Object *data2;

				data2 = json_array_get_object(oneOf, j);
				if (data2 == NULL) {
					DebugBreak();
					continue;
				}

				memset(dataInfo2, 0, sizeof(*dataInfo2));

				parse_data(iot_pnp, data2, dataInfo2);
			}
		}
		else if (strcmp(member, "format") == 0) {
			const char *format = json_object_get_string(data, "format");

			if (strcmp(format, "int8") == 0) {
				dataInfo->numFormat = NUMBER_FORMAT_INT8;
			}
			else if (strcmp(format, "int16") == 0) {
				dataInfo->numFormat = NUMBER_FORMAT_INT16;
			}
			else if (strcmp(format, "int32") == 0) {
				dataInfo->numFormat = NUMBER_FORMAT_INT32;
			}
			else if (strcmp(format, "uint8") == 0) {
				dataInfo->numFormat = NUMBER_FORMAT_UINT8;
			}
			else if (strcmp(format, "uint16") == 0) {
				dataInfo->numFormat = NUMBER_FORMAT_UINT16;
			}
			else if (strcmp(format, "uint32") == 0) {
				dataInfo->numFormat = NUMBER_FORMAT_UINT32;
			}
			else {
				DebugBreak();
				continue;
			}
		}
		else if (strcmp(member, "$ref") == 0) {
			const char *ref = json_object_get_string(data, "$ref");
			dataInfo->ref = ref;

			if (strncmp(ref, "#/definitions/", 14) != 0) {
				DebugBreak();
				continue;
			}

			JSON_Object *data2 = json_object_get_object(iot_pnp->el_definitions_object, &ref[14]);
			if (data2 == NULL) {
				DebugBreak();
				continue;
			}

			parse_data(iot_pnp, data2, dataInfo);
		}
		else {
			DebugBreak();
			continue;
		}
	}
}

void make_dt_interface(iot_pnp *iot_pnp, int index, unsigned short access_value,
	const char *propertyNameJa, const char *propertyNameEn, data_info *dataInfo);

#define ALL_ACCESS_RULE(g,s,i) (((i & 0xF) << 8) | ((s & 0xF) << 4) | ((g & 0xF) << 0))

void parse_property(iot_pnp *iot_pnp, JSON_Object *elProperty)
{
	const char *propertyNameJa = NULL, *propertyNameEn = NULL;
	union {
		struct {
			access_rule get_access : 4;
			access_rule set_access : 4;
			access_rule inf_access : 4;
		};
		unsigned short access_value;
	};
	data_info dataInfoImpl = { DATA_TYPE_NONE };

	access_value = 0;

	for (int i = 0; i < json_object_get_count(elProperty); i++) {
		const char *propertyMember = json_object_get_name(elProperty, i);

		if (strcmp(propertyMember, "validRelease") == 0) {
			JSON_Object *validRelease;

			validRelease = json_object_get_object(elProperty, "validRelease");
			if (validRelease == NULL) {
				DebugBreak();
				continue;
			}
		}
		else if (strcmp(propertyMember, "propertyName") == 0) {
			JSON_Object *propertyName;

			propertyName = json_object_get_object(elProperty, "propertyName");
			if (propertyName == NULL) {
				DebugBreak();
				continue;
			}

			propertyNameJa = json_object_get_string(propertyName, "ja");
			propertyNameEn = json_object_get_string(propertyName, "en");
		}
		else if (strcmp(propertyMember, "accessRule") == 0) {
			JSON_Object *accessRule;

			accessRule = json_object_get_object(elProperty, "accessRule");
			if (accessRule == NULL) {
				DebugBreak();
				continue;
			}

			for (int j = 0; j < json_object_get_count(accessRule); j++) {
				const char *access = json_object_get_name(accessRule, j);
				const char *rule;

				rule = json_object_get_string(accessRule, access);
				if (rule == NULL) {
					DebugBreak();
					continue;
				}

				if (strcmp(access, "get") == 0) {
					get_access = get_access_rule(rule);
				}
				else if (strcmp(access, "set") == 0) {
					set_access = get_access_rule(rule);
				}
				else if (strcmp(access, "inf") == 0) {
					inf_access = get_access_rule(rule);
				}
				else {
					DebugBreak();
					continue;
				}
			}
		}
		else if (strcmp(propertyMember, "data") == 0) {
			JSON_Object *data;

			data = json_object_get_object(elProperty, "data");
			if (data == NULL) {
				DebugBreak();
				continue;
			}

			parse_data(iot_pnp, data, &dataInfoImpl);
		}
		else if (strcmp(propertyMember, "oneOf") == 0) {
			JSON_Array *oneOf;

			oneOf = json_object_get_array(elProperty, "oneOf");
			if (oneOf == NULL) {
				DebugBreak();
				continue;
			}

			for (int j = 0; j < json_array_get_count(oneOf); j++) {
				JSON_Object *elProperty2;

				elProperty2 = json_array_get_object(oneOf, j);
				if (elProperty2 == NULL) {
					DebugBreak();
					continue;
				}

				parse_property(iot_pnp, elProperty2);
			}
		}
		else if (strcmp(propertyMember, "atomic") == 0) {
			const char *atomic;

			atomic = json_object_get_string(elProperty, "atomic");
			if (atomic == NULL) {
				DebugBreak();
				continue;
			}
		}
		else if (strcmp(propertyMember, "note") == 0) {
			JSON_Object *note;

			note = json_object_get_object(elProperty, "note");
			if (note == NULL) {
				DebugBreak();
				continue;
			}
		}
		else {
			DebugBreak();
			continue;
		}
	}

	if (dataInfoImpl.type == DATA_TYPE_ONE_OF) {
		data_info *dataInfo = dataInfoImpl.dataInfos;
		for (int i = 0; i < dataInfoImpl.dataInfoCount; i++, dataInfo++) {
			make_dt_interface(iot_pnp, i + 1, access_value, propertyNameJa, propertyNameEn, dataInfo);
		}
	}
	else {
		make_dt_interface(iot_pnp, 0, access_value, propertyNameJa, propertyNameEn, &dataInfoImpl);
	}

	free_data_info(&dataInfoImpl);
}

JSON_Value *make_schema(data_info *dataInfo)
{
	JSON_Value *result;

	switch (dataInfo->type) {
	case DATA_TYPE_STATE: {
		result = json_value_init_object();
		JSON_Object *schema = json_value_get_object(result);
		json_object_set_string(schema, "@type", "Enum");
		json_object_set_string(schema, "valueSchema", "integer");

		JSON_Value *enumValues_value = json_value_init_array();
		JSON_Array *enumValues = json_value_get_array(enumValues_value);
		json_object_set_value(schema, "enumValues", enumValues_value);

		edt_info *edt = dataInfo->edts;
		for (int i = 0; i < dataInfo->edtCount; i++, edt++) {
			JSON_Value *enumValue_value = json_value_init_object();
			JSON_Object *enumValue = json_value_get_object(enumValue_value);
			json_array_append_value(enumValues, enumValue_value);

			char name[65];
			if (edt->stateEn != NULL) {
				set_digital_twin_id(name, edt->stateEn, sizeof(name));
			}
			else {
				sprintf_s(name, "edt%x", edt->edt);
			}
			json_object_set_string(enumValue, "name", name);

			json_object_set_number(enumValue, "enumValue", edt->edt);

			if (edt->stateJa != NULL) {
				json_object_dotset_string(enumValue, "displayName.en", edt->stateEn);
				json_object_dotset_string(enumValue, "displayName.ja", edt->stateJa);
			}
			else {
				json_object_set_string(enumValue, "displayName", edt->stateEn);
			}
		}
		break;
	}
	case DATA_TYPE_OBJECT: {
		result = json_value_init_object();
		JSON_Object *schema = json_value_get_object(result);
		json_object_set_string(schema, "@type", "Object");

		JSON_Value *fields_value = json_value_init_array();
		JSON_Array *fields = json_value_get_array(fields_value);
		json_object_set_value(schema, "fields", fields_value);

		data_info *dataInfo2 = dataInfo->dataInfos;
		for (int i = 0; i < dataInfo->dataInfoCount; i++, dataInfo2++) {
			JSON_Value *field_value = json_value_init_object();
			JSON_Object *field = json_value_get_object(field_value);
			json_array_append_value(fields, field_value);

			json_object_set_string(field, "name", dataInfo2->name);
			JSON_Value *schema2 = make_schema(dataInfo2);
			json_object_set_value(field, "schema", schema2);
		}
		break;
	}
	case DATA_TYPE_DATE_TIME: {
		result = json_value_init_string("datetime");
		break;
	}
	case DATA_TYPE_TIME: {
		result = json_value_init_string("time");
		break;
	}
	case DATA_TYPE_RAW: {
		result = json_value_init_string("string");
		break;
	}
	case DATA_TYPE_ARRAY: {
		result = json_value_init_object();
		JSON_Object *schema = json_value_get_object(result);
		json_object_set_string(schema, "@type", "Array");
		break;
	}
	case DATA_TYPE_BITMAP: {
		result = json_value_init_object();
		JSON_Object *schema = json_value_get_object(result);
		json_object_set_string(schema, "@type", "Object");

		JSON_Value *fields_value = json_value_init_array();
		JSON_Array *fields = json_value_get_array(fields_value);
		json_object_set_value(schema, "fields", fields_value);

		bitmap_info *bitmapInfo = dataInfo->bitmapInfos;
		for (int i = 0; i < dataInfo->bitmapInfoCount; i++, bitmapInfo++) {
			JSON_Value *field_value = json_value_init_object();
			JSON_Object *field = json_value_get_object(field_value);
			json_array_append_value(fields, field_value);

			json_object_set_string(field, "name", bitmapInfo->name);
			JSON_Value *schema2 = make_schema(&bitmapInfo->value);
			json_object_set_value(field, "schema", schema2);
		}
		break;
	}
	case DATA_TYPE_LEVEL: {
		result = json_value_init_string("integer");
		break;
	}
	case DATA_TYPE_NUMBER: {
		switch (dataInfo->numFormat) {
		case NUMBER_FORMAT_INT8:
		case NUMBER_FORMAT_INT16:
		case NUMBER_FORMAT_INT32:
		case NUMBER_FORMAT_UINT8:
		case NUMBER_FORMAT_UINT16:
			result = json_value_init_string("integer");
			break;
		case NUMBER_FORMAT_UINT32:
			result = json_value_init_string("long");
			break;
		default:
			result = NULL;
			DebugBreak();
			break;
		}
		break;
	}
	case DATA_TYPE_NUMERIC_VALUE: {
		result = json_value_init_string("integer");
		break;
	}
	case DATA_TYPE_ONE_OF: {
		result = json_value_init_object();
		JSON_Object *schema = json_value_get_object(result);
		json_object_set_string(schema, "@type", "Object");

		JSON_Value *fields_value = json_value_init_array();
		JSON_Array *fields = json_value_get_array(fields_value);
		json_object_set_value(schema, "fields", fields_value);

		data_info *dataInfo2 = dataInfo->dataInfos;
		for (int i = 0; i < dataInfo->dataInfoCount; i++, dataInfo2++) {
			JSON_Value *field_value = json_value_init_object();
			JSON_Object *field = json_value_get_object(field_value);
			json_array_append_value(fields, field_value);

			json_object_set_string(field, "name", dataInfo2->name);
			JSON_Value *schema2 = make_schema(dataInfo2);
			json_object_set_value(field, "schema", schema2);
		}
		break;
	}
	default:
		result = NULL;
		DebugBreak();
		break;
	}

	return result;
}

JSON_Value *make_command_payload(const char *propertyNameJa, const char *propertyNameEn, data_info *dataInfo)
{
	JSON_Value *command_value = json_value_init_object();
	JSON_Object *command = json_value_get_object(command_value);

	char name[65];
	set_digital_twin_id(name, propertyNameEn, sizeof(name));

	char temp[256];
	sprintf_s(temp, "urn:EchonetLite:%s:1", name);

	json_object_set_string(command, "@id", temp);

	json_object_set_string(command, "name", name);

	JSON_Value *request = make_schema(dataInfo);
	json_object_set_value(command, "schema", request);

	if (propertyNameJa != NULL) {
		json_object_dotset_string(command, "displayName.en", propertyNameEn);
		json_object_dotset_string(command, "displayName.ja", propertyNameJa);
	}
	else {
		json_object_set_string(command, "displayName", propertyNameEn);
	}

	json_object_set_string(command, "displayUnit", dataInfo->unit);
	//json_object_set_value(command, "unit", );

	return command_value;
}

void make_dt_interface(iot_pnp *iot_pnp, int index, unsigned short access_value,
	const char *propertyNameJa, const char *propertyNameEn, data_info *dataInfo)
{
	if (propertyNameEn == NULL)
		return;

	dt_if_type if_type = DT_IF_TYPE_NONE;
	bool writable = false;

	switch (access_value)
	{
	case ALL_ACCESS_RULE(ACCESS_RULE_NA, ACCESS_RULE_OPTIONAL, ACCESS_RULE_NA):
		// GETなし
		if_type = DT_IF_TYPE_COMMAND;
		break;
	case ALL_ACCESS_RULE(ACCESS_RULE_NA, ACCESS_RULE_REQUIRED, ACCESS_RULE_OPTIONAL):
		// GETなし
		if_type = DT_IF_TYPE_COMMAND;
		break;
	case ALL_ACCESS_RULE(ACCESS_RULE_OPTIONAL, ACCESS_RULE_NA, ACCESS_RULE_REQUIRED):
	case ALL_ACCESS_RULE(ACCESS_RULE_REQUIRED, ACCESS_RULE_NA, ACCESS_RULE_REQUIRED):
	case ALL_ACCESS_RULE(ACCESS_RULE_BY_CASE, ACCESS_RULE_NA, ACCESS_RULE_REQUIRED):
		// SETなし
		if_type = DT_IF_TYPE_TELEMETRY;
		break;
	case ALL_ACCESS_RULE(ACCESS_RULE_OPTIONAL, ACCESS_RULE_NA, ACCESS_RULE_OPTIONAL):
	case ALL_ACCESS_RULE(ACCESS_RULE_REQUIRED, ACCESS_RULE_NA, ACCESS_RULE_OPTIONAL):
	case ALL_ACCESS_RULE(ACCESS_RULE_BY_CASE, ACCESS_RULE_NA, ACCESS_RULE_OPTIONAL):
		// SETなし
		if_type = (dataInfo->type == DATA_TYPE_STATE) ? DT_IF_TYPE_TELEMETRY : DT_IF_TYPE_PROPERTY;
		break;
	case ALL_ACCESS_RULE(ACCESS_RULE_REQUIRED, ACCESS_RULE_REQUIRED, ACCESS_RULE_NA):
		// GET/SET
		if_type = DT_IF_TYPE_PROPERTY;
		writable = true;
		break;
	case ALL_ACCESS_RULE(ACCESS_RULE_OPTIONAL, ACCESS_RULE_OPTIONAL, ACCESS_RULE_REQUIRED):
	case ALL_ACCESS_RULE(ACCESS_RULE_REQUIRED, ACCESS_RULE_OPTIONAL, ACCESS_RULE_REQUIRED):
	case ALL_ACCESS_RULE(ACCESS_RULE_REQUIRED, ACCESS_RULE_REQUIRED, ACCESS_RULE_REQUIRED):
	case ALL_ACCESS_RULE(ACCESS_RULE_BY_CASE, ACCESS_RULE_BY_CASE, ACCESS_RULE_REQUIRED):
		// GET/SET
		if_type = DT_IF_TYPE_PROPERTY;
		writable = true;
		break;
	case ALL_ACCESS_RULE(ACCESS_RULE_OPTIONAL, ACCESS_RULE_OPTIONAL, ACCESS_RULE_OPTIONAL):
		// GET/SET
		if_type = DT_IF_TYPE_PROPERTY;
		writable = true;
		break;
	case ALL_ACCESS_RULE(ACCESS_RULE_REQUIRED, ACCESS_RULE_OPTIONAL, ACCESS_RULE_OPTIONAL):
	case ALL_ACCESS_RULE(ACCESS_RULE_BY_CASE, ACCESS_RULE_OPTIONAL, ACCESS_RULE_OPTIONAL):
		// GET/SET
		if_type = DT_IF_TYPE_PROPERTY;
		writable = true;
		break;
	case ALL_ACCESS_RULE(ACCESS_RULE_REQUIRED, ACCESS_RULE_REQUIRED, ACCESS_RULE_OPTIONAL):
	case ALL_ACCESS_RULE(ACCESS_RULE_BY_CASE, ACCESS_RULE_BY_CASE, ACCESS_RULE_OPTIONAL):
		// GET/SET
		if_type = DT_IF_TYPE_PROPERTY;
		writable = true;
		break;
	default:
		DebugBreak();
		return;
	}

	if ((if_type == DT_IF_TYPE_COMMAND) && (dataInfo->type == DATA_TYPE_STATE)) {
		edt_info *edt = dataInfo->edts;
		for (int i = 0; i < dataInfo->edtCount; i++, edt++) {
			JSON_Value *command_value = json_value_init_object();
			JSON_Object *command = json_value_get_object(command_value);

			char name[65];
			if (edt->stateEn != NULL) {
				set_digital_twin_id(name, edt->stateEn, sizeof(name));
			}
			else {
				sprintf_s(name, "edt%x", edt->edt);
			}

			json_object_set_string(command, "@type", "Command");

			json_object_set_string(command, "@context", "http://azureiot.com/v1/contexts/IoTModel.json");

			json_object_set_string(command, "name", name);

			if (edt->stateJa != NULL) {
				json_object_dotset_string(command, "displayName.en", edt->stateEn);
				json_object_dotset_string(command, "displayName.ja", edt->stateJa);
			}
			else {
				json_object_set_string(command, "displayName", edt->stateEn);
			}

			json_object_set_string(command, "commandType", "synchronous");
			//json_object_set_string(command, "commandType", "asynchronous");

			json_array_append_value(iot_pnp->dt_contents_array, command_value);
		}
	}
	else {
		JSON_Value *ifcnt = json_value_init_object();
		JSON_Object *if_content = json_value_get_object(ifcnt);

		char name[65];
		set_digital_twin_id(name, propertyNameEn, sizeof(name));

		char temp[256];
		if (index == 0) {
			sprintf_s(temp, "urn:EchonetLite:%s:1", name);
		}
		else {
			sprintf_s(temp, "urn:EchonetLite:%s%d:1", name, index + 1);
		}

		json_object_set_string(if_content, "@id", temp);

		switch (if_type) {
		case DT_IF_TYPE_COMMAND:
			json_object_set_string(if_content, "@type", "Command");
			break;
		case DT_IF_TYPE_TELEMETRY:
			json_object_set_string(if_content, "@type", "Telemetry");
			break;
		case DT_IF_TYPE_PROPERTY:
			json_object_set_string(if_content, "@type", "Property");
			break;
		default:
			DebugBreak();
			json_value_free(ifcnt);
			return;
		}

		json_object_set_string(if_content, "@context", "http://azureiot.com/v1/contexts/IoTModel.json");

		json_object_set_string(if_content, "name", name);

		switch (if_type) {
		case DT_IF_TYPE_TELEMETRY:
		case DT_IF_TYPE_PROPERTY:
			JSON_Value *request = make_schema(dataInfo);
			json_object_set_value(if_content, "schema", request);
			break;
		}

		if (propertyNameJa != NULL) {
			json_object_dotset_string(if_content, "displayName.en", propertyNameEn);
			json_object_dotset_string(if_content, "displayName.ja", propertyNameJa);
		}
		else {
			json_object_set_string(if_content, "displayName", propertyNameEn);
		}

		switch (if_type) {
		case DT_IF_TYPE_COMMAND: {
			json_object_set_string(if_content, "commandType", "synchronous");
			//json_object_set_string(if_content, "commandType", "asynchronous");
			JSON_Value *request = make_command_payload(propertyNameJa, propertyNameEn, dataInfo);
			json_object_set_value(if_content, "request", request);
			JSON_Value *response = make_command_payload(propertyNameJa, propertyNameEn, dataInfo);
			json_object_set_value(if_content, "response", response);
			break;
		}
		case DT_IF_TYPE_TELEMETRY:
			if (dataInfo->unit != NULL) {
				json_object_set_string(if_content, "displayUnit", dataInfo->unit);
			}
			//json_object_set_value(if_content, "unit", );
			break;
		case DT_IF_TYPE_PROPERTY:
			if (dataInfo->unit != NULL) {
				json_object_set_string(if_content, "displayUnit", dataInfo->unit);
			}
			//json_object_set_value(if_content, "unit", );
			json_object_set_boolean(if_content, "writable", writable);
			break;
		default:
			DebugBreak();
			json_value_free(ifcnt);
			return;
		}

		json_array_append_value(iot_pnp->dt_contents_array, ifcnt);
	}
}

void parse_device(iot_pnp *iot_pnp, JSON_Object *device)
{
	const char *classNameJa = NULL, *classNameEn = NULL;

	for (int i = 0; i < json_object_get_count(device); i++) {
		const char *deviceMember = json_object_get_name(device, i);

		if (strcmp(deviceMember, "validRelease") == 0) {
			JSON_Object *validRelease;

			validRelease = json_object_get_object(device, "validRelease");
			if (validRelease == NULL) {
				DebugBreak();
				continue;
			}
		}
		else if (strcmp(deviceMember, "className") == 0) {
			JSON_Object *className;

			className = json_object_get_object(device, "className");
			if (className == NULL) {
				DebugBreak();
				continue;
			}

			classNameJa = json_object_get_string(className, "ja");
			classNameEn = json_object_get_string(className, "en");
		}
		else if (strcmp(deviceMember, "elProperties") == 0) {
			JSON_Object *elProperties;

			elProperties = json_object_get_object(device, "elProperties");
			if (elProperties == NULL) {
				DebugBreak();
				continue;
			}

			for (int j = 0; j < json_object_get_count(elProperties); j++) {
				const char *propertieId = json_object_get_name(elProperties, j);
				JSON_Object *elProperty;

				elProperty = json_object_get_object(elProperties, propertieId);
				if (elProperty == NULL) {
					DebugBreak();
					continue;
				}

				if (iot_pnp->dt_contents_value == NULL) {
					iot_pnp->dt_contents_value = json_value_init_array();
					iot_pnp->dt_contents_array = json_value_get_array(iot_pnp->dt_contents_value);
				}
				parse_property(iot_pnp, elProperty);
			}
		}
		else if (strcmp(deviceMember, "firstRelease") == 0) {
			const char *firstRelease;

			firstRelease = json_object_get_string(device, "firstRelease");
			if (firstRelease == NULL) {
				DebugBreak();
				continue;
			}
		}
		else if (strcmp(deviceMember, "oneOf") == 0) {
			JSON_Array *oneOf;

			oneOf = json_object_get_array(device, "oneOf");
			if (oneOf == NULL) {
				DebugBreak();
				continue;
			}

			for (int j = 0; j < json_array_get_count(oneOf); j++) {
				JSON_Object *device2;

				device2 = json_array_get_object(oneOf, j);
				if (device2 == NULL) {
					DebugBreak();
					continue;
				}

				parse_device(iot_pnp, device2);
			}
		}
		else {
			DebugBreak();
			continue;
		}
	}

	if (classNameEn != NULL) {
		JSON_Value *cm = json_value_init_object();
		JSON_Object *dt_interface = json_value_get_object(cm);
		json_array_append_value(iot_pnp->dt_root_array, cm);

		char temp[256];
		strcpy_s(temp, "urn:EchonetLite:");
		int len = strlen(temp);
		len += set_digital_twin_id(&temp[len], classNameEn, sizeof(temp) - len - 2);
		temp[len++] = ':';
		temp[len++] = '1';
		temp[len++] = '\0';

		json_object_set_string(dt_interface, "@id", temp);
		json_object_set_string(dt_interface, "@type", "Interface");
		json_object_set_string(dt_interface, "@context", "http://azureiot.com/v1/contexts/IoTModel.json");

		if (classNameJa != NULL) {
			json_object_dotset_string(dt_interface, "displayName.en", classNameEn);
			json_object_dotset_string(dt_interface, "displayName.ja", classNameJa);
		}
		else {
			json_object_set_string(dt_interface, "displayName", classNameEn);
		}

		json_object_set_value(dt_interface, "contents", iot_pnp->dt_contents_value);
	}
	else {
		json_value_free(iot_pnp->dt_contents_value);
	}

	iot_pnp->dt_contents_value = NULL;
	iot_pnp->dt_contents_array = NULL;
}

#define MEM_DEBUG

int main()
{
#ifdef MEM_DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	JSON_Value *el_root_value;
	JSON_Object *el_root, *el_devices;
	char filename[] = "AppendixData\\EL_DeviceDescription_3_1_5r4.json";
	iot_pnp iot_pnp;

	memset(&iot_pnp, 0, sizeof(iot_pnp));

	el_root_value = json_parse_file(filename);
	el_root = json_value_get_object(el_root_value);
	if (el_root == NULL) {
		return -1;
	}

	iot_pnp.el_definitions_value = json_object_get_value(el_root, "definitions");
	if (iot_pnp.el_definitions_value == NULL) {
		return -1;
	}

	iot_pnp.el_definitions_object = json_value_get_object(iot_pnp.el_definitions_value);
	if (iot_pnp.el_definitions_object == NULL) {
		return -1;
	}

	el_devices = json_object_get_object(el_root, "devices");
	if (el_devices == NULL) {
		return -1;
	}

	iot_pnp.dt_root_value = json_value_init_array();
	iot_pnp.dt_root_array = json_value_get_array(iot_pnp.dt_root_value);

	for (int i = 0; i < json_object_get_count(el_devices); i++) {
		const char *deviceId = json_object_get_name(el_devices, i);
		JSON_Object *device;

		device = json_object_get_object(el_devices, deviceId);
		if (device == NULL) {
			DebugBreak();
			continue;
		}

		parse_device(&iot_pnp, device);
	}

	json_value_free(el_root_value);

	json_set_escape_slashes(0);
	json_serialize_to_file_pretty(iot_pnp.dt_root_value, "el_iot_pnp.json");
	json_value_free(iot_pnp.dt_root_value);

#ifdef MEM_DEBUG
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
