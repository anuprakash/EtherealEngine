#pragma once

#include "../../interface/gui_system.h"
#include "core/serialization/serialization.h"
#include "core/logging/logging.h"

SAVE(GUIStyle::HSVSetup)
{
	try_save(ar, cereal::make_nvp("col_main_hue", obj.col_main_hue));
	try_save(ar, cereal::make_nvp("col_main_sat", obj.col_main_sat));
	try_save(ar, cereal::make_nvp("col_main_val", obj.col_main_val));
	try_save(ar, cereal::make_nvp("col_area_hue", obj.col_area_hue));
	try_save(ar, cereal::make_nvp("col_area_sat", obj.col_area_sat));
	try_save(ar, cereal::make_nvp("col_area_val", obj.col_area_val));
	try_save(ar, cereal::make_nvp("col_back_hue", obj.col_back_hue));
	try_save(ar, cereal::make_nvp("col_back_sat", obj.col_back_sat));
	try_save(ar, cereal::make_nvp("col_back_val", obj.col_back_val));
	try_save(ar, cereal::make_nvp("col_text_hue", obj.col_text_hue));
	try_save(ar, cereal::make_nvp("col_text_sat", obj.col_text_sat));
	try_save(ar, cereal::make_nvp("col_text_val", obj.col_text_val));
	try_save(ar, cereal::make_nvp("frame_rounding", obj.frame_rounding));
}

LOAD(GUIStyle::HSVSetup)
{
	try_load(ar, cereal::make_nvp("col_main_hue", obj.col_main_hue));
	try_load(ar, cereal::make_nvp("col_main_sat", obj.col_main_sat));
	try_load(ar, cereal::make_nvp("col_main_val", obj.col_main_val));
	try_load(ar, cereal::make_nvp("col_area_hue", obj.col_area_hue));
	try_load(ar, cereal::make_nvp("col_area_sat", obj.col_area_sat));
	try_load(ar, cereal::make_nvp("col_area_val", obj.col_area_val));
	try_load(ar, cereal::make_nvp("col_back_hue", obj.col_back_hue));
	try_load(ar, cereal::make_nvp("col_back_sat", obj.col_back_sat));
	try_load(ar, cereal::make_nvp("col_back_val", obj.col_back_val));
	try_load(ar, cereal::make_nvp("col_text_hue", obj.col_text_hue));
	try_load(ar, cereal::make_nvp("col_text_sat", obj.col_text_sat));
	try_load(ar, cereal::make_nvp("col_text_val", obj.col_text_val));
	try_load(ar, cereal::make_nvp("frame_rounding", obj.frame_rounding));
}
