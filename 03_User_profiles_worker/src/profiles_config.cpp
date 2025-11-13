#include <cmath>
#include <limits>

#include "profiles_config.h"

namespace user {

	bool floatsNotEqual(float a, float b) {
		
		return std::abs(a - b) > USER_FLOAT_EPSILON;
	}

	bool TPprofile::operator!=(const TPprofile& other) const {
		
		return name != other.name ||
			correctionX != other.correctionX ||
			correctionY != other.correctionY ||
			correctionH != other.correctionH ||
			correctionV != other.correctionV ||
			brightnessOLED != other.brightnessOLED ||
			contrastOLED != other.contrastOLED ||
			zoomTF != other.zoomTF ||
			pictureProfileIndex != other.pictureProfileIndex ||
			contrastPredust != other.contrastPredust ||
			gammaPredust != other.gammaPredust ||
			palette != other.palette ||
			paletteWidth != other.paletteWidth ||
			paletteOffset != other.paletteOffset ||
			pricelType != other.pricelType ||
			pricelColor != other.pricelColor ||
			pricelBright != other.pricelBright;
	}

	bool TPprofiles::operator!=(const TPprofiles& other) const {
		
		if (indexOfSelected != other.indexOfSelected) {

			return true;
		}

		for (size_t i = 0; i < USER_PROFILES_NUMBER_MAX; ++i) {

			if (profile[i] != other.profile[i]) {

				return true;
			}
		}

		return false;
	}

	bool TPCommonSettings::operator!=(const TPCommonSettings& other) const {

		return Menu_Main_Number != other.Menu_Main_Number ||
			Menu_TimeOut != other.Menu_TimeOut ||
			Lang != other.Lang ||
			OLED_shift_x != other.OLED_shift_x ||
			OLED_shift_y != other.OLED_shift_y ||
			OLED_shift_dir_x != other.OLED_shift_dir_x ||
			OLED_shift_dir_y != other.OLED_shift_dir_y ||
			Dalnomer_TargetSize != other.Dalnomer_TargetSize ||
			Dalnomer_TargetSizeIndex != other.Dalnomer_TargetSizeIndex ||
			Dalnomer_StepSize != other.Dalnomer_StepSize ||
			Dalnomer_BoxSize != other.Dalnomer_BoxSize ||
			Dalnomer_BoxSize_Last != other.Dalnomer_BoxSize_Last ||
			floatsNotEqual(Dalnomer_Distance, other.Dalnomer_Distance) ||
			Dalnomer_OnAim != other.Dalnomer_OnAim ||
			BadPixThreshold != other.BadPixThreshold ||
			AimFlashCenter != other.AimFlashCenter ||
			ZoomInNM != other.ZoomInNM ||
			ChangeTPInNM != other.ChangeTPInNM ||
			INVencL != other.INVencL ||
			INVencT != other.INVencT ||
			INVencR != other.INVencR ||
			ZoomStep != other.ZoomStep;
	}

	bool TPCommonSettingsHW::operator!=(const TPCommonSettingsHW& other) const {
		
		return DisplayModel != other.DisplayModel ||
			DisplayRes != other.DisplayRes ||
			ModuleType != other.ModuleType ||
			floatsNotEqual(Module_ver, other.Module_ver) ||
			floatsNotEqual(PixelStep, other.PixelStep) ||
			fAutoDetectedOLEDModel != other.fAutoDetectedOLEDModel ||
			fAutoDetectedOLEDRes != other.fAutoDetectedOLEDRes ||
			fAutoDetectedModule != other.fAutoDetectedModule ||
			fAutoDetectedModuleVer != other.fAutoDetectedModuleVer ||
			FocalLenght != other.FocalLenght ||
			PixelSize != other.PixelSize ||
			floatsNotEqual(ScaleSensorOLED, other.ScaleSensorOLED) ||
			floatsNotEqual(ADC_BatteryCorr, other.ADC_BatteryCorr) ||
			floatsNotEqual(ADC_TemperatureCorr, other.ADC_TemperatureCorr) ||
			MountAngle != other.MountAngle ||
			DisplayResX != other.DisplayResX ||
			DisplayResY != other.DisplayResY ||
			DisplayResX2 != other.DisplayResX2 ||
			DisplayResY2 != other.DisplayResY2 ||
			ModuleResX != other.ModuleResX ||
			ModuleResY != other.ModuleResY ||
			ModuleResX2 != other.ModuleResX2 ||
			ModuleResY2 != other.ModuleResY2;
	}

} /* namespace user */