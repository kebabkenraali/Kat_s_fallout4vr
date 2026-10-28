Scriptname KATLocomotionVR_MCM Hidden
{The - and + buttons of the KAT Locomotion VR MCM page: step a slider's value, since VR sliders are hard to drag.}

; Add afDelta to the float MCM setting asSetting ("fInteriorSpeedMult:Main"), keep it within afMin..afMax,
; then refresh the menu so the slider shows the new value. The plugin picks the change up from MCM's settings file.
Function StepFloat(string asSetting, float afDelta, float afMin, float afMax) global
	float value = MCM.GetModSettingFloat("KATLocomotionVR", asSetting) + afDelta
	; round to 2 decimals so repeated steps of 0.1 don't drift to 1.4000001
	value = Math.Floor(value * 100.0 + 0.5) / 100.0
	if value < afMin
		value = afMin
	elseif value > afMax
		value = afMax
	endif
	MCM.SetModSettingFloat("KATLocomotionVR", asSetting, value)
	MCM.RefreshMenu()
EndFunction
